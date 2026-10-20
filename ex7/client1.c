
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8081

struct Request {
    int seq;
    int choice;
    int book_id;
    char title[60];
};

struct Response {
    int seq;
    char message[2048];
};

int main(void) {
    int s, attempts, received;
    int seq = 0;
    struct sockaddr_in server;
    socklen_t len = sizeof(server);
    struct Request req;
    struct Response res;
    struct timeval timeout = {2, 0};

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (1) {
        memset(&req, 0, sizeof(req));
        req.seq = ++seq;

        printf("\n--- LIBRARY MENU ---\n");
        printf("1. View Available Books\n");
        printf("2. Search Book by ID\n");
        printf("3. Issue Book\n");
        printf("4. Return Book\n");
        printf("5. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &req.choice) != 1)
            break;

        if (req.choice < 1 || req.choice > 5) {
            printf("Invalid choice. Try again.\n");
            seq--;
            continue;
        }

        if (req.choice >= 2 && req.choice <= 4) {
            printf("Enter book ID: ");
            if (scanf("%d", &req.book_id) != 1) {
                printf("Invalid book ID.\n");
                break;
            }
        }

        received = 0;

        for (attempts = 1; attempts <= 3; attempts++) {
            if (sendto(s, &req, sizeof(req), 0,
                       (struct sockaddr *)&server, len) < 0) {
                perror("sendto");
                break;
            }

            printf("Request sent (attempt %d)\n", attempts);

            if (recvfrom(s, &res, sizeof(res), 0,
                         (struct sockaddr *)&server, &len)
                    == sizeof(res) &&
                res.seq == req.seq) {
                received = 1;
                break;
            }

            printf("No ACK received. Retrying...\n");
        }

        if (received) {
            res.message[sizeof(res.message) - 1] = '\0';
            printf("\nServer response:\n%s\n", res.message);
        } else {
            printf("Request failed after 3 attempts.\n");
        }

        if (req.choice == 5)
            break;
    }

    close(s);
    return 0;
}
