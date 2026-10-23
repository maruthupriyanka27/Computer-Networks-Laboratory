
#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define ROOM_COUNT 10
#define NAME_SIZE 100
#define DB_FILE "reservations.dat"

typedef struct {
    int room_no;
    int booked;
    int booking_id;
    int nights;
    char guest[NAME_SIZE];
} Room;

static int send_text(int fd, const char *text) {
    size_t len = strlen(text), sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, text + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static int read_line(int fd, char *buf, size_t size) {
    size_t i = 0;

    while (i + 1 < size) {
        char c;
        ssize_t n = recv(fd, &c, 1, 0);

        if (n == 0) return i ? (buf[i] = '\0', 1) : 0;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }

        if (c == '\n') break;
        if (c != '\r') buf[i++] = c;
    }

    buf[i] = '\0';
    return 1;
}

/* Lock and load shared reservation data. */
static int load_rooms(int *fd, Room rooms[ROOM_COUNT]) {
    *fd = open(DB_FILE, O_RDWR | O_CREAT, 0666);
    if (*fd < 0) return -1;

    if (flock(*fd, LOCK_EX) < 0) {
        close(*fd);
        return -1;
    }

    struct stat st;

    if (fstat(*fd, &st) < 0) goto error;

    if (st.st_size != (off_t)sizeof(Room) * ROOM_COUNT) {
        memset(rooms, 0, sizeof(Room) * ROOM_COUNT);

        for (int i = 0; i < ROOM_COUNT; i++)
            rooms[i].room_no = 101 + i;

        if (ftruncate(*fd, 0) < 0 ||
            pwrite(*fd, rooms, sizeof(Room) * ROOM_COUNT, 0)
                != (ssize_t)(sizeof(Room) * ROOM_COUNT))
            goto error;

        fsync(*fd);
    } else {
        if (pread(*fd, rooms, sizeof(Room) * ROOM_COUNT, 0)
                != (ssize_t)(sizeof(Room) * ROOM_COUNT))
            goto error;
    }

    return 0; /* Caller must unlock and close. */

error:
    flock(*fd, LOCK_UN);
    close(*fd);
    return -1;
}

static void unlock_rooms(int fd) {
    flock(fd, LOCK_UN);
    close(fd);
}

static int save_rooms(int fd, Room rooms[ROOM_COUNT]) {
    if (pwrite(fd, rooms, sizeof(Room) * ROOM_COUNT, 0)
            != (ssize_t)(sizeof(Room) * ROOM_COUNT))
        return -1;

    return fsync(fd);
}

static int next_booking_id(Room rooms[ROOM_COUNT]) {
    int max_id = 1000;

    for (int i = 0; i < ROOM_COUNT; i++)
        if (rooms[i].booking_id > max_id)
            max_id = rooms[i].booking_id;

    return max_id + 1;
}

static void reply(int fd, const char *message) {
    send_text(fd, message);
    send_text(fd, "END\n");
}

static void handle_command(int fd, char *cmd) {
    char out[512];
    int db;
    Room rooms[ROOM_COUNT];

    if (strcmp(cmd, "LIST") == 0) {
        if (load_rooms(&db, rooms) < 0) {
            reply(fd, "ERROR: Cannot read reservation data.\n");
            return;
        }

        send_text(fd, "\nROOMS | STATUS | PRICE PER NIGHT\n");

        for (int i = 0; i < ROOM_COUNT; i++) {
            snprintf(out, sizeof(out),
                     "Room %d | %s | Rs.%d\n",
                     rooms[i].room_no,
                     rooms[i].booked ? "BOOKED" : "AVAILABLE",
                     1500 + i * 100);
            send_text(fd, out);
        }

        send_text(fd, "END\n");
        unlock_rooms(db);
        return;
    }

    if (strcmp(cmd, "BOOK") == 0 ||
        strncmp(cmd, "BOOK ", 5) == 0) {
        int room_no, nights;
        char guest[NAME_SIZE] = {0};

        if (sscanf(cmd + 4, "%d %d %99[^\n]",
                   &room_no, &nights, guest) != 3 ||
            room_no < 101 || room_no > 110 ||
            nights < 1 || nights > 365) {
            reply(fd,
                "ERROR: Enter room number, nights and guest name.\n"
                "Example: BOOK 101 2 Priya\n");
            return;
        }

        if (load_rooms(&db, rooms) < 0) {
            reply(fd, "ERROR: Cannot read reservation data.\n");
            return;
        }

        Room *r = &rooms[room_no - 101];

        if (r->booked) {
            snprintf(out, sizeof(out),
                     "ERROR: Room %d is already booked.\n", room_no);
            reply(fd, out);
        } else {
            r->booked = 1;
            r->booking_id = next_booking_id(rooms);
            r->nights = nights;
            snprintf(r->guest, sizeof(r->guest), "%s", guest);

            if (save_rooms(db, rooms) < 0) {
                reply(fd, "ERROR: Could not save booking.\n");
            } else {
                snprintf(out, sizeof(out),
                    "BOOKING CONFIRMED\nBooking ID: %d\n"
                    "Room: %d\nGuest: %s\nNights: %d\n"
                    "Total: Rs.%d\n",
                    r->booking_id, room_no, r->guest, nights,
                    (1500 + (room_no - 101) * 100) * nights);
                reply(fd, out);
            }
        }

        unlock_rooms(db);
        return;
    }

    if (strcmp(cmd, "RESERVATIONS") == 0) {
        if (load_rooms(&db, rooms) < 0) {
            reply(fd, "ERROR: Cannot read reservation data.\n");
            return;
        }

        send_text(fd, "CURRENT RESERVATIONS\n");
        int count = 0;

        for (int i = 0; i < ROOM_COUNT; i++) {
            if (rooms[i].booked) {
                snprintf(out, sizeof(out),
                    "ID: %d | Room: %d | Guest: %s | Nights: %d\n",
                    rooms[i].booking_id, rooms[i].room_no,
                    rooms[i].guest, rooms[i].nights);
                send_text(fd, out);
                count++;
            }
        }

        if (count == 0) send_text(fd, "No reservations yet.\n");

        send_text(fd, "END\n");
        unlock_rooms(db);
        return;
    }

    if (strncmp(cmd, "SEARCH ", 7) == 0 ||
        strncmp(cmd, "CANCEL ", 7) == 0) {
        int id;
        int cancel = strncmp(cmd, "CANCEL ", 7) == 0;

        if (sscanf(cmd + 7, "%d", &id) != 1) {
            reply(fd, "ERROR: Invalid booking ID.\n");
            return;
        }

        if (load_rooms(&db, rooms) < 0) {
            reply(fd, "ERROR: Cannot read reservation data.\n");
            return;
        }

        int found = 0;

        for (int i = 0; i < ROOM_COUNT; i++) {
            Room *r = &rooms[i];

            if (r->booked && r->booking_id == id) {
                found = 1;

                if (cancel) {
                    r->booked = 0;
                    r->booking_id = 0;
                    r->nights = 0;
                    r->guest[0] = '\0';
                } else {
                    snprintf(out, sizeof(out),
                        "Booking ID: %d | Room: %d | Guest: %s | Nights: %d\n",
                        r->booking_id, r->room_no, r->guest, r->nights);
                    reply(fd, out);
                }
                break;
            }
        }

        if (!found) {
            reply(fd, "Booking ID not found.\n");
        } else if (cancel) {
            if (save_rooms(db, rooms) < 0)
                reply(fd, "ERROR: Could not save cancellation.\n");
            else
                reply(fd, "Reservation cancelled successfully.\n");
        }

        unlock_rooms(db);
        return;
    }

    if (strcmp(cmd, "QUIT") == 0) {
        reply(fd, "BYE\n");
        return;
    }

    reply(fd, "ERROR: Unknown command.\n");
}

static void serve_client(int fd, struct sockaddr_in peer) {
    char ip[INET_ADDRSTRLEN] = "?";
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));

    printf("[CONNECTED] %s:%d | PID %ld\n",
           ip, ntohs(peer.sin_port), (long)getpid());
    fflush(stdout);

    send_text(fd,
        "Welcome to Hotel Reservation Management System\n"
        "READY\n");

    char cmd[512];

    while (read_line(fd, cmd, sizeof(cmd)) > 0) {
        handle_command(fd, cmd);
        if (strcmp(cmd, "QUIT") == 0) break;
    }

    printf("[DISCONNECTED] %s:%d | PID %ld\n",
           ip, ntohs(peer.sin_port), (long)getpid());
    fflush(stdout);
    close(fd);
}

int main(int argc, char *argv[]) {
    int port = PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port < 1 || port > 65535) {
            fprintf(stderr, "Invalid port number.\n");
            return 1;
        }
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Hotel server listening on port %d\n", port);
    printf("Rooms: 101-110. Press Ctrl+C to stop.\n");
    fflush(stdout);

    while (1) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&peer, &peer_len);

        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            close(server_fd);
            serve_client(client_fd, peer);
            _exit(0);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
