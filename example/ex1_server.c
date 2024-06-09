#include <pthread.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define RTP_HEADER_SIZE 12
#define RTP_PAYLOAD_SIZE 16

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
fec *ctx;
fec_param param = {
    .gf_power = 8,
    .gen_size = 8,
    .rtp_payload_size = RTP_PAYLOAD_SIZE,
    .packet_num = 8,
    .pt = 97};

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

void *send_rtp(void *arg) {
    char **argv = (char **)arg;

    fec_packet out_pkts[MAX_PACKET_NUM];
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buf[1024] = {0};
    char rtp_buf[1024] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        pthread_exit(NULL);
    }

    server_addr.sin_family = PF_INET,
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to bind socket\n");
        close(sockfd);
        pthread_exit(NULL);
    }
    printf("Server started at %s:%d\n", argv[1], atoi(argv[2]));

    while (1) {
        int count = 0;
        socklen_t client_addr_len = sizeof(client_addr);
        int len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            fprintf(stderr, "Failed to receive data\n");
            break;
        }

        printf("Client %s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("Data: %s\n", buf);

        for (int i = 0; i < atoi(argv[3]); i++) {
            rtp_encode(rtp_buf);
            fec_encode(ctx, rtp_buf, RTP_HEADER_SIZE + RTP_PAYLOAD_SIZE, out_pkts, &count);
            for (int j = 0; j < count; j++) {
                // printf("Send packet:\n");
                // for (int k = 0; k < out_pkts[j].len; k++) {
                //     printf("%02X ", ((unsigned char *)out_pkts[j].buf)[k]);
                // }
                // printf("\n\n");
                sendto(sockfd, out_pkts[j].buf, out_pkts[j].len, 0, (struct sockaddr *)&client_addr, client_addr_len);

                // sleep 20ms
                usleep(20000);
            }
        }

        sendto(sockfd, "End!", 5, 0, (struct sockaddr *)&client_addr, client_addr_len);
    }

    close(sockfd);
    pthread_exit(NULL);
}

void *recv_rtcp(void *arg) {
    char **argv = (char **)arg;
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[1024] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        pthread_exit(NULL);
    }

    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]) + 1);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Failed to bind socket\n");
        close(sockfd);
        pthread_exit(NULL);
    }
    printf("RTCP server started at %s:%d\n", argv[1], atoi(argv[2]) + 1);

    while (1) {
        int len = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);
        if (len < 0) {
            fprintf(stderr, "Failed to receive data\n");
            break;
        }
        int recv = ((int*)buf)[0];
        int lost = ((int*)buf)[1];
        if (recv + lost == 0) {
            continue;
        }
        float loss_rate = (float)lost / (recv + lost);
        // printf("recievd = %d, lost = %d\n", recv, lost);
        ctx->param->packet_num = ctx->param->gen_size / (1 - loss_rate) + 3;
        printf("packet_num = %d\n", ctx->param->packet_num);
        printf("loss rate = %f\n", loss_rate);
    }

    close(sockfd);
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    pthread_t tid[2];

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <serverIP> <serverPort> <num>\n", argv[0]);
        return 1;
    }

    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    char *arg[] = {argv[0], argv[1], argv[2], argv[3]};

    pthread_create(&tid[0], NULL, send_rtp,(void *) arg);
    pthread_create(&tid[1], NULL, recv_rtcp, (void *) arg);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    return 0;
}