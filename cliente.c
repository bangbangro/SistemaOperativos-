#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct Mensaje {
    pid_t pid;        
    char buf[256];  
};

int main() {
    pid_t pid = getpid();

    // Abrir FIFO central
    int fd_central = open("central.fifo", O_WRONLY);

    // Registrarse en el servidor
    struct Mensaje reg;
    reg.pid = pid;
    strcpy(reg.buf, "/register");
    write(fd_central, &reg, sizeof(reg));

    // Crear FIFO personal para recibir mensajes
    char fifo_name[64];
    sprintf(fifo_name, "user_%d.fifo", pid);
    mkfifo(fifo_name, 0666);

    int fd_in = open(fifo_name, O_RDONLY);

    if (fork() == 0) {
        // Hijo: escucha mensajes
        char buf[256];
        while (1) {
            int n = read(fd_in, buf, sizeof(buf));
            if (n > 0) {
                printf("[Mensaje recibido]: %s\n", buf);
            }
        }
    } else {
        // Padre: envía mensajes desde teclado
        struct Mensaje msg;
        msg.pid = pid;

        while (1) {
            fgets(msg.buf, sizeof(msg.buf), stdin);
            msg.buf[strcspn(msg.buf, "\n")] = 0; // quitar salto de línea
            write(fd_central, &msg, sizeof(msg));
        }
    }

    return 0;
}
