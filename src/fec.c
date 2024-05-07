#include <stdlib.h>
#include <string.h>

#include "../include/fec.h"
#include "galois.h"

int fec_encode(fec *ctx, void *pkt, int len, packet out_pkts[], int *count) {
  int out_pkt_count = 0;
  int rtp_hdr_len = 12;
  unsigned short seq; // Sequence number
  GF_ELEMENT *coeffs;
  GF_ELEMENT *payload;
  void *rtp_hdr;

  /* Check if the packet is within the generation window */
  if (seq >= ctx->seq + ctx->param->gen_size) {
    /* Set new generation window */
    ctx->seq = seq;
    ctx->rank = 0;
  }

  /* Insert the packet into the encode matrix */
  coeffs = ctx->coeff_mat[ctx->rank];
  coeffs[ctx->rank] = 1;

  payload = ctx->payload_mat[ctx->rank];
  memcpy(payload, pkt, len);

  ctx->rank++;

  /* Insert the packet into the out_pkts */
  out_pkts[out_pkt_count].size = len;
  out_pkts[out_pkt_count++].buf = pkt;

  /* If the rank is not equal to the generation size, we need to wait for more
   * packets */
  if (ctx->rank != ctx->param->gen_size) {
    return 0;
  }

  /* Create k - gen_size repair packets */
  for (int i = 0; i < ctx->param->k; i++) {
    for (int j = 0; j < ctx->param->gen_size; j++) {
      int coeff;
      coeff = rand() % (1 << ctx->param->gf_power);

      /* Set the coefficient in the encode matrix */

      for (int k = 0; k < ctx->param->symbol_size; k++) {
        /* payload[j] += coeff * buf[i][j] */
        if (coeff) {
        }
      }
    }

    /* Insert the repair packet into the out_pkts */
    out_pkts[out_pkt_count].size =
        ctx->param->symbol_size + ctx->param->gen_size + rtp_hdr_len;
    out_pkts[out_pkt_count++].buf = payload;
  }

  return 0;
}

int fec_decode(fec *ctx, void *pkt, int len, packet out_pkts[], int *count) {
  unsigned char cc;   // CSRC count
  unsigned char pt;   // Payload type
  unsigned short seq; // Sequence number
  void *payload;

  /* Get sequence number of the incoming RTP packet */
  seq = ((char *)pkt)[2] << 8 | ((char *)pkt)[3];

  /* Check if the sequence number is within the generation window */
  if (seq < ctx->seq) {
    /* Drop the packet */
    count = 0;
    return 0;
  } else if (ctx->seq + ctx->param->gen_size <= seq) {
    /* Clear the decode matrix */
    for (int i = 0; i < ctx->param->gen_size; i++) {
      memset(ctx->payload_mat[i], 0,
             ctx->param->symbol_size * sizeof(GF_ELEMENT));
      memset(ctx->coeff_mat[i], 0, ctx->param->gen_size * sizeof(GF_ELEMENT));
    }
    /* Set new generation window */
    ctx->seq = seq;
  }

  pt = ((char *)pkt)[1] & 0x7F;
  /* If the packet is a repair packet, we need to extract the repair payload */
  if (pt == ctx->param->pt) {
    cc = ((char *)pkt)[0] & 0x0F; // CSRC count

    int rtp_hdr_len = 12 + cc * 4;
  }

  /* upper triangular matrix */

  /* If there are any packet that have been decoded, insert them into the
   * out_pkts */

  return 0;
}
