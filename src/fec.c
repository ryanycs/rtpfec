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
    // (*ctx)->rank = 0;
    (*ctx)->encode_count = 0;

    galois_init(param->gf_power);

    /* Allocate memory for the coefficient matrix */
    (*ctx)->encode_coeff = malloc(param->n * sizeof(GF_ELEMENT *));
    if (!(*ctx)->encode_coeff) {
        return -1;
    }
    for (int i = 0; i < param->n; i++) {
        (*ctx)->encode_coeff[i] = malloc(param->gen_size * sizeof(GF_ELEMENT));
        if (!(*ctx)->encode_coeff[i]) {
            return -1;
        }
    }

    /* Allocate memory for the payload matrix */
    (*ctx)->encode_payload = malloc(param->n * sizeof(GF_ELEMENT *));
    if (!(*ctx)->encode_payload) {
        return -1;
    }
    for (int i = 0; i < param->n; i++) {
        (*ctx)->encode_payload[i] = malloc((RTP_HDR_LEN + param->rtp_payload_size) * sizeof(GF_ELEMENT));
        if (!(*ctx)->encode_payload[i]) {
            return -1;
        }
    }

    /* Allocate memory for the output packets */
    (*ctx)->out_pkts = malloc(MAX_PACKET_NUM * sizeof(fec_packet));
    if (!(*ctx)->out_pkts) {
        return -1;
    }
    for (int i = 0; i < MAX_PACKET_NUM; i++) {
        // TODO: check the real size of the packet
        (*ctx)->out_pkts[i].buf = malloc(param->rtp_payload_size + param->gen_size + RTP_HDR_LEN * 2);
        if (!(*ctx)->out_pkts[i].buf) {
            return -1;
        }
    }

    return 0;
}

int fec_encode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count) {
    int out_pkt_count = 0;
    int n = ctx->param->n;
    int k = ctx->param->gen_size;
    int coeff_len = ctx->param->gen_size;
    unsigned short seq;         // Sequence number
    GF_ELEMENT *coeff = NULL;   // Coefficients of the encode matrix
    GF_ELEMENT *payload = NULL; // Payload of the encode matrix

    seq = ((char *)pkt)[2] << 8 | ((char *)pkt)[3];

    /* Check if the packet is within the generation window */
    if (seq >= ctx->encode_seq + ctx->param->gen_size || ctx->encode_seq == 0) {
        /* Set new generation window */
        ctx->encode_seq = seq;
        ctx->encode_count = 0;
    }

    /* Insert the packet into the encode matrix */
    payload = ctx->encode_payload[ctx->encode_count];
    memcpy(payload, pkt, len);

    /* Insert the source RTP packet into the out_pkts */
    memcpy(ctx->out_pkts[ctx->encode_count].buf, pkt, len);
    ctx->out_pkts[ctx->encode_count].len = len;

    out_pkts[out_pkt_count++] = ctx->out_pkts[ctx->encode_count++]; // Set the output packets

    /* Return if we have not received enough packets to generate repair packets */
    if (ctx->encode_count < ctx->param->gen_size) {
        *count = out_pkt_count;
        return 0;
    }

    /* Create (n - k) repair packets */
    for (int i = 0; i < n - k; i++) {
        /* Copy the RTP header */
        memcpy(ctx->out_pkts[ctx->encode_count].buf, pkt, RTP_HDR_LEN);

        /* Modify the PT field to the repair packet type */
        ((char *)ctx->out_pkts[ctx->encode_count].buf)[1] &= 0x80;           // Clear the PT field
        ((char *)ctx->out_pkts[ctx->encode_count].buf)[1] |= ctx->param->pt; // Set the PT field

        /* initialize the encode matrix */
        coeff = ctx->encode_coeff[ctx->encode_count];
        memset(coeff, 0, coeff_len * sizeof(GF_ELEMENT));
        payload = ctx->encode_payload[ctx->encode_count];
        memset(payload, 0, (RTP_HDR_LEN + ctx->param->rtp_payload_size) * sizeof(GF_ELEMENT));
        printf("done\n");

        /* Generate the RTP repair packet payload */
        for (int j = 0; j < ctx->param->gen_size; j++) {
            unsigned char c; // Coefficient
            c = rand() % (1 << ctx->param->gf_power);

            /* Set the coefficient in the encode matrix */
            coeff[j] = c;

            for (int k = 0; k < RTP_HDR_LEN + ctx->param->rtp_payload_size; k++) {
                if (c == 0) {
                    continue;
                }

                /* payload[k] = payload[k] + coeff * ctx->payload_mat[j][k] */
                payload[k] = galois_add(payload[k], galois_mul(c, ctx->encode_payload[j][k]));
            }
        }

        /* Insert the repair packet into the out_pkts */
        memcpy(ctx->out_pkts[ctx->encode_count].buf + RTP_HDR_LEN, coeff, coeff_len);
        memcpy(ctx->out_pkts[ctx->encode_count].buf + RTP_HDR_LEN + ctx->param->gen_size, payload, RTP_HDR_LEN + ctx->param->rtp_payload_size);
        ctx->out_pkts[ctx->encode_count].len = ctx->param->rtp_payload_size + ctx->param->gen_size + RTP_HDR_LEN * 2;
        out_pkts[out_pkt_count] = ctx->out_pkts[ctx->encode_count];

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
    if (seq < ctx->seq) {
        /* Drop the packet */
        *count = 0;
        return 0;
    } else if (ctx->seq + ctx->param->gen_size <= seq) {
        /* Clear the decode matrix */
        for (int i = 0; i < ctx->param->gen_size; i++) {
            memset(ctx->payload_mat[i], 0,
                   (RTP_HDR_LEN + ctx->param->rtp_payload_size) * sizeof(GF_ELEMENT));
            memset(ctx->coeff_mat[i], 0, ctx->param->gen_size * sizeof(GF_ELEMENT));
        }
        /* Set new generation window */
        ctx->seq = seq;
    }

    pt = ((char *)pkt)[1] & 0x7F;
    if (pt != ctx->param->pt) {
        /* Insert the source RTP packet into the out_pkts */
        memcpy(ctx->out_pkts[ctx->out_pkt_count].buf, pkt, len);
        ctx->out_pkts[ctx->out_pkt_count].len = len;
        out_pkts[0] = ctx->out_pkts[ctx->out_pkt_count];

        *count = 1;
        return 0;
    }

    /* If the packet is a repair packet, we need to extract the repair payload */
    cc = ((char *)pkt)[0] & 0x0F; // CSRC count
    int rtp_hdr_len = 12 + cc * 4;

    /* Extract the encode matrix from the repair packet */
    coeffs = ctx->coeff_mat[ctx->rank];
    memcpy(coeffs, pkt + rtp_hdr_len, ctx->param->gen_size);
    payload = ctx->payload_mat[ctx->rank];
    memcpy(payload, pkt + rtp_hdr_len + ctx->param->gen_size, RTP_HDR_LEN + ctx->param->rtp_payload_size);

    /* upper triangular matrix */

    /* If there are any packet that have been decoded, insert them into the
     * out_pkts */

    return 0;
}
