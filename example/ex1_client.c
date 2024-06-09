#include <pthread.h>
#include <arpa/inet.h>
#include <fec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

fec *ctx;
fec_param param = {
    .gf_power = 8,
    .gen_size = 8,
    .rtp_payload_size = 16,
    .packet_num = 8,
    .pt = 97};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int packet_received = 0;
int packet_lost = 0;
int rtp_seq = 0;
int rtp_decoded = 0;
int flag = 0;
int loss_rate = 0;

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void *recv_rtp(void *arg) {
    char **argv = (char **)arg;
    fec_packet out_pkts[MAX_PACKET_NUM];
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[1024] = {0};
    loss_rate = atoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        pthread_exit(NULL);
    }

    server_addr.sin_family = PF_INET,
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    sendto(sockfd, "Start!", 6, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

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

        pthread_mutex_lock(&mutex);
        rtp_seq = ((unsigned char *)buf)[2] << 8 | ((unsigned char *)buf)[3];
        // printf("Received rtp_seq = %d rtp_decoded = %d\n", rtp_seq, rtp_decoded);
        pthread_mutex_unlock(&mutex);

        // printf("recievd = %d, lost = %d\n", packet_received, packet_lost);
        if (rand() % 100 < loss_rate) {
            // printf("Packet loss\n\n");
            pthread_mutex_lock(&mutex);
            packet_lost++;
            pthread_mutex_unlock(&mutex);
            continue;
        } else {
            // printf("Received\n\n");
            pthread_mutex_lock(&mutex);
            packet_received++;
            pthread_mutex_unlock(&mutex);
        }

        // char *buf2 = malloc(1024);
        // memcpy(buf2, buf, len);
        fec_decode(ctx, buf, len, out_pkts, &count);

        pthread_mutex_lock(&mutex);
        rtp_decoded += count;
        pthread_mutex_unlock(&mutex);

        // for (int i = 0; i < count; i++) {
        //     printf("Decoded:\n");
        //     for (int j = 0; j < out_pkts[i].len; j++) {
        //         printf("%02X ", ((unsigned char *)out_pkts[i].buf)[j]);
        //     }
        //     printf("\n\n");
        // }
        // printf("rtp_seq = %d, rtp_decoded = %d\n", rtp_seq, rtp_decoded);
    }

    close(sockfd);
    pthread_exit(NULL);
}

void *send_rtcp(void *arg) {
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

    while (1) {
        pthread_mutex_lock(&mutex);
        ((int*)buf)[0] = packet_received;
        ((int*)buf)[1] = packet_lost;
        packet_received = 0;
        packet_lost = 0;
        pthread_mutex_unlock(&mutex);
        sendto(sockfd, buf, 2 * sizeof(int), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        sleep(3);
    }
}

void *analyze(void *arg) {
    static int last_rtp_seq = 0;
    while (1) {
        sleep(20);
        pthread_mutex_lock(&mutex);
        printf("rtp_decoded = %3d, n = %3d, recovery_rate = %.2f%%\n", rtp_decoded, (rtp_seq - last_rtp_seq + 1), (float)rtp_decoded / (rtp_seq - last_rtp_seq + 1) * 100);
        last_rtp_seq = rtp_seq;
        rtp_decoded = 0;
        loss_rate += 5;
        printf("loss rate increased to %d\n", loss_rate);
        pthread_mutex_unlock(&mutex);
        flag++;
        if (flag % 4 == 0) {
            //increase the loss rate
            loss_rate += 5;
            printf("loss rate increased to %d\n", loss_rate);
        }
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
    pthread_t tid[3];

    pthread_mutex_init(&mutex, NULL);

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <serverIP> <serverPort> <lossRate>\n", argv[0]);
        return 1;
    }

    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    char *arg[] = {argv[0], argv[1], argv[2], argv[3]};
    pthread_create(&tid[0], NULL, recv_rtp, (void *)arg);
    pthread_create(&tid[1], NULL, send_rtcp, (void *)arg);
    pthread_create(&tid[2], NULL, analyze, NULL);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

    fec_destroy(ctx);
}