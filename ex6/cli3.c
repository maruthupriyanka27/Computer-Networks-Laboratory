
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

struct Student {
    int regno;
    char name[50];
    char dept[30];
    int semester;
    float marks;
    char grade[5];
    float cgpa;
};

struct Request {
    int choice;
    struct Student st;
};

int main() {
    int s, n;
    struct sockaddr_in server;
    struct Request req;
    char result[300];

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        perror("Socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(s);
        return 1;
    }

    printf("Connected to Student Server.\n");

    while (1) {
        memset(&req, 0, sizeof(req));

        printf("\n--- Student Result Management ---\n");
        printf("1. Add Student\n");
        printf("2. Display Students\n");
        printf("3. Search Student\n");
        printf("4. Update Student\n");
        printf("5. Delete Student\n");
        printf("6. Exit\n");
        printf("7. Class Topper\n");
        printf("Enter choice: ");

        if (scanf("%d", &req.choice) != 1)
            break;

        if (req.choice == 1 || req.choice == 4) {
            printf("Register number: ");
            scanf("%d", &req.st.regno);

            printf("Name (no spaces): ");
            scanf("%49s", req.st.name);

            printf("Department (no spaces): ");
            scanf("%29s", req.st.dept);

            printf("Semester: ");
            scanf("%d", &req.st.semester);

            printf("Subject marks: ");
            scanf("%f", &req.st.marks);

            printf("Grade: ");
            scanf("%4s", req.st.grade);

            printf("CGPA: ");
            scanf("%f", &req.st.cgpa);
        }

        else if (req.choice == 3 || req.choice == 5) {
            printf("Enter register number: ");
            scanf("%d", &req.st.regno);
        }

        else if (req.choice < 1 || req.choice > 7) {
            printf("Invalid choice\n");
            continue;
        }

        if (send(s, &req, sizeof(req), 0) != sizeof(req)) {
            printf("Send failed\n");
            break;
        }

        n = recv(s, result, sizeof(result), MSG_WAITALL);

        if (n <= 0) {
            printf("Server disconnected\n");
            break;
        }

        result[sizeof(result) - 1] = '\0';
        printf("\nServer: %s\n", result);

        if (req.choice == 6)
            break;
    }

    close(s);
    return 0;
}
