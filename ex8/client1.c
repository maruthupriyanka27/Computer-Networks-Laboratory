
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8082

struct Request {
    int choice, patient_id, doctor_id, slot, appointment_id;
    char name[50];
};

struct Response {
    char message[2048];
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

        printf("\n--- HOSPITAL MENU ---\n");
        printf("1. Register patient\n");
        printf("2. View doctors\n");
        printf("3. Book appointment\n");
        printf("4. Cancel appointment\n");
        printf("5. Check doctor availability\n");
        printf("6. View patient appointment history\n");
        printf("7. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &q.choice) != 1) break;
        getchar();

        if (q.choice == 1) {
            printf("Enter patient name: ");
            if (!fgets(q.name, sizeof(q.name), stdin)) break;
            q.name[strcspn(q.name, "\n")] = '\0';
        } else if (q.choice == 3) {
            printf("Enter patient ID: ");
            if (scanf("%d", &q.patient_id) != 1) break;
            printf("Enter doctor ID (1-3): ");
            if (scanf("%d", &q.doctor_id) != 1) break;
            printf("Enter slot (1-3): ");
            if (scanf("%d", &q.slot) != 1) break;
        } else if (q.choice == 4) {
            printf("Enter appointment ID: ");
            if (scanf("%d", &q.appointment_id) != 1) break;
        } else if (q.choice == 6) {
            printf("Enter patient ID: ");
            if (scanf("%d", &q.patient_id) != 1) break;
        } else if (q.choice < 1 || q.choice > 7) {
            printf("Invalid choice.\n");
            continue;
        }

        if (!send_all(s, &q, sizeof(q)) ||
            !recv_all(s, &r, sizeof(r))) {
            printf("Server disconnected.\n");
            break;
        }

        r.message[sizeof(r.message) - 1] = '\0';
        printf("\nServer response:\n%s\n", r.message);

        if (q.choice == 7) break;
    }

    close(s);
    return 0;
}
