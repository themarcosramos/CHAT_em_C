#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define TOTAL_PARAMETERS 3
#define BUFFSIZE 1024

enum Parameters {
    FileParameter = 1,
    DestinationIPParameter,
    DestinationPortParameter,
};

typedef struct _ConnectionParameters {
    char* File;
    char* DestinationIp;
    int DestinationPort;
} ConnectionParameters;

int configure_parameters(ConnectionParameters*, char* argv[]);
void show_parameters(ConnectionParameters*);
char *format_bytes(size_t bytes);

int main(int argc, char *argv[])
{
    printf("Client TCP\n==========\n");

    if (argc != TOTAL_PARAMETERS + 1)
    {
        fprintf(stderr, "Invalid parameters!\n[FILE PATH] [DESTINATION IP] [DESTINATION PORT]\n");
        exit(EXIT_FAILURE);
    }

    ConnectionParameters parameters;
    if (configure_parameters(&parameters, argv) < 0)
    {
        exit(EXIT_FAILURE);
    }

    show_parameters(&parameters);

    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    // specify the server address and port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(parameters.DestinationPort);
    int result = inet_aton(parameters.DestinationIp, &server_addr.sin_addr);
    if (result == 0) {
        fprintf(stderr, "inet_aton failed\n");
        close(sockfd);
        return 1;
    }

    // connect to the server
    result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0) {
        perror("connect failed");
        close(sockfd);
        return 1;
    }

    // open the file for reading
    FILE *fp = fopen(parameters.File, "r");
    if (fp == NULL) {
        perror("fopen failed");
        close(sockfd);
        return 1;
    }

    // send the file name
    result = send(sockfd, parameters.File, strlen(parameters.File), MSG_DONTWAIT);
    if (result < 0) {
        perror("send failed");
        fclose(fp);
        close(sockfd);
        return 1;
    }
    usleep(1000);

    // send the file to the server
    char buffer[BUFFSIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFSIZE, fp)) > 0) {
        result = send(sockfd, buffer, bytes_read, 0);
        if (result < 0) {
            perror("send failed");
            fclose(fp);
            close(sockfd);
            return 1;
        }
    }

    return 0;
}

int configure_parameters(ConnectionParameters* parameters, char* argv[])
{
    parameters->File = argv[FileParameter];
    parameters->DestinationIp = argv[DestinationIPParameter];
    parameters->DestinationPort = atoi(argv[DestinationPortParameter]);

    if (strlen(parameters->File) < 1)
    {
        fprintf(stderr, "File must be at least 1 character!\n");
        exit(EXIT_FAILURE);
    }

    if (parameters->DestinationPort <= 0)
    {
        fprintf(stderr, "Invalid destination port!\n");
        return -1;
    }

    return 0;
}

void show_parameters(ConnectionParameters* parameters)
{
    printf("Server connection parameters:\n");
    printf("File: %s\n", parameters->File);
    printf("Destination IP: %s\n", parameters->DestinationIp);
    printf("Destination port: %d\n", parameters->DestinationPort);
}
