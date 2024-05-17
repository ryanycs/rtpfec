#include "util.h"
#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>

#define N 4

void print_packet(fec_packet *pkt) {
    for (int i = 0; i < pkt->len; i++) {
        printf("%02X ", ((unsigned char *)pkt->buf)[i]);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    fec_param param = {
        .gf_power = 3,
        .gen_size = 4,
        .rtp_payload_size = 8,
        .n = 8,
        .pt = 97,
    };
    fec *ctx;

    // if (argc != 2) {
    //     fprintf(stderr, "Usage: %s <m>\n", argv[0]);
    //     return 1;
    // }

    printf("Initialize fec context\n");
    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    int seq = 1;
    char *pkt[N];

    for (int i = 0; i < N; i++) {
        RTP_header *header;

        pkt[i] = malloc(20);

        header = (RTP_header *)pkt[i];
        header->v_p_x_cc = (unsigned char)0x80;
        header->m_pt = 0xE0;
        header->seq = htons(seq++);
        header->timestamp = htonl(i * 160);
        header->ssrc = htonl(1);

        for (int j = RTP_HDR_LEN; j < 20; j++) {
            pkt[i][j] = i * 8 + j - RTP_HDR_LEN;
        }

        printf("RTP packet %d:\n", i);
        for (int j = 0; j < 20; j++) {
            printf("%02X ", (unsigned char)pkt[i][j]);
        }
        printf("\n");
    }

    fec_packet out_pkts[MAX_PACKET_NUM];
    int count = 0;
    for (int i = 0; i < N; i++) {
        fec_encode(ctx, pkt[i], 20, out_pkts, &count);
        printf("count = %d\n", count);
        printf("Generated FEC packets:\n");
        for (int i = 0; i < count; i++) {
            print_packet(&out_pkts[i]);
        }
    }
}