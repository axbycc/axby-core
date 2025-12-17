// C implementation of the time sync server for deployment on
// Raspberry PI.  Bazel does not support Raspberry Pi so the repo does
// not build without more effort. Setting up cross compile requires
// Bazel expertise that I don't have. The easiest thing to do is just
// rewrite the server in pure C so that we can build directly on
// Raspberry Pi.


#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void error(char* msg) {
    perror(msg);
    exit(1);
}

// https://stackoverflow.com/questions/5833094/get-a-timestamp-in-c-in-microseconds
#define NS_TO_US(ns) ((ns) / 1000)
#define SEC_TO_US(sec) ((sec)*1000000)

uint64_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t us =
        SEC_TO_US((uint64_t)ts.tv_sec) + NS_TO_US((uint64_t)ts.tv_nsec);
    return us;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    printf("using ip %s\n", argv[1]);

    const uint16_t port = strtoul(argv[2], 0, 10);
    printf("using port %u\n", port);

    struct sockaddr_in server_addr;

    const char* server_ip = argv[1];
    int optval;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        error("could not create udp socket");
    }

    // prevent address already in use
    optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
               sizeof(int));

    bzero((char*)&server_addr, sizeof(server_addr));
    if (!inet_aton(server_ip, &server_addr.sin_addr)) {
        printf("invalid ip %s\n", server_ip);
        exit(-1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        error("could not bind");
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

		const uint64_t micros_start = micros();
    while (1) {
        uint64_t client_message;
        ssize_t recvd_size =
            recvfrom(sock, &client_message, sizeof(client_message), 0,
                     (struct sockaddr*)&client_addr, &client_addr_size);
        if (recvd_size < 0) {
            error("recvfrom error");
        }

        uint64_t response[2];
        response[0] = client_message;
        response[1] = micros() - micros_start;

        ssize_t sent_size =
            sendto(sock, response, 2 * sizeof(uint64_t), 0,
                   (struct sockaddr*)&client_addr, client_addr_size);
        if (sent_size < 0) {
            error("sendto error");
        }
    }
}
