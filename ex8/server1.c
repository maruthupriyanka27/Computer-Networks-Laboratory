
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8082
#define MAX 100

struct Request {
    int choice, patient_id, doctor_id, slot, appointment_id;
    char name[50];
};

struct Patient {
    int id;
    char name[50];
};

struct Appointment {
    int id, patient_id, doctor_id, slot, active;
};

struct Response {
    char message[2048];
};

struct Patient patients[MAX];
struct Appointment appointments[MAX];
int np = 0, na = 0, next_patient = 1, next_appt = 1;

const char *doctors[] = {
    "Dr. Kumar - Cardiology",
    "Dr. Priya - Neurology",
    "Dr. Ravi - General Medicine"
};

void log_event(const char *msg) {
    FILE *f = fopen("hospital.log", "a");
    if (f) {
        time_t now = time(NULL);
        fprintf(f, "%s %s\n", ctime(&now), msg);
        fclose(f);
    }
}

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

int find_patient(int id) {
    for (int i = 0; i < np; i++)
        if (patients[i].id == id) return i;
    return -1;
}

int slot_taken(int doctor, int slot) {
    for (int i = 0; i < na; i++)
        if (appointments[i].active &&
            appointments[i].doctor_id == doctor &&
            appointments[i].slot == slot)
            return 1;
    return 0;
}

void process(struct Request *q, struct Response *r) {
    memset(r, 0, sizeof(*r));

    switch (q->choice) {
    case 1:
        if (np >= MAX) {
            strcpy(r->message, "Patient database is full.");
            break;
        }
        if (strlen(q->name) == 0) {
            strcpy(r->message, "Invalid patient name.");
            break;
        }
        patients[np].id = next_patient++;
        snprintf(patients[np].name, sizeof(patients[np].name),
                 "%s", q->name);
        snprintf(r->message, sizeof(r->message),
                 "Patient registered. Patient ID: %d",
                 patients[np].id);
        np++;
        log_event("Patient registered");
        break;

    case 2:
        snprintf(r->message, sizeof(r->message),
                 "Available doctors:\n1. %s\n2. %s\n3. %s\n"
                 "Slots: 1, 2, 3 (one appointment per doctor per slot)",
                 doctors[0], doctors[1], doctors[2]);
        break;

    case 3: {
        if (find_patient(q->patient_id) < 0) {
            strcpy(r->message, "Patient ID not found.");
            break;
        }
        if (q->doctor_id < 1 || q->doctor_id > 3 ||
            q->slot < 1 || q->slot > 3) {
            strcpy(r->message, "Invalid doctor ID or slot.");
            break;
        }
        if (slot_taken(q->doctor_id, q->slot)) {
            strcpy(r->message, "Slot unavailable. Choose another slot.");
            break;
        }
        if (na >= MAX) {
            strcpy(r->message, "Appointment database is full.");
            break;
        }

        appointments[na].id = next_appt++;
        appointments[na].patient_id = q->patient_id;
        appointments[na].doctor_id = q->doctor_id;
        appointments[na].slot = q->slot;
        appointments[na].active = 1;

        snprintf(r->message, sizeof(r->message),
                 "Appointment booked successfully. Appointment ID: %d",
                 appointments[na].id);
        na++;
        log_event("Appointment booked");
        break;
    }

    case 4: {
        int found = 0;
        for (int i = 0; i < na; i++) {
            if (appointments[i].id == q->appointment_id &&
                appointments[i].active) {
                appointments[i].active = 0;
                found = 1;
                break;
            }
        }
        if (found) {
            strcpy(r->message, "Appointment cancelled successfully.");
            log_event("Appointment cancelled");
        } else {
            strcpy(r->message, "Active appointment ID not found.");
        }
        break;
    }

    case 5: {
        int used = snprintf(r->message, sizeof(r->message),
                            "Doctor availability:\n");
        for (int d = 1; d <= 3; d++) {
            used += snprintf(r->message + used,
                             sizeof(r->message) - used,
                             "%s: ", doctors[d - 1]);
            for (int s = 1; s <= 3; s++) {
                used += snprintf(r->message + used,
                                 sizeof(r->message) - used,
                                 "Slot %d=%s  ", s,
                                 slot_taken(d, s) ? "Booked" : "Free");
            }
            used += snprintf(r->message + used,
                             sizeof(r->message) - used, "\n");
        }
        break;
    }

    case 6: {
        int used = snprintf(r->message, sizeof(r->message),
                            "Appointment history:\n");
        int found = 0;
        for (int i = 0; i < na; i++) {
            if (appointments[i].patient_id == q->patient_id) {
                used += snprintf(r->message + used,
                                 sizeof(r->message) - used,
                                 "Appointment %d, Doctor: %s, Slot: %d, "
                                 "Status: %s\n",
                                 appointments[i].id,
                                 appointments[i].doctor_id >= 1 &&
                                 appointments[i].doctor_id <= 3 ?
                                 doctors[appointments[i].doctor_id - 1] :
                                 "Unknown",
                                 appointments[i].slot,
                                 appointments[i].active ?
                                 "Booked" : "Cancelled");
                found = 1;
                if (used >= (int)sizeof(r->message) - 150) break;
            }
        }
        if (!found) strcpy(r->message, "No appointments found for this patient.");
        break;
    }

    case 7:
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

    printf("Hospital iterative TCP server on port %d...\n", PORT);

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

            if (q.choice == 7) break;
        }

        close(client_fd);
        printf("Client disconnected. Waiting for next client...\n");
    }

    close(s);
    return 0;
}
