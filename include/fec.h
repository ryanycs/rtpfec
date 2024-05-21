#ifndef FEC_H
#define FEC_H

#define RTP_HDR_LEN 12
#define MAX_GEN_SIZE 10
#define MAX_PACKET_NUM 10

typedef struct fec_packet fec_packet;
typedef struct fec_param fec_param;
typedef struct fec fec;

typedef unsigned char GF_ELEMENT;

struct fec_packet {
    void *buf;
    int len;
};

struct fec_param {
    int gf_power;         // power of the galois field (2^gf_power)
    int gen_size;         // number of symbols in a generation
    int rtp_payload_size; // size of the RTP payload
    int n;                // fec(n, k)
    unsigned char pt;     // payload type, used to identify the source RTP & repair RTP
};

struct fec {
    fec_param *param; // fec parameters

    unsigned short encode_seq; // sequence number of the encode matrix
    int encode_count;          // number of packets been encoded
    GF_ELEMENT **encode_buf;   // buffer for storing the RTP source packets
    fec_packet *encode_pkts;   // encoded packets buffer

    unsigned short decode_seq;   // sequence number of the decode matrix
    int rank;                    // rank of the decode matrix
    GF_ELEMENT **decode_coeff;   // decode coefficient matrix
    GF_ELEMENT **decode_payload; // decode payload matrix
};

/*
 * Initialize the fec context
 *
 * @param ctx      The fec context
 * @param param    The fec parameters
 *
 * @return         0 if success, -1 if failed
 */
int fec_init(fec **ctx, fec_param *param);

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
int fec_encode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count);

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
int fec_decode(fec *ctx, void *pkt, int len, fec_packet out_pkts[], int *count);

/*
 * Free the fec context
 *
 * @param ctx
 *
 * @return          0 if success, -1 if failed
 */
int fec_free(fec *ctx);

#endif /* FEC_H */
