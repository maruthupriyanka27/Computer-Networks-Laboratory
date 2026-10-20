
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
    int s;
    struct sockaddr_in server, client;
    socklen_t len;
    struct Request req;
    struct Response res;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind");
        close(s);
        return 1;
    }

    printf("Calculator UDP server running on port %d...\n", PORT);

    while (1) {
        len = sizeof(client);

        int n = recvfrom(s, &req, sizeof(req), 0,
                         (struct sockaddr *)&client, &len);

        if (n != sizeof(req))
            continue;

        memset(&res, 0, sizeof(res));
        res.seq = req.seq;
        res.status = 1;

        if (req.op == 'x' || req.op == 'X') {
            strcpy(res.message, "Goodbye.");
        } else {
            switch (req.op) {
                case '+':
                    snprintf(res.message, sizeof(res.message),
                             "Result = %.2f", req.a + req.b);
                    break;

                case '-':
                    snprintf(res.message, sizeof(res.message),
                             "Result = %.2f", req.a - req.b);
                    break;

                case '*':
                    snprintf(res.message, sizeof(res.message),
                             "Result = %.2f", req.a * req.b);
                    break;

                case '/':
                    if (req.b == 0) {
                        res.status = 0;
                        strcpy(res.message,
                               "Error: Division by zero");
                    } else {
                        snprintf(res.message, sizeof(res.message),
                                 "Result = %.2f", req.a / req.b);
                    }
                    break;

                default:
                    res.status = 0;
                    strcpy(res.message, "Error: Invalid operator");
            }
        }

        printf("Request %d: %.2f %c %.2f -> %s\n",
               req.seq, req.a, req.op, req.b, res.message);

        sendto(s, &res, sizeof(res), 0,
               (struct sockaddr *)&client, len);

        if (req.op == 'x' || req.op == 'X')
            printf("Exit request acknowledged.\n");
    }

    close(s);
    return 0;
}
