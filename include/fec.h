#ifndef FEC_H
#define FEC_H

typedef struct packet packet;
typedef struct fec_param fec_param;
typedef struct fec fec;

typedef unsigned char GF_ELEMENT;

struct packet {
  void *buf;
  int size;
};

struct fec_param {
  int gf_power;      // power of the galois field (2^gf_power)
  int gen_size;      // number of symbols in a generation
  int symbol_size;   // size of a symbol in bytes
  int k;             // fec(n, k)
  unsigned short pt; // payload type
};

struct fec {
  struct fec_param *param;
  unsigned short seq;
  int rank; // rank of the matrix

  GF_ELEMENT **coeff_mat;   // coefficient matrix
  GF_ELEMENT **payload_mat; // payload matrix
};

int fec_init(fec *ctx, fec_param *param);

/*
 * Encode the RTP packet into FEC packets
 *
 * @param ctx
 * @param pkt       The RTP packet
 * @param len       The length of the RTP packet
 * @param out_pkts  The output FEC packets (Encapsulated in a new RTP packet)
 * @param count     The number of FEC packets been generated
 *
 * @return          0 if success, -1 if failed
 */
int fec_encode(fec *ctx, void *pkt, int len, packet out_pkts[], int *count);

/*
 * Decode the FEC packet into RTP packets
 *
 * @param ctx
 * @param pkt       The incomming RTP packet
 * @param len       The length of the RTP packet
 * @param out_pkts  RTP packets that been decoded
 * @param count     The number of RTP packets that been decoded
 *
 * @return          0 if success, -1 if failed
 */
int fec_decode(fec *ctx, void *pkt, int len, packet out_pkts[], int *count);

#endif /* FEC_H */
