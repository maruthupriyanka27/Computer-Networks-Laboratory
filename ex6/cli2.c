
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int s, n;
    char str[100], result[100];

    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(s);
        return 1;
    }

    printf("Enter a string: ");
    scanf("%99s", str);

    send(s, str, strlen(str), 0);

    n = recv(s, result, sizeof(result) - 1, 0);

    if (n > 0) {
        result[n] = '\0';
        printf("Server says: %s\n", result);
    } else {
        printf("No response from server.\n");
    }

    close(s);
    return 0;
}
