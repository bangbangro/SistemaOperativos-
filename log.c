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

struct Usuario {
    pid_t pid;
    int fd; 
};

int main(){

    mkfifo("central.fifo", 0666);

    int fd_central_in = open("central.fifo", O_RDONLY);

struct Usuario usuarios[100];

int num = 0; 

struct Mensaje msg; 

while (1)
{
    int n = read(fd_central_in, &msg, sizeof(msg));

    if(n>0){

        for (int i = 0; i < num; i++)
        {
            if(usuarios[i].pid != msg.pid){
                write(usuarios[i].fd, msg.buf, strlen(msg.buf));
            }
        }

    }
}

close(fd_central_in);
return 0;        
    





}
