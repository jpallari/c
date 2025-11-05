#define _POSIX_C_SOURCE 200112L

#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(void) {
    int status = 0;
    int sockfd = 0;
    int clientfd = 0;
    int yes = 1;
    struct addrinfo hints = {0};
    struct addrinfo *servinfo = 0;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((status = getaddrinfo(NULL, "6060", &hints, &servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        goto end;
    }

    sockfd = socket(
        servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol
    );
    if (sockfd < 0) {
        perror("socket");
        goto end;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
        perror("setsockopt");
        goto end;
    }
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("bind");
        goto end;
    }
    if (listen(sockfd, 100) < 0) {
        perror("listen");
        goto end;
    }

    struct sockaddr_storage client_addr = {0};
    socklen_t client_addr_size = sizeof(client_addr);
    clientfd =
        accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (clientfd < 0) {
        perror("accept");
        goto end;
    }

    char buffer[1024] = {0};
    ssize_t received_len = recv(clientfd, buffer, sizeof(buffer), 0);
    if (received_len <= 0) {
        perror("recv");
        goto end;
    }
    printf("got: %s\n", buffer);

end:
    if (clientfd > 0) {
        close(clientfd);
        perror("close client");
    }
    if (sockfd > 0) {
        close(sockfd);
        perror("close server");
    }
    if (servinfo) {
        freeaddrinfo(servinfo);
    }
    return 0;
}
