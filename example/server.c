#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define RTP_HEADER_SIZE 12
#define RTP_PAYLOAD_SIZE 16

struct RTP_header {
    unsigned char v_p_x_cc;
    unsigned char m_pt;
    unsigned short seq;
    unsigned int timestamp;
    unsigned int ssrc;
};

void rtp_encode(char *buf) {
    static unsigned short seq = 0;
    static unsigned int timestamp = 0;
    struct RTP_header *header = (struct RTP_header *)buf;

    header->v_p_x_cc = 0x80;
    header->m_pt = 8; // PCMU
    header->seq = htons(seq);
    header->timestamp = htonl(timestamp += 160);
    header->ssrc = htonl(0);

    for (int i = RTP_HEADER_SIZE; i < RTP_HEADER_SIZE + RTP_PAYLOAD_SIZE; i++) {
        buf[i] = seq * RTP_PAYLOAD_SIZE + i - RTP_HEADER_SIZE;
    }

    seq++;
}

int main(int argc, char **argv) {
    fec *ctx;
    fec_param param = {.gf_power = 8,
                       .gen_size = 4,
                       .rtp_payload_size = RTP_PAYLOAD_SIZE,
                       .packet_num = 8,
                       .pt = 97};
    fec_packet out_pkts[MAX_PACKET_NUM];
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buf[1024] = {0};
    char rtp_buf[1024] = {0};

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <serverIP> <serverPort> <num>\n", argv[0]);
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

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        fprintf(stderr, "Failed to bind socket\n");
        close(sockfd);
        return 1;
    }
    printf("Server started at %s:%d\n", argv[1], atoi(argv[2]));

    while (1) {
        int count = 0;
        socklen_t client_addr_len = sizeof(client_addr);
        int len = recvfrom(sockfd, buf, sizeof(buf), 0,
                           (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            fprintf(stderr, "Failed to receive data\n");
            break;
        }

        printf("Client %s:%d connected\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        printf("Data: %s\n", buf);

        for (int i = 0; i < atoi(argv[3]); i++) {
            rtp_encode(rtp_buf);
            fec_encode(ctx, rtp_buf, RTP_HEADER_SIZE + RTP_PAYLOAD_SIZE,
                       out_pkts, &count);
            for (int j = 0; j < count; j++) {
                printf("Send packet:\n");
                for (int k = 0; k < out_pkts[j].len; k++) {
                    printf("%02X ", ((unsigned char *)out_pkts[j].buf)[k]);
                }
                printf("\n\n");
                sendto(sockfd, out_pkts[j].buf, out_pkts[j].len, 0,
                       (struct sockaddr *)&client_addr, client_addr_len);

                sleep(1);
            }
        }

        sendto(sockfd, "End!", 5, 0, (struct sockaddr *)&client_addr,
               client_addr_len);
    }

    close(sockfd);
    fec_destroy(ctx);

    return 0;
}