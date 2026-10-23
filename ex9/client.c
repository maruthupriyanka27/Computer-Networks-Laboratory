
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 512

static int send_text(int fd, const char *text) {
    size_t len = strlen(text), sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, text + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int read_line(int fd, char *buf, size_t size) {
    size_t i = 0;

    while (i + 1 < size) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);

        if (n == 0) return i ? (buf[i] = '\0', 1) : 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }

        if (c == '\n') break;
        if (c != '\r') buf[i++] = c;
    }

    buf[i] = '\0';
    return 1;
}

static void trim(char *s) {
    size_t n = strlen(s);

    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = '\0';
}

static int receive_response(int fd) {
    char line[BUFFER_SIZE];

    while (1) {
        int result = read_line(fd, line, sizeof(line));

        if (result <= 0) return -1;
        if (strcmp(line, "END") == 0) return 0;

        puts(line);
    }
}

int main(int argc, char *argv[]) {
    const char *server_ip =
        argc > 1 ? argv[1] : "127.0.0.1";

    int port = argc > 2 ? atoi(argv[2]) : 8080;

    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port number.\n");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, server_ip,
                  &server.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IPv4 address.\n");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0) {
        perror("connect");
        fprintf(stderr,
                "Check server IP, port and firewall settings.\n");
        close(sock);
        return 1;
    }

    struct sockaddr_in local = {0};
    socklen_t local_len = sizeof(local);

    if (getsockname(sock, (struct sockaddr *)&local,
                    &local_len) == 0) {
        char local_ip[INET_ADDRSTRLEN] = "?";
        inet_ntop(AF_INET, &local.sin_addr,
                  local_ip, sizeof(local_ip));

        printf("Client: %s:%d\n",
               local_ip, ntohs(local.sin_port));
        printf("Server: %s:%d\n",
               server_ip, port);
    }

    char line[BUFFER_SIZE];

    /* Read the server greeting until READY. */
    while (1) {
        int result = read_line(sock, line, sizeof(line));

        if (result <= 0) {
            fprintf(stderr, "Server disconnected.\n");
            close(sock);
            return 1;
        }

        if (strcmp(line, "READY") == 0) break;
        puts(line);
    }

    while (1) {
        printf("\n===== HOTEL RESERVATION SYSTEM =====\n");
        printf("1. View rooms\n");
        printf("2. Book a room\n");
        printf("3. View all reservations\n");
        printf("4. Search booking ID\n");
        printf("5. Cancel reservation\n");
        printf("6. Exit\n");
        printf("Enter choice: ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        trim(line);
        int choice = atoi(line);
        char command[BUFFER_SIZE];

        if (choice == 1) {
            snprintf(command, sizeof(command), "LIST\n");
        } else if (choice == 2) {
            char room[32], nights[32], guest[100];

            printf("Enter room number (101-110): ");
            if (!fgets(room, sizeof(room), stdin)) continue;
            trim(room);

            printf("Enter number of nights: ");
            if (!fgets(nights, sizeof(nights), stdin)) continue;
            trim(nights);

            printf("Enter guest name: ");
            if (!fgets(guest, sizeof(guest), stdin)) continue;
            trim(guest);

            if (strchr(guest, '\n') != NULL) {
                fprintf(stderr, "Guest name is too long.\n");
                continue;
            }

            if (guest[0] == '\0' ||
                strchr(guest, '\r') || strchr(guest, '\n')) {
                printf("Guest name cannot be empty.\n");
                continue;
            }

            snprintf(command, sizeof(command),
                     "BOOK %d %d %s\n",
                     atoi(room), atoi(nights), guest);

        } else if (choice == 3) {
            snprintf(command, sizeof(command), "RESERVATIONS\n");

        } else if (choice == 4 || choice == 5) {
            char id[32];

            printf("Enter booking ID: ");
            if (!fgets(id, sizeof(id), stdin)) continue;
            trim(id);

            snprintf(command, sizeof(command), "%s %d\n",
                     choice == 4 ? "SEARCH" : "CANCEL",
                     atoi(id));

        } else if (choice == 6) {
            send_text(sock, "QUIT\n");
            receive_response(sock);
            break;

        } else {
            printf("Invalid choice. Enter 1-6.\n");
            continue;
        }

        if (send_text(sock, command) < 0) {
            perror("send");
            break;
        }

        if (receive_response(sock) < 0) {
            printf("Server disconnected.\n");
            break;
        }
    }

    close(sock);
    printf("Disconnected from hotel server.\n");
    return 0;
}
