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

    /* Initialize the encoding context */
    (*ctx)->param = param;
    (*ctx)->encode_gen = -1;
    (*ctx)->encode_count = 0;

    /* Initialize the decoding context */
    (*ctx)->decode_gen = -1;
    (*ctx)->decode_rank = 0;

    /* Initialize the galois field */
    galois_init(param->gf_power);

    /* Allocate memory for the encode buffer */
    (*ctx)->encode_buf = malloc(param->packet_num * sizeof(GF_ELEMENT *));
    if (!(*ctx)->encode_buf) {
        return -1;
    }
    for (int i = 0; i < param->packet_num; i++) {
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

    /* Allocate memory for the decode coefficient matrix */
    (*ctx)->decode_coeff = malloc(param->gen_size * sizeof(GF_ELEMENT *));
    if (!(*ctx)->decode_coeff) {
        return -1;
    }
    for (int i = 0; i < param->gen_size; i++) {
        (*ctx)->decode_coeff[i] = malloc(param->gen_size * sizeof(GF_ELEMENT));
        if (!(*ctx)->decode_coeff[i]) {
            return -1;
        }
    }

    /* Allocate memory for the decode payload */
    (*ctx)->decode_payload = malloc(param->gen_size * sizeof(GF_ELEMENT *));
    if (!(*ctx)->decode_payload) {
        return -1;
    }
    for (int i = 0; i < param->gen_size; i++) {
        (*ctx)->decode_payload[i] = malloc((RTP_HDR_LEN + param->rtp_payload_size) * sizeof(GF_ELEMENT));
        if (!(*ctx)->decode_payload[i]) {
            return -1;
        }
    }

    /* Allocate memory for the been_decoded flag */
    (*ctx)->been_decoded = malloc(param->gen_size * sizeof(int));
    if (!(*ctx)->been_decoded) {
        return -1;
    }
    // memset((*ctx)->been_decoded, 0, param->gen_size * sizeof(int));

    return 0;
}

int fec_encode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count) {
    int out_pkt_count = 0;
    int n = ctx->param->packet_num;
    int generation;
    unsigned short seq = ((char *)pkt)[2] << 8 | ((char *)pkt)[3]; // Sequence number
    GF_ELEMENT *coeff = NULL;                                      // Pointer to the coding vector of the RTP repair packet
    GF_ELEMENT *payload = NULL;                                    // Pointer to the payload of the RTP repair packet

    generation = seq / ctx->param->gen_size;

    /* Check if the packet is within the generation window */
    if (generation != ctx->encode_gen || ctx->encode_gen == -1) {
        /* Set new generation window */
        ctx->encode_gen = generation;
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
    int out_pkt_count = 0;
    int generation;
    unsigned char cc = ((unsigned char *)pkt)[0] & 0x0F;                    // CSRC count
    unsigned char pt = ((unsigned char *)pkt)[1] & 0x7F;                    // Payload type
    unsigned short seq = ((unsigned char *)pkt)[2] << 8 | ((char *)pkt)[3]; // Sequence number
    int rtp_hdr_len = 12 + cc * 4;                                          // Length of the RTP header
    GF_ELEMENT *coeff = NULL;                                               // pointer to repair packet's coding vector
    GF_ELEMENT *payload = NULL;                                             // pointer to repair packet's payload

    generation = seq / ctx->param->gen_size;

    /* Check if the sequence number is within the generation window */
    if (generation < ctx->decode_gen) {
        /* Drop the packet */
        *count = 0;
        return 0;
    } else if (generation > ctx->decode_gen) {
        /* Clear the decode matrix */
        for (int i = 0; i < ctx->param->gen_size; i++) {
            memset(ctx->decode_payload[i], 0,
                   (RTP_HDR_LEN + ctx->param->rtp_payload_size) * sizeof(GF_ELEMENT));
            memset(ctx->decode_coeff[i], 0, ctx->param->gen_size * sizeof(GF_ELEMENT));
        }

        memset(ctx->been_decoded, 0, ctx->param->gen_size * sizeof(int));

        /* Set new generation window */
        ctx->decode_gen = generation;
        ctx->decode_rank = 0;
    }

    if (ctx->decode_rank == ctx->param->gen_size) {
        /* Drop the packet */
        *count = 0;
        return 0;
    }

    if (pt != ctx->param->pt) { /* Is source RTP packet */
        int idx = seq % ctx->param->gen_size;

        if (ctx->been_decoded[idx]) {
            /* Drop the packet */
            *count = 0;
            return 0;
        }

        /* Copy the source RTP packet into the decode matrix */
        memset(ctx->decode_coeff[idx], 0, ctx->param->gen_size * sizeof(GF_ELEMENT));
        ctx->decode_coeff[idx][idx] = 1;

        memcpy(ctx->decode_payload[idx], pkt, len);

        /* Insert the source RTP packet into the out_pkts */
        out_pkts[out_pkt_count].buf = ctx->decode_payload[idx];
        out_pkts[out_pkt_count].len = len;

        ctx->decode_rank++;
        ctx->been_decoded[idx] = 1;
        out_pkt_count++;
    } else {
        /* Extract the coeff and payload from the repair packet */
        coeff = pkt + rtp_hdr_len;
        payload = pkt + rtp_hdr_len + ctx->param->gen_size;

        /* Insert the repair packet into the decode matrix and reduce it to upper triangular form */
        for (int i = 0; i < ctx->param->gen_size; i++) {
            GF_ELEMENT quotient;

            if (coeff[i] == 0) {
                continue;
            }

            /* The row is empty, insert the repair packet into the decode matrix */
            if (ctx->decode_coeff[i][i] == 0) {
                memcpy(ctx->decode_coeff[i], coeff, ctx->param->gen_size);
                memcpy(ctx->decode_payload[i], payload, rtp_hdr_len + ctx->param->rtp_payload_size);

                ctx->decode_rank++;
                break;
            }

            quotient = galois_div(coeff[i], ctx->decode_coeff[i][i]);

            /* Reduce the row */
            for (int j = 0; j < ctx->param->gen_size; j++) {
                coeff[j] = galois_sub(coeff[j], galois_mul(quotient, ctx->decode_coeff[i][j]));
            }
            for (int j = 0; j < rtp_hdr_len + ctx->param->rtp_payload_size; j++) {
                payload[j] = galois_sub(payload[j], galois_mul(quotient, ctx->decode_payload[i][j]));
            }
        }
    }

    if (ctx->decode_rank < ctx->param->gen_size) {
        *count = out_pkt_count;
        return 0;
    }

    /* Gaussian jordan elimination */
    for (int i = ctx->param->gen_size - 1; i >= 0; i--) {
        GF_ELEMENT pivot = ctx->decode_coeff[i][i];

        for (int j = 0; j < i; j++) {
            GF_ELEMENT quotient = galois_div(ctx->decode_coeff[j][i], pivot);

            for (int k = 0; k < ctx->param->gen_size; k++) {
                ctx->decode_coeff[j][k] = galois_sub(ctx->decode_coeff[j][k], galois_mul(quotient, ctx->decode_coeff[i][k]));
            }
            for (int k = 0; k < rtp_hdr_len + ctx->param->rtp_payload_size; k++) {
                ctx->decode_payload[j][k] = galois_sub(ctx->decode_payload[j][k], galois_mul(quotient, ctx->decode_payload[i][k]));
            }
        }

        for (int j = 0; j < ctx->param->gen_size; j++) {
            ctx->decode_coeff[i][j] = galois_div(ctx->decode_coeff[i][j], pivot);
        }
        for (int j = 0; j < rtp_hdr_len + ctx->param->rtp_payload_size; j++) {
            ctx->decode_payload[i][j] = galois_div(ctx->decode_payload[i][j], pivot);
        }
    }

    /* If there are any packet that have been decoded, insert them into the out_pkts */
    for (int i = 0; i < ctx->param->gen_size; i++) {
        if (!ctx->been_decoded[i]) {
            out_pkts[out_pkt_count].buf = ctx->decode_payload[i];
            out_pkts[out_pkt_count].len = rtp_hdr_len + ctx->param->rtp_payload_size;
            out_pkt_count++;
        }
    }

    *count = out_pkt_count;

    return 0;
}

int fec_destroy(fec *ctx) {
    /* Free the galois field */
    free_galois();

    /* Free the encode buffer */
    for (int i = 0; i < ctx->param->packet_num; i++) {
        free(ctx->encode_buf[i]);
    }
    free(ctx->encode_buf);

    /* Free the encode packets */
    for (int i = 0; i < MAX_PACKET_NUM; i++) {
        free(ctx->encode_pkts[i].buf);
    }
    free(ctx->encode_pkts);

    /* Free the decode coefficient matrix */
    for (int i = 0; i < ctx->param->gen_size; i++) {
        free(ctx->decode_coeff[i]);
    }
    free(ctx->decode_coeff);

    /* Free the decode payload */
    for (int i = 0; i < ctx->param->gen_size; i++) {
        free(ctx->decode_payload[i]);
    }
    free(ctx->decode_payload);

    /* Free the been_decoded flag */
    free(ctx->been_decoded);

    /* Free the fec context */
    free(ctx);

    return 0;
}