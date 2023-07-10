#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        exit(1);
    }

    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Create client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        error("Error opening socket");
    }

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2]));

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, argv[1], &(server_address.sin_addr)) <= 0) {
        error("Invalid address");
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        error("Error connecting");
    }

    printf("Connected to server at %s:%s\n", argv[1], argv[2]);

    while (1) {
        // Read message from the standard input
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send the message to the server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            error("Error sending");
        }

        // Receive and print the message from the server
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == -1) {
            error("Error receiving");
        } else if (bytes_received == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            buffer[bytes_received] = '\0';
            printf("Received message from server: %s", buffer);
        }
    }

    // Close the client socket
    close(client_socket);

    return 0;
}
