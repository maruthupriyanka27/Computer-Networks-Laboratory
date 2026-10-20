
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main() {
    int s, c, n, i, palindrome;
    char str[100], result[100];

    struct sockaddr_in server;

    s = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(s, (struct sockaddr *)&server, sizeof(server));
    listen(s, 5);

    printf("Server waiting...\n");

    while (1) {
        c = accept(s, NULL, NULL);
        if (c < 0)
            continue;

        printf("Client connected.\n");

        n = recv(c, str, sizeof(str) - 1, 0);

        if (n > 0) {
            str[n] = '\0';
            palindrome = 1;

            for (i = 0; i < n / 2; i++) {
                if (str[i] != str[n - i - 1]) {
                    palindrome = 0;
                    break;
                }
            }

            if (palindrome)
                strcpy(result, "Palindrome");
            else
                strcpy(result, "Not Palindrome");

            send(c, result, strlen(result), 0);
            printf("%s -> %s\n", str, result);
        }

        close(c);
    }

    close(s);
    return 0;
}
