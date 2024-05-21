#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fec.h>
#include <galois.h>

int fec_init(fec **ctx, fec_param *param) {
    *ctx = malloc(sizeof(fec));
    if (!*ctx) {
        return -1;
    }

    (*ctx)->param = param;
    (*ctx)->encode_seq = 0;
    (*ctx)->encode_count = 0;

    galois_init(param->gf_power);

    /* Allocate memory for the encode buffer */
    (*ctx)->encode_buf = malloc(param->n * sizeof(GF_ELEMENT *));
    if (!(*ctx)->encode_buf) {
        return -1;
    }
    for (int i = 0; i < param->n; i++) {
        (*ctx)->encode_buf[i] = malloc((RTP_HDR_LEN + param->rtp_payload_size) * sizeof(GF_ELEMENT));
        if (!(*ctx)->encode_buf[i]) {
            return -1;
        }
    }

    /* Allocate memory for the encode packets */
    (*ctx)->encode_pkts = malloc(MAX_PACKET_NUM * sizeof(fec_packet));
    if (!(*ctx)->encode_pkts) {
        return -1;
    }
    for (int i = 0; i < MAX_PACKET_NUM; i++) {
        // TODO: check the real size of the packet
        (*ctx)->encode_pkts[i].buf = malloc(param->rtp_payload_size + param->gen_size + RTP_HDR_LEN * 2);
        if (!(*ctx)->encode_pkts[i].buf) {
            return -1;
        }
    }

    return 0;
}

int fec_encode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count) {
    int out_pkt_count = 0;
    int n = ctx->param->n;
    unsigned short seq;         // Sequence number
    GF_ELEMENT *coeff = NULL;   // Pointer to the coding vector of the RTP repair packet
    GF_ELEMENT *payload = NULL; // Pointer to the payload of the RTP repair packet

    seq = ((char *)pkt)[2] << 8 | ((char *)pkt)[3];

    /* Check if the packet is within the generation window */
    if (seq >= ctx->encode_seq + ctx->param->gen_size || ctx->encode_seq == 0) {
        /* Set new generation window */
        ctx->encode_seq = seq;
        ctx->encode_count = 0;
    }

    /* Copy the packet into the encode buffer */
    memcpy(ctx->encode_buf[ctx->encode_count], pkt, len);

    /* Copy the source RTP packet into the out_pkts */
    memcpy(ctx->encode_pkts[ctx->encode_count].buf, pkt, len);
    ctx->encode_pkts[ctx->encode_count].len = len;

    out_pkts[out_pkt_count++] = ctx->encode_pkts[ctx->encode_count++]; // Set the output packets

    /* Return if we have not received enough packets to generate repair packets */
    if (ctx->encode_count < ctx->param->gen_size) {
        *count = out_pkt_count;
        return 0;
    }

    /* Create (n - k) repair packets */
    for (int i = 0; i < n - ctx->param->gen_size; i++) {
        /* Set the pointers to the coefficient and payload */
        coeff = ctx->encode_pkts[ctx->encode_count].buf + RTP_HDR_LEN;
        payload = ctx->encode_pkts[ctx->encode_count].buf + RTP_HDR_LEN + ctx->param->gen_size;

        /* Initialize the payload */
        memset(payload, 0, (RTP_HDR_LEN + ctx->param->rtp_payload_size) * sizeof(GF_ELEMENT));

        /* Copy the RTP header */
        memcpy(ctx->encode_pkts[ctx->encode_count].buf, pkt, RTP_HDR_LEN);

        /* Modify the PT field to the repair packet type */
        ((char *)ctx->encode_pkts[ctx->encode_count].buf)[1] &= 0x80;           // Clear the PT field
        ((char *)ctx->encode_pkts[ctx->encode_count].buf)[1] |= ctx->param->pt; // Set the PT field

        /* Generate the RTP repair packet payload */
        for (int j = 0; j < ctx->param->gen_size; j++) {
            unsigned char coefficient;
            coefficient = rand() % (1 << ctx->param->gf_power);

            /* Set the coefficient in the encode matrix */
            coeff[j] = coefficient;

            for (int k = 0; k < RTP_HDR_LEN + ctx->param->rtp_payload_size; k++) {
                if (coefficient == 0) {
                    continue;
                }

                /* payload[k] = payload[k] + c * encode_buf[j][k] */
                payload[k] = galois_add(payload[k], galois_mul(coefficient, ctx->encode_buf[j][k]));
            }
        }

        /* Insert the repair packet into the out_pkts */
        ctx->encode_pkts[ctx->encode_count].len = ctx->param->rtp_payload_size + ctx->param->gen_size + RTP_HDR_LEN * 2;
        out_pkts[out_pkt_count] = ctx->encode_pkts[ctx->encode_count];

        out_pkt_count++;
        ctx->encode_count++;
    }

    *count = out_pkt_count;

    return 0;
}

int fec_decode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count) {
    unsigned char cc;           // CSRC count
    unsigned char pt;           // Payload type
    unsigned short seq;         // Sequence number
    GF_ELEMENT *coeffs = NULL;  // pointer to ctx->coeff_mat[]
    GF_ELEMENT *payload = NULL; // pointer to ctx->payload_mat[]

    /* Get sequence number of the incoming RTP packet */
    seq = ((char *)pkt)[2] << 8 | ((char *)pkt)[3];

    /* Check if the sequence number is within the generation window */
    if (seq < ctx->decode_seq) {
        /* Drop the packet */
        *count = 0;
        return 0;
    } else if (ctx->decode_seq + ctx->param->gen_size <= seq) {
        /* Clear the decode matrix */
        for (int i = 0; i < ctx->param->gen_size; i++) {
            memset(ctx->decode_payload[i], 0,
                   (RTP_HDR_LEN + ctx->param->rtp_payload_size) * sizeof(GF_ELEMENT));
            memset(ctx->decode_coeff[i], 0, ctx->param->gen_size * sizeof(GF_ELEMENT));
        }
        /* Set new generation window */
        ctx->decode_seq = seq;
    }

    pt = ((char *)pkt)[1] & 0x7F;
    if (pt != ctx->param->pt) {
        /* Insert the source RTP packet into the out_pkts */
        memcpy(ctx->encode_pkts[ctx->rank].buf, pkt, len);
        ctx->encode_pkts[ctx->rank].len = len;
        out_pkts[0] = ctx->encode_pkts[ctx->rank];

        *count = 1;
        return 0;
    }

    /* If the packet is a repair packet, we need to extract the repair payload */
    cc = ((char *)pkt)[0] & 0x0F; // CSRC count
    int rtp_hdr_len = 12 + cc * 4;

    /* Extract the encode matrix from the repair packet */
    coeffs = ctx->decode_coeff[ctx->rank];
    memcpy(coeffs, pkt + rtp_hdr_len, ctx->param->gen_size);
    payload = ctx->decode_payload[ctx->rank];
    memcpy(payload, pkt + rtp_hdr_len + ctx->param->gen_size, RTP_HDR_LEN + ctx->param->rtp_payload_size);

    /* upper triangular matrix */

    /* If there are any packet that have been decoded, insert them into the
     * out_pkts */

    return 0;
}
