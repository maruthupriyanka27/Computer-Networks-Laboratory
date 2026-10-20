
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8082

struct Request {
    int seq;
    float a, b;
    char op;
};

struct Response {
    int seq;
    int status;
    char message[100];
};

int main(void) {
    int s, n, seq = 0;
    struct sockaddr_in server;
    socklen_t len = sizeof(server);
    struct Request req;
    struct Response res;
    struct timeval timeout = {3, 0};

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

        printf("\n--- UDP CALCULATOR ---\n");
        printf("Enter operator (+, -, *, /) or x to exit: ");

        if (scanf(" %c", &req.op) != 1)
            break;

        if (req.op != 'x' && req.op != 'X' &&
            req.op != '+' && req.op != '-' &&
            req.op != '*' && req.op != '/') {
            printf("Invalid operator. Try again.\n");
            seq--;
            continue;
        }

        if (req.op != 'x' && req.op != 'X') {
            printf("Enter two numbers: ");

            if (scanf("%f %f", &req.a, &req.b) != 2) {
                printf("Invalid numeric input.\n");
                close(s);
                return 1;
            }
        }

        if (sendto(s, &req, sizeof(req), 0,
                   (struct sockaddr *)&server, len) < 0) {
            perror("sendto");
            break;
        }

        n = recvfrom(s, &res, sizeof(res), 0,
                     (struct sockaddr *)&server, &len);

        if (n == sizeof(res) && res.seq == req.seq) {
            res.message[sizeof(res.message) - 1] = '\0';
            printf("Server: %s\n", res.message);
        } else {
            printf("No valid response from server.\n");
        }

        if (req.op == 'x' || req.op == 'X')
            break;
    }

    close(s);
    return 0;
}
