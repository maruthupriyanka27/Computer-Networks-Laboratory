
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int s, c;
    char msg[100];

    s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));
    listen(s, 5);

    printf("Server waiting...\n");

    c = accept(s, NULL, NULL);
    printf("Client connected.\n");

    recv(c, msg, sizeof(msg) - 1, 0);
    msg[99] = '\0';

    printf("Client says: %s\n", msg);
    send(c, msg, strlen(msg), 0);

    printf("Message echoed.\n");

    close(c);
    close(s);
    return 0;
}
