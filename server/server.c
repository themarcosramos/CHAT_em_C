#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0
#define BUFFSIZE 1024
#define TOTAL_PARAMETERS 3
#define MAX_CLIENTS 30
#define MAX_PENDING_CONNECTIONS 3

enum Parameters {
    FolderParameter = 1,
    ListeningIPParameter,
    ListeningPortParameter,
};

typedef enum _ConnectionStatus {
    Disconnected = 0,
    Connected = 1,
    FileNameReceived,
    ReceivingFile,
    Error,
    ReceivedFile,
    Completed,
} ConnectionStatus;

typedef int SocketHandle;

typedef struct _ConnectionParameters {
    char* Folder;
    char* ListeningIp;
    int ListeningPort;
} ConnectionParameters;

typedef struct _ClientParameters {
    char* File;
    int FileSize;
    SocketHandle Socket;
    ConnectionStatus Status;
} ClientParameters;

const static char* welcome_message = "TCP Server v1.0 \r\n";

void create_folder(const char* folder);
int configure_parameters(ConnectionParameters*, char* argv[]);
void show_parameters(ConnectionParameters*);
SocketHandle prepare_sockets(ConnectionParameters*, struct sockaddr_in*);
int start_listening(ConnectionParameters*, SocketHandle, struct sockaddr_in*);
void initialize_client_sockets(ClientParameters client_socket[]);
void bind_socket_listener(ConnectionParameters*, int, struct sockaddr_in*);
void append_data_file(const char* filename, const char* data);
char *format_bytes(size_t bytes);

int main(int argc, char* argv[])
{
    printf("Server TCP\n==========\n");

    if (argc != TOTAL_PARAMETERS + 1)
    {
        fprintf(stderr, "Invalid parameters!\n[FOLDER] [LISTENING IP] [LISTENING PORT]\n");
        exit(EXIT_FAILURE);
    }

    ConnectionParameters parameters;
    if (configure_parameters(&parameters, argv) < 0)
    {
        exit(EXIT_FAILURE);
    }

    show_parameters(&parameters);

    create_folder(parameters.Folder);

    struct sockaddr_in address;
    SocketHandle master_socket = prepare_sockets(&parameters, &address);

    return start_listening(&parameters, master_socket, &address);
}

int configure_parameters(ConnectionParameters* parameters, char* argv[])
{
    parameters->Folder = argv[FolderParameter];
    parameters->ListeningIp = argv[ListeningIPParameter];
    parameters->ListeningPort = atoi(argv[ListeningPortParameter]);

    if (strlen(parameters->Folder) < 1)
    {
        fprintf(stderr, "Folder must be at least 1 character!\n");
        exit(EXIT_FAILURE);
    }

    if (parameters->ListeningPort <= 0)
    {
        fprintf(stderr, "Invalid listening port!\n");
        return -1;
    }

    return 0;
}

void show_parameters(ConnectionParameters* parameters)
{
    printf("Server connection parameters:\n");
    printf("Folder: %s\n", parameters->Folder);
    printf("Listening IP: %s\n", parameters->ListeningIp);
    printf("Listening port: %d\n", parameters->ListeningPort);
}

void create_folder(const char* folder)
{
    struct stat st;
    int result = stat(folder, &st);
    if (result == 0 && S_ISDIR(st.st_mode)) {
        return;
    }

    result = mkdir(folder, 0777);
    if (result != 0) {
        perror("mkdir failed");
        exit(EXIT_FAILURE);
    }
}

SocketHandle prepare_sockets(ConnectionParameters* parameters, struct sockaddr_in* address)
{
    int opt = TRUE;
    SocketHandle master_socket;

    if ((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    bind_socket_listener(parameters, master_socket, address);

    if (listen(master_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    puts("Waiting for connections...");

    return master_socket;
}

int start_listening(ConnectionParameters* parameters, SocketHandle master_socket, struct sockaddr_in* address)
{
    fd_set readfds;
    int socket_descriptor;
    int max_sd;
    int i;
    int activity;
    int valread;
    SocketHandle new_socket;
    ClientParameters client_parameter[MAX_CLIENTS];
    char buffer[BUFFSIZE + 1];

    int addrlen = sizeof(struct sockaddr_in);

    initialize_client_sockets(client_parameter);

    while (TRUE)
    {
        FD_ZERO(&readfds);

        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            socket_descriptor = client_parameter[i].Socket;

            if (socket_descriptor > 0)
                FD_SET(socket_descriptor , &readfds);

            if (socket_descriptor > max_sd)
                max_sd = socket_descriptor;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }


        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket, (struct sockaddr*)address, (socklen_t*)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n",
                new_socket, inet_ntoa(address->sin_addr), ntohs(address->sin_port));

            //add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                //if position is empty
                if (client_parameter[i].Socket == 0)
                {
                    client_parameter[i].Socket = new_socket;
                    client_parameter[i].Status = Connected;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            socket_descriptor = client_parameter[i].Socket;

            if (FD_ISSET(socket_descriptor, &readfds))
            {
                if ((valread = read(socket_descriptor, buffer, BUFFSIZE)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(socket_descriptor , (struct sockaddr*)address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n",
                          inet_ntoa(address->sin_addr) , ntohs(address->sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(socket_descriptor);

                    char* file_size = format_bytes(client_parameter[i].FileSize);
                    fprintf(stdout, "File received: %s of size %s\n", client_parameter[i].File, file_size);
                    free(file_size);

                    client_parameter[i].Socket = 0;
                    client_parameter[i].Status = Disconnected;
                    free(client_parameter[i].File);
                    client_parameter[i].FileSize = 0;
                }
                else
                {
                    switch (client_parameter[i].Status)
                    {
                    case Connected:
                        buffer[valread] = '\0';
                        client_parameter[i].File = malloc(strlen(parameters->Folder) + strlen(buffer) + 1);
                        strcpy(client_parameter[i].File, parameters->Folder);
                        strcat(client_parameter[i].File, (const char*)buffer);
                        send(socket_descriptor, buffer, strlen(buffer), 0);
                        client_parameter[i].Status = FileNameReceived;
                        memset(buffer, 0, sizeof(buffer));
                        break;

                    case FileNameReceived:
                        client_parameter[i].FileSize += strlen(buffer);
                        append_data_file(client_parameter[i].File, buffer);
                        memset(buffer, 0, sizeof(buffer));
                        break;

                    default:
                        break;
                    }
                }
            }
        }
    }

    return 0;
}

void initialize_client_sockets(ClientParameters client_socket[])
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_socket[i].Socket = 0;
        client_socket[i].Status = Disconnected;
        client_socket[i].FileSize = 0;
    }
}

void bind_socket_listener(ConnectionParameters* parameters, int master_socket, struct sockaddr_in* address)
{
    static struct in_addr ip_address;

    inet_aton(parameters->ListeningIp, &ip_address);

    address->sin_family = AF_INET;
    address->sin_addr = ip_address;
    address->sin_port = htons(parameters->ListeningPort);

    if (bind(master_socket, (struct sockaddr*)address, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Listener on port %d \n", parameters->ListeningPort);
}

void append_data_file(const char* filename, const char* data)
{
    FILE* file;
    file = fopen(filename, "a+");

    fwrite(data, 1, strlen(data), file);

    fclose(file);
}

char *format_bytes(size_t bytes)
{
    if (bytes < 1024)
    {
        // less than 1 KB, just return the number of bytes
        char *result = malloc(32);
        snprintf(result, 32, "%zu bytes", bytes);
        return result;
    }
    else if (bytes < 1048576)
    {
        // less than 1 MB, return the number of KB
        char *result = malloc(32);
        snprintf(result, 32, "%.2f KB", (float) bytes / 1024.0);
        return result;
    }
    else if (bytes < 1073741824)
    {
        // less than 1 GB, return the number of MB
        char *result = malloc(32);
        snprintf(result, 32, "%.2f MB", (float) bytes / 1048576.0);
        return result;
    }
    else
    {
        // more than 1 GB, return the number of GB
        char *result = malloc(32);
        snprintf(result, 32, "%.2f GB", (float) bytes / 1073741824.0);
        return result;
    }
}
