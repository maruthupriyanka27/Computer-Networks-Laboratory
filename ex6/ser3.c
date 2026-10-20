
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
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

void log_action(char *msg) {
    FILE *fp = fopen("student.log", "a");
    time_t now = time(NULL);

    if (fp != NULL) {
        fprintf(fp, "%s: %s\n", ctime(&now), msg);
        fclose(fp);
    }
}

int main() {
    int s, c, found, choice;
    struct sockaddr_in server;
    struct Request req;
    struct Student st, topper;
    FILE *fp, *temp;
    char result[300];

    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        perror("Socket");
        return 1;
    }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind");
        close(s);
        return 1;
    }

    listen(s, 5);
    printf("Student server waiting on port 8080...\n");

    c = accept(s, NULL, NULL);

    if (c < 0) {
        perror("Accept");
        close(s);
        return 1;
    }

    printf("Client connected.\n");

    while (recv(c, &req, sizeof(req), MSG_WAITALL) == sizeof(req)) {
        choice = req.choice;
        strcpy(result, "Operation completed");

        if (choice == 1) {
            fp = fopen("students.dat", "ab");

            if (fp != NULL) {
                fwrite(&req.st, sizeof(st), 1, fp);
                fclose(fp);
                strcpy(result, "Student added successfully");
                log_action("Student added");
            } else {
                strcpy(result, "File error");
            }
        }

        else if (choice == 2) {
            fp = fopen("students.dat", "rb");

            if (fp == NULL) {
                strcpy(result, "No student records");
            } else {
                printf("\n--- Student Records ---\n");

                while (fread(&st, sizeof(st), 1, fp) == 1) {
                    printf("Reg No: %d, Name: %s, Dept: %s, "
                           "Semester: %d, Marks: %.2f, "
                           "Grade: %s, CGPA: %.2f\n",
                           st.regno, st.name, st.dept,
                           st.semester, st.marks,
                           st.grade, st.cgpa);
                }

                fclose(fp);
                strcpy(result, "Records displayed on server");
                log_action("Student records displayed");
            }
        }

        else if (choice == 3) {
            found = 0;
            fp = fopen("students.dat", "rb");

            if (fp != NULL) {
                while (fread(&st, sizeof(st), 1, fp) == 1) {
                    if (st.regno == req.st.regno) {
                        snprintf(result, sizeof(result),
                            "Reg No: %d, Name: %s, Dept: %s, "
                            "Semester: %d, Marks: %.2f, "
                            "Grade: %s, CGPA: %.2f",
                            st.regno, st.name, st.dept,
                            st.semester, st.marks,
                            st.grade, st.cgpa);
                        found = 1;
                        break;
                    }
                }

                fclose(fp);
            }

            if (!found)
                strcpy(result, "Student not found");

            log_action("Student search performed");
        }

        else if (choice == 4 || choice == 5) {
            found = 0;
            fp = fopen("students.dat", "rb");
            temp = fopen("temp.dat", "wb");

            if (temp == NULL) {
                if (fp != NULL)
                    fclose(fp);

                strcpy(result, "File error");
            } else {
                while (fp != NULL &&
                       fread(&st, sizeof(st), 1, fp) == 1) {
                    if (st.regno == req.st.regno) {
                        found = 1;

                        if (choice == 4)
                            fwrite(&req.st, sizeof(st), 1, temp);
                    } else {
                        fwrite(&st, sizeof(st), 1, temp);
                    }
                }

                if (fp != NULL)
                    fclose(fp);

                fclose(temp);

                if (found) {
                    remove("students.dat");
                    rename("temp.dat", "students.dat");

                    if (choice == 4) {
                        strcpy(result, "Student updated successfully");
                        log_action("Student updated");
                    } else {
                        strcpy(result, "Student deleted successfully");
                        log_action("Student deleted");
                    }
                } else {
                    remove("temp.dat");
                    strcpy(result, "Student not found");
                }
            }
        }

        else if (choice == 6) {
            strcpy(result, "Session terminated");
            send(c, result, sizeof(result), 0);
            log_action("Client disconnected");
            break;
        }

        else if (choice == 7) {
            found = 0;
            fp = fopen("students.dat", "rb");

            if (fp != NULL) {
                while (fread(&st, sizeof(st), 1, fp) == 1) {
                    if (!found || st.marks > topper.marks) {
                        topper = st;
                        found = 1;
                    }
                }

                fclose(fp);
            }

            if (found) {
                snprintf(result, sizeof(result),
                    "Class Topper: %s, Reg No: %d, Marks: %.2f, CGPA: %.2f",
                    topper.name, topper.regno,
                    topper.marks, topper.cgpa);
                log_action("Class topper displayed");
            } else {
                strcpy(result, "No student records");
            }
        }

        else {
            strcpy(result, "Invalid choice");
        }

        send(c, result, sizeof(result), 0);
    }

    close(c);
    close(s);

    return 0;
}
