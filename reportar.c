#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h> 
#include <signal.h>

struct Reportar
{
    pid_t pid;
    int contador;
};



int main(){

    int fd = open("/tmp/reporte.fifo", O_RDONLY);


    struct Reportar reportar[100];
    int num_reportes = 0; 

    pid_t pid_reportado;

    while (1) {
        int n = read(fd, &pid_reportado, sizeof(pid_reportado));
        if (n > 0) {
            int encontrado = 0; 
            printf("[Reporte] Llegó un reporte contra %d\n", pid_reportado);
            for (int i = 0; i < num_reportes; i++) {
            if (reportar[i].pid == pid_reportado) {
                reportar[i].contador++;
                

                printf("El usuario %d tiene %d reportes\n", pid_reportado, reportar[i].contador);

                if(reportar[i].contador >= 10 ){
                    printf("Usuario %d expulsado del servidor", pid_reportado);
                    kill(pid_reportado, SIGKILL);

                }

                encontrado = 1;

                break;

            } 
        }

         if (!encontrado && num_reportes < 100) {
                reportar[num_reportes].pid = pid_reportado;
                reportar[num_reportes].contador = 1;
                num_reportes++;
                printf("[Reporte] Nuevo reporte contra PID %d\n", pid_reportado);
            }

        
    }
 }
   close(fd);
    return 0;
}
