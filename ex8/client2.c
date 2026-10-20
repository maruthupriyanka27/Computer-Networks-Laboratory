
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8084

struct Request {
    int choice, book_id;
    char query[60];
};

struct Response {
    char message[4096];
};

int send_all(int fd, const void *buf, size_t size) {
    const char *p = buf;
    while (size > 0) {
        ssize_t n = send(fd, p, size, 0);
        if (n <= 0) return 0;
        p += n;
        size -= n;
    }
    return 1;
}

int recv_all(int fd, void *buf, size_t size) {
    char *p = buf;
    while (size > 0) {
        ssize_t n = recv(fd, p, size, 0);
        if (n <= 0) return 0;
        p += n;
        size -= n;
    }
    return 1;
}

int main(void) {
    int s;
    struct sockaddr_in server;
    struct Request q;
    struct Response r;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(s);
        return 1;
    }

    while (1) {
        memset(&q, 0, sizeof(q));

        printf("\n--- COLLEGE LIBRARY MENU ---\n");
        printf("1. View all books\n");
        printf("2. Search by title or author\n");
        printf("3. Issue a book\n");
        printf("4. Return a book\n");
        printf("5. View issued books\n");
        printf("6. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &q.choice) != 1) break;
        getchar();

        if (q.choice == 2) {
            printf("Enter title or author (use underscores for spaces): ");
            if (!fgets(q.query, sizeof(q.query), stdin)) break;
            q.query[strcspn(q.query, "\n")] = '\0';

            if (strlen(q.query) == 0) {
                printf("Search text cannot be empty.\n");
                continue;
            }
        } else if (q.choice == 3 || q.choice == 4) {
            printf("Enter book ID: ");
            if (scanf("%d", &q.book_id) != 1) break;
            getchar();
        } else if (q.choice < 1 || q.choice > 6) {
            printf("Invalid choice. Try again.\n");
            continue;
        }

        if (!send_all(s, &q, sizeof(q)) ||
            !recv_all(s, &r, sizeof(r))) {
            printf("Server disconnected.\n");
            break;
        }

        r.message[sizeof(r.message) - 1] = '\0';
        printf("\nServer response:\n%s\n", r.message);

        if (q.choice == 6) break;
    }

    close(s);
    return 0;
}
