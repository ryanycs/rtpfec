#include "utils.h"
#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 10
#define RTP_PKT_SIZE 20

int main(int argc, char **argv) {
    int out_packet_count = 0;
    int count = 0;
    char **pkts = NULL;
    fec_param param;
    fec *ctx;
    fec_packet out_pkts[MAX_PACKET_NUM];
    fec_packet *encoded_pkts;

    param.gf_power = 8;
    param.gen_size = 4;
    param.rtp_payload_size = 8;
    param.packet_num = 8;
    param.pt = 97;

    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    encoded_pkts = malloc(MAX_PACKET_NUM * MAX_GEN_SIZE * sizeof(fec_packet));
    for (int i = 0; i < 50; i++) {
        encoded_pkts[i].buf = malloc(RTP_PKT_SIZE * 5);
    }

    allocate_rtp_packet(&pkts, N, RTP_PKT_SIZE);
    generate_rtp_packet(pkts, N, RTP_PKT_SIZE);

    for (int i = 0; i < N; i++) {
        fec_encode(ctx, pkts[i], RTP_PKT_SIZE, out_pkts, &count);

        /* Copy the out_pkts to the encoded_pkts buffer */
        for (int j = 0; j < count; j++) {
            memcpy(encoded_pkts[out_packet_count].buf, out_pkts[j].buf, out_pkts[j].len);
            encoded_pkts[out_packet_count].len = out_pkts[j].len;
            out_packet_count++;
        }
    }

    for (int i = 0; i < out_packet_count; i++) {
        printf("Encoded packet %d:\n", i);
        print_packet(&encoded_pkts[i]);
    }
    printf("\n");

    fec_destroy(ctx);

    return 0;
}