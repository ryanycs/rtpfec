#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>

#define RTP_HDR_LEN 12

typedef struct RTP_header RTP_header;

struct RTP_header {
    unsigned char v_p_x_cc;
    unsigned char m_pt;
    unsigned short seq;
    unsigned int timestamp;
    unsigned int ssrc;
};

void print_packet(fec_packet *pkt) {
    for (int i = 0; i < pkt->len; i++) {
        printf("%02X ", ((unsigned char *)pkt->buf)[i]);
    }
    printf("\n");
}

void allocate_rtp_packet(char ***pkts, int num, int packet_size) {
    (*pkts) = malloc(num * sizeof(char *));
    for (int i = 0; i < num; i++) {
        (*pkts)[i] = malloc(packet_size);
    }
}

void generate_rtp_packet(char **pkts, int num, int packet_size) {
    static int seq = 0;

    for (int i = 0; i < num; i++) {
        RTP_header *header;

        header = (RTP_header *)pkts[i];
        header->v_p_x_cc = (unsigned char)0x80;
        header->m_pt = 0xE0;
        header->seq = htons(seq++);
        header->timestamp = htonl(i * 160);
        header->ssrc = htonl(1);

        for (int j = RTP_HDR_LEN; j < packet_size; j++) {
            pkts[i][j] = i * (packet_size - RTP_HDR_LEN) + j - RTP_HDR_LEN;
        }
    }
}