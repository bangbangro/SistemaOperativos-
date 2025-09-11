#  Tarea 1 - Sistemas operativos 

Este proyecto implementa un **chat comunitario en C** utilizando **pipes (FIFOs)** para la comunicación entre procesos.  

---

##  Estructura del sistema

1. **Proceso central (`log.c`)**  
   - Maneja la totalidad del log del chat (conversación de todos los usuarios).  
   - Recibe los mensajes desde los procesos usuarios y los reenvía al resto.  
   - Gestiona los reportes de usuarios y los envía al proceso `reportar.c`.

2. **Procesos independientes (`usuarios.c`)**  
   - Se conectan al proceso central mediante FIFOs.  
   - Pueden enviar y recibir mensajes.  
   - Pueden salir del sistema en cualquier momento.  
   - Pueden clonarse mediante el comando `/fork`, creando un nuevo proceso que se comporta como un usuario independiente.

3. **Sistema de reportes (`reportar.c`)**  
   - Es un proceso secundario que se ejecuta junto al proceso central.  
   - Verifica cuántos reportes ha recibido cada proceso (por su PID).  
   - Si un proceso acumula **10 reportes**, este es expulsado del chat mediante `kill(pid, SIGKILL)`.

---

##  Archivos del proyecto

- `usuarios.c` → Implementa los clientes/usuarios del chat.  
- `log.c` → Proceso central, encargado de gestionar mensajes y registros.  
- `reportar.c` → Sistema de reportes y expulsión automática de usuarios.  

---

##  Compilación

Compilar cada módulo por separado y en ese mismo orden:

```bash
gcc log.c -o log
./log

gcc usuarios.c -o usuarios
./usuarios

gcc reportar.c -o reportar
./reportar
