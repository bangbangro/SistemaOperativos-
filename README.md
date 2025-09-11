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


1. Ejecutar el proceso central (log):
   
```bash
gcc log.c -o log
./log
```

2. Ejecutar uno o varios procesos de usuarios:

```bash
gcc usuarios.c -o usuarios
./usuarios
```

3. En otra terminal, ejecutar el proceso de reportes:

```bash
gcc reportar.c -o reportar
./reportar
```

---

##  Comandos

En la interfaz de usuario, se pueden usar los siguientes comandos:

1. Enviar mensaje normal:

Solo escribir el texto y presionar Enter.

2. Clonar proceso usuario

```bash
/fork
```
Crea un nuevo cliente conectado automáticamente al chat

3. Reportar a un usuario por su PID

```bash
/report 1234
```
Envía un reporte contra el usuario con PID 1234.
Si acumula 10 reportes, el proceso será finalizado.

---
##  Ejemplo

1. Usuario con PID 1234 envía el mensaje:
   
```bash
hola a todos
```

El proceso central lo registra y lo reenvía a los demás usuarios.

2. Usuario 2324 ejecuta:
   
```bash
/report 1234
```
El proceso central envía el reporte al módulo de reportes.

3. El módulo de reportes muestra:
```bash
[Reporte] Llegó un reporte contra 1234
El usuario 1234 tiene 1 reporte
```

4. Si el usuario llega a 10 reportes:
   ```bash
Usuario 1010 expulsado del servidor
```

---

##  Autor

Proyecto desarrollado por Rocio Sanchez y Avril Peje para la tarea de Sistemas Operativos.
