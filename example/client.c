#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RTP_PAYLOAD_SIZE 16

int main(int argc, char **argv) {
    fec *ctx;
    fec_param param = {.gf_power = 8,
                       .gen_size = 4,
                       .rtp_payload_size = RTP_PAYLOAD_SIZE,
                       .packet_num = 8,
                       .pt = 97};
    fec_packet out_pkts[MAX_PACKET_NUM];
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[1024] = {0};

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <serverIP> <serverPort> <lossRate>\n",
                argv[0]);
        return 1;
    }

    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }

    server_addr.sin_family = PF_INET,
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    sendto(sockfd, "Start!", 6, 0, (struct sockaddr *)&server_addr,
           sizeof(server_addr));

    while (1) {
        int count = 0;
        int len = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
        if (len < 0) {
            fprintf(stderr, "Failed to receive data\n");
            break;
        }
        if (strcmp(buf, "End!") == 0) {
            break;
        }

        if (rand() % 100 < atoi(argv[3])) {
            printf("Packet loss\n\n");
            continue;
        } else {
            printf("Received\n\n");
        }

        fec_decode(ctx, buf, len, out_pkts, &count);

        for (int i = 0; i < count; i++) {
            printf("Decoded:\n");
            for (int j = 0; j < out_pkts[i].len; j++) {
                printf("%02X ", ((unsigned char *)out_pkts[i].buf)[j]);
            }
            printf("\n\n");
        }
    }

    close(sockfd);
    fec_destroy(ctx);
}