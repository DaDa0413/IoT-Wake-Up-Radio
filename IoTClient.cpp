//
// Created by ray on 2020-05-21.
//
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <iostream>

#define PORT 7001
#define LENGTH 3000
#define IP_ADDRESS "192.168.10.2"

using namespace std;

int sockfd;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void connect_w_to(void)
{
    int res;
    struct sockaddr_in addr;
    long arg;
    fd_set myset;
    struct timeval tv;
    int valopt;
    socklen_t lon;

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno));
        fprintf(stdout, "Error creating socket (%d %s)\n", errno, strerror(errno));
        exit(0);
    }

    /* Fill the socket address struct */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    /* Set non-blocking */
    if ((arg = fcntl(sockfd, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        fprintf(stdout, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    arg |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, arg) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        fprintf(stdout, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    /* Trying to connect with timeout */
    res = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0)
    {
        if (errno == EINPROGRESS)
        {
            fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
            do
            {
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                FD_ZERO(&myset);
                FD_SET(sockfd, &myset);
                res = select(sockfd + 1, NULL, &myset, NULL, &tv);
                if (res < 0 && errno != EINTR)
                {
                    fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                    fprintf(stdout, "Error connecting %d - %s\n", errno, strerror(errno));
                    exit(0);
                }
                else if (res > 0)
                {
                    /* Socket selected for write */
                    lon = sizeof(int);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &lon) < 0)
                    {
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        fprintf(stdout, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        exit(0);
                    }
                    /* Check the value returned... */
                    if (valopt)
                    {
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        fprintf(stdout, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        exit(0);
                    }
                    break;
                }
                else
                {
                    fprintf(stderr, "Timeout in select() - Cancelling!\n");
                    fprintf(stdout, "Timeout in select() - Cancelling!\n");
                    exit(0);
                }
            } while (1);
        }
        else
        {
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
            fprintf(stdout, "Error connecting %d - %s\n", errno, strerror(errno));
            exit(0);
        }
    }
    /* Set to blocking mode again... */
    if ((arg = fcntl(sockfd, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        fprintf(stdout, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }
    arg &= (~O_NONBLOCK);
    if (fcntl(sockfd, F_SETFL, arg) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        fprintf(stdout, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: IoTClient [filename]\n");
        exit(1);
    }
    /* Try to connect the remote */
    connect_w_to();

    /* Send File to Server */
    //if(!fork())
    //{
    char fs_name[15];

    char sdbuf[LENGTH];
    printf("[Client] Sending %s to the Server... ", argv[1]);
    FILE *fs = fopen(argv[1], "r");
    if (fs == NULL)
    {
        printf("ERROR: File %s not found.\n", argv[1]);
        exit(1);
    }

    bzero(sdbuf, LENGTH);
    int fs_block_sz;
    while ((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
    {
        if (send(sockfd, sdbuf, fs_block_sz, 0) < 0)
        {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", argv[1], errno);
            break;
        }
        bzero(sdbuf, LENGTH);
    }
    printf("Ok File %s from Client was Sent!\n", argv[1]);
    //}

    close(sockfd);
    printf("[Client] Connection lost.\n");
    return (0);
}
