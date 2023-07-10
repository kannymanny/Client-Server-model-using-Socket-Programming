#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int server_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        error("Error opening socket");
    }

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    // Bind server socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        error("Error binding");
    }

    // Listen for client connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        error("Error listening");
    }

    printf("Server listening on port %s\n", argv[1]);

    fd_set read_fds, master_fds;
    int max_fd, activity, i, client_socket, valread, sd;
    char buffer[BUFFER_SIZE];

    // Initialize client_sockets array
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Clear the set of file descriptors
    FD_ZERO(&master_fds);

    // Add the server socket to the set
    FD_SET(server_socket, &master_fds);
    max_fd = server_socket;

    while (1) {
        // Copy master_fds to read_fds
        read_fds = master_fds;

        // Wait for activity on any of the sockets
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity == -1) {
            error("Error selecting");
        }

        // Check if there is a new incoming connection
        if (FD_ISSET(server_socket, &read_fds)) {
            client_address_length = sizeof(client_address);

            // Accept the new connection
            client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
            if (client_socket == -1) {
                error("Error accepting");
            }

            printf("New connection, socket fd is %d, IP is: %s, port is: %d\n", client_socket, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Add the new client socket to the set
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    break;
                }
            }

            // Add the new client socket to the master set
            FD_SET(client_socket, &master_fds);

            // Update the maximum file descriptor
            if (client_socket > max_fd) {
                max_fd = client_socket;
            }
        }

        // Check if there are any incoming messages
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &read_fds)) {
                // Receive the message
                valread = recv(sd, buffer, BUFFER_SIZE, 0);
                if (valread <= 0) {
                    // Connection closed or error occurred
                    getpeername(sd, (struct sockaddr *)&client_address, &client_address_length);
                    printf("Host disconnected, IP is: %s, port is: %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    // Close the socket and remove it from the set
                    close(sd);
                    FD_CLR(sd, &master_fds);
                    client_sockets[i] = 0;
                } else {
                    // Broadcast the received message to all clients
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        int dest_sd = client_sockets[j];
                        if (dest_sd != 0 && dest_sd != sd) {
                            if (send(dest_sd, buffer, valread, 0) == -1) {
                                error("Error sending");
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
