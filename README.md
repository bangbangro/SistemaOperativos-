#  Tarea 2 - Sistemas operativos 

Este proyecto implementa una simulación concurrente en C inspirada en el videojuego Doom.

**Existen dos versiones:**
1. Versión con un héroe (doom.c)
2. Versión con varios héroes (doom2.c)

---

##  Estructura del sistema

1. **Héroes**
- Se mueven en un grid siguiendo una ruta predefinida desde su posición inicial hasta su destino.
- Atacan a los monstruos que estén dentro de su rango de ataque.
- Cada héroe corre en un hilo independiente para permitir concurrencia.

2. **Monstruos**
- Permanecen dormidos hasta detectar un héroe dentro de su rango de visión o ser alertados por otro monstruo.
- Pueden moverse hacia los héroes y atacarlos si están dentro de su rango de ataque.
- Cada monstruo corre en un hilo independiente.

3. **Simulación concurrente**
- Se sincroniza mediante mutexes y variables de condición para asegurar que movimientos y ataques no se superpongan.
- La simulación imprime en consola todos los movimientos, ataques, alertas y muertes.

---

##  Archivos del proyecto

- doom.c → Versión con un solo héroe.
- doom2.c → Versión con varios héroes.
- configuracion.txt → Archivo de configuración con la grilla, héroes y monstruos.

---

##  Compilación

Compilar la versión que desée:

Versión un héroe:
```bash
gcc doom.c -o doom -lpthread
./doom configuracion.txt
```
Versión varios héroes
```bash
gcc doom_multi_heroes.c -o doom_multi_heroes -lpthread
./doom_multi_heroes config.txt
```

---

##  Archivo de configuración



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
