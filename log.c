#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h> 
#include <signal.h>

struct Mensaje {
    pid_t pid;
    char buf[256];
};

struct Usuario {
    pid_t pid;
    int fd;
};

int main() {
    mkfifo("/tmp/central.fifo", 0666);
    mkfifo("/tmp/reporte.fifo", 0666);

    int fd_central_in = open("/tmp/central.fifo", O_RDONLY);
    if (fd_central_in < 0) {
        perror("Error al abrir central.fifo");
        return 1;
    }

    
    struct Usuario usuarios[100];
    int num = 0;
    struct Mensaje msg;

    while (1) {
        int n = read(fd_central_in, &msg, sizeof(msg));
        if (n > 0) {
            if (strcmp(msg.buf, "/register") == 0) {
                char fifo_name[64];
                sprintf(fifo_name, "/tmp/user_%d.fifo", msg.pid);

                int fd_user;
    while ((fd_user = open(fifo_name, O_WRONLY)) < 0) {
        perror("Esperando FIFO del usuario...");
        sleep(1);
    }

                usuarios[num].pid = msg.pid;
                usuarios[num].fd = fd_user;
                num++;

                printf("Usuario %d registrado\n", msg.pid);
                fflush(stdout);

            }  else if (strncmp(msg.buf, "/report", 7) == 0) {
                       pid_t pid_reportado = atoi(msg.buf + 8);  // convierte "1234" en int
                       printf("[Central] Recibido reporte contra %d\n", pid_reportado);
                       int fd_rep = open("/tmp/reporte.fifo", O_WRONLY);
                       if (fd_rep != -1) {
                        write(fd_rep, &pid_reportado, sizeof(pid_reportado));
                        close(fd_rep);
                    }
            } else {
                for (int i = 0; i < num; i++) {
                    if (usuarios[i].pid != msg.pid) {
                        write(usuarios[i].fd, msg.buf, strlen(msg.buf)+1);
                    }
                }
                printf("Usuario %d: %s\n", msg.pid, msg.buf);
                fflush(stdout);
            }
        }
    }

    close(fd_central_in);
    return 0;
}
