
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8081
#define MAX_BOOKS 100
#define MAX_MSG 2048

struct Book {
    int id;
    char title[60];
    char author[40];
    int issued;
};

struct Request {
    int seq;
    int choice;
    int book_id;
    char title[60];
};

struct Response {
    int seq;
    char message[MAX_MSG];
};

struct Cache {
    struct sockaddr_in client;
    int seq;
    struct Response response;
    int valid;
};

struct Book books[MAX_BOOKS];
int count = 0;

void log_activity(const char *message) {
    FILE *fp = fopen("library.log", "a");
    time_t now = time(NULL);

    if (fp != NULL) {
        fprintf(fp, "%s %s\n", ctime(&now), message);
        fclose(fp);
    }
}

void save_books(void) {
    FILE *fp = fopen("books.dat", "wb");
    if (fp != NULL) {
        fwrite(books, sizeof(struct Book), count, fp);
        fclose(fp);
    }
}

void load_books(void) {
    FILE *fp = fopen("books.dat", "rb");

    if (fp != NULL) {
        count = (int)fread(books, sizeof(struct Book),
                           MAX_BOOKS, fp);
        fclose(fp);
    }

    if (count == 0) {
        struct Book initial[] = {
            {1, "C_Programming", "Dennis_Ritchie", 0},
            {2, "Computer_Networks", "Tanenbaum", 0},
            {3, "Operating_Systems", "Galvin", 0},
            {4, "Data_Structures", "Mark_Allen", 0},
            {5, "Database_Systems", "Elmasri", 0}
        };

        count = 5;
        memcpy(books, initial, sizeof(initial));
        save_books();
    }
}

int find_id(int id) {
    int i;
    for (i = 0; i < count; i++) {
        if (books[i].id == id)
            return i;
    }
    return -1;
}

int same_client(struct sockaddr_in a, struct sockaddr_in b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr &&
           a.sin_port == b.sin_port;
}

int main(void) {
    int s;
    struct sockaddr_in server, client;
    socklen_t len;
    struct Request req;
    struct Response res;
    struct Cache cache[50] = {0};
    int next_cache = 0;

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

    load_books();
    printf("Library UDP server running on port %d...\n", PORT);

    while (1) {
        len = sizeof(client);

        int n = recvfrom(s, &req, sizeof(req), 0,
                         (struct sockaddr *)&client, &len);

        if (n != sizeof(req))
            continue;

        /* Resend the previous response for a duplicate request. */
        int duplicate = 0;
        for (int i = 0; i < 50; i++) {
            if (cache[i].valid &&
                cache[i].seq == req.seq &&
                same_client(cache[i].client, client)) {
                sendto(s, &cache[i].response,
                       sizeof(cache[i].response), 0,
                       (struct sockaddr *)&client, len);
                duplicate = 1;
                break;
            }
        }
        if (duplicate)
            continue;

        memset(&res, 0, sizeof(res));
        res.seq = req.seq;

        if (req.choice == 1) {
            /* Display available books. */
            int available = 0;
            int used = snprintf(res.message, sizeof(res.message),
                                "Available books:\n");

            for (int i = 0; i < count; i++) {
                if (!books[i].issued) {
                    used += snprintf(res.message + used,
                                     sizeof(res.message) - used,
                                     "ID: %d | %s | Author: %s\n",
                                     books[i].id,
                                     books[i].title,
                                     books[i].author);
                    available++;
                    if (used >= (int)sizeof(res.message) - 100)
                        break;
                }
            }

            if (!available)
                strcpy(res.message, "No books available.");

            log_activity("Listed available books");
        }

        else if (req.choice == 2) {
            /* Search for a book by ID. */
            int i = find_id(req.book_id);

            if (i < 0) {
                strcpy(res.message, "Book not found.");
            } else {
                snprintf(res.message, sizeof(res.message),
                         "Found: ID %d, %s, Author: %s, Status: %s",
                         books[i].id, books[i].title,
                         books[i].author,
                         books[i].issued ? "Issued" : "Available");
            }

            log_activity("Searched for a book");
        }

        else if (req.choice == 3) {
            /* Issue a book. */
            int i = find_id(req.book_id);

            if (i < 0) {
                strcpy(res.message, "Book not found.");
            } else if (books[i].issued) {
                strcpy(res.message, "Book is already issued.");
            } else {
                books[i].issued = 1;
                save_books();
                snprintf(res.message, sizeof(res.message),
                         "Book issued successfully: %s",
                         books[i].title);
            }

            log_activity("Book issue request");
        }

        else if (req.choice == 4) {
            /* Return a book. */
            int i = find_id(req.book_id);

            if (i < 0) {
                strcpy(res.message, "Book not found.");
            } else if (!books[i].issued) {
                strcpy(res.message, "Book was not issued.");
            } else {
                books[i].issued = 0;
                save_books();
                snprintf(res.message, sizeof(res.message),
                         "Book returned successfully: %s",
                         books[i].title);
            }

            log_activity("Book return request");
        }

        else if (req.choice == 5) {
            strcpy(res.message, "Goodbye.");
        }

        else {
            strcpy(res.message, "Invalid menu choice.");
        }

        /* Cache the response so retransmission won't repeat the action. */
        cache[next_cache].client = client;
        cache[next_cache].seq = req.seq;
        cache[next_cache].response = res;
        cache[next_cache].valid = 1;
        next_cache = (next_cache + 1) % 50;

        /* Sending this response acts as the application-level ACK. */
        sendto(s, &res, sizeof(res), 0,
               (struct sockaddr *)&client, len);

        printf("Request %d processed: %s\n",
               req.seq, res.message);

        if (req.choice == 5)
            log_activity("Client requested exit");
    }

    close(s);
    return 0;
}
