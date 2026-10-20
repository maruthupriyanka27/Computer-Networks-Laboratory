#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int s;
    char msg[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(s, (struct sockaddr *)&server, sizeof(server));

    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = '\0';

    send(s, msg, strlen(msg), 0);

    recv(s, msg, sizeof(msg) - 1, 0);
    msg[sizeof(msg) - 1] = '\0';

    printf("Echo from server: %s\n", msg);

    close(s);
    return 0;
}
