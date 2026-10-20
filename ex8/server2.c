
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8084
#define MAX 100

struct Book {
    int id, issued;
    char title[60], author[50], category[40];
};

struct Request {
    int choice, book_id;
    char query[60];
};

struct Response {
    char message[4096];
};

struct Book books[MAX];
int count = 0;

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

void log_event(const char *message) {
    FILE *fp = fopen("library4.log", "a");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "%s %s\n", ctime(&now), message);
        fclose(fp);
    }
}

void save_books(void) {
    FILE *fp = fopen("library4.dat", "wb");
    if (fp) {
        fwrite(&count, sizeof(count), 1, fp);
        fwrite(books, sizeof(struct Book), count, fp);
        fclose(fp);
    } else {
        perror("Unable to save library file");
    }
}

void load_books(void) {
    FILE *fp = fopen("library4.dat", "rb");

    if (fp) {
        if (fread(&count, sizeof(count), 1, fp) != 1 ||
            count < 0 || count > MAX ||
            fread(books, sizeof(struct Book), count, fp) !=
                (size_t)count) {
            count = 0;
        }
        fclose(fp);
    }

    if (count == 0) {
        struct Book initial[] = {
            {101, 0, "C_Programming", "Dennis_Ritchie", "Programming"},
            {102, 0, "Computer_Networks", "Tanenbaum", "Networking"},
            {103, 0, "Operating_Systems", "Galvin", "Systems"},
            {104, 0, "Data_Structures", "Mark_Weiss", "Programming"},
            {105, 0, "Database_Systems", "Elmasri", "Database"}
        };

        count = 5;
        memcpy(books, initial, sizeof(initial));
        save_books();
    }
}

int find_book(int id) {
    for (int i = 0; i < count; i++)
        if (books[i].id == id) return i;
    return -1;
}

void process(struct Request *q, struct Response *r) {
    memset(r, 0, sizeof(*r));

    switch (q->choice) {
    case 1: {
        int used = snprintf(r->message, sizeof(r->message),
                            "LIBRARY CATALOG\n");
        for (int i = 0; i < count; i++) {
            used += snprintf(r->message + used,
                             sizeof(r->message) - used,
                             "ID:%d | Title:%s | Author:%s | "
                             "Category:%s | Status:%s\n",
                             books[i].id, books[i].title,
                             books[i].author, books[i].category,
                             books[i].issued ? "Issued" : "Available");
            if (used >= (int)sizeof(r->message) - 150) break;
        }
        break;
    }

    case 2: {
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strstr(books[i].title, q->query) ||
                strstr(books[i].author, q->query)) {
                snprintf(r->message, sizeof(r->message),
                         "Book found: ID:%d | Title:%s | Author:%s | "
                         "Category:%s | Status:%s",
                         books[i].id, books[i].title, books[i].author,
                         books[i].category,
                         books[i].issued ? "Issued" : "Available");
                found = 1;
                break;
            }
        }
        if (!found) strcpy(r->message, "Book not found.");
        log_event("Book search performed");
        break;
    }

    case 3: {
        int i = find_book(q->book_id);
        if (i < 0) {
            strcpy(r->message, "Book ID not found.");
        } else if (books[i].issued) {
            strcpy(r->message, "Book is already issued.");
        } else {
            books[i].issued = 1;
            save_books();
            snprintf(r->message, sizeof(r->message),
                     "Book issued successfully: %s", books[i].title);
            log_event("Book issued");
        }
        break;
    }

    case 4: {
        int i = find_book(q->book_id);
        if (i < 0) {
            strcpy(r->message, "Book ID not found.");
        } else if (!books[i].issued) {
            strcpy(r->message, "Book is not currently issued.");
        } else {
            books[i].issued = 0;
            save_books();
            snprintf(r->message, sizeof(r->message),
                     "Book returned successfully: %s", books[i].title);
            log_event("Book returned");
        }
        break;
    }

    case 5: {
        int used = snprintf(r->message, sizeof(r->message),
                            "ISSUED BOOKS\n");
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (books[i].issued) {
                used += snprintf(r->message + used,
                                 sizeof(r->message) - used,
                                 "ID:%d | Title:%s | Author:%s\n",
                                 books[i].id, books[i].title,
                                 books[i].author);
                found = 1;
                if (used >= (int)sizeof(r->message) - 100) break;
            }
        }
        if (!found) strcpy(r->message, "No books are currently issued.");
        break;
    }

    case 6:
        strcpy(r->message, "Goodbye.");
        break;

    default:
        strcpy(r->message, "Invalid menu choice.");
    }
}

int main(void) {
    int s, client_fd;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0 ||
        listen(s, 5) < 0) {
        perror("bind/listen");
        close(s);
        return 1;
    }

    load_books();
    printf("Library iterative TCP server on port %d...\n", PORT);

    while (1) {
        client_fd = accept(s, (struct sockaddr *)&client, &len);
        if (client_fd < 0) { perror("accept"); continue; }

        printf("Client connected: %s\n", inet_ntoa(client.sin_addr));

        while (1) {
            struct Request q;
            struct Response r;

            if (!recv_all(client_fd, &q, sizeof(q))) break;

            process(&q, &r);

            if (!send_all(client_fd, &r, sizeof(r))) break;

            if (q.choice == 6) break;
        }

        close(client_fd);
        printf("Client disconnected. Waiting for next client...\n");
    }

    close(s);
    return 0;
}
