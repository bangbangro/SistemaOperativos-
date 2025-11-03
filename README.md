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

## Sincronización

Para que la simulación funcione correctamente, es clave sincronizar los hilos y evitar Condiciones de Carrera.

La Sección Critica principal es cualquier acceso a los datos compartidos (el estado y posiciones de todas las entidades).

Para proteger esta sección, se utilizan dos mecanismos:


**1. Mutex (pthread_mutex_t):** 
Se usa como un "candado" global para proteger la modificación de los datos compartidos. Cualquier thread (heroe o monstruo) que necesite modificar un estado critico (como el HP de un enemigo, su propia posición en el grid, o el estado de MUERTO) debe primero adquirir el mutex. Esto garantiza que solo un thread a la vez pueda alterar el estado del juego, previniendo que los movimientos y ataques se corrompan entre si.


**2. Variable de Condición (pthread_cond_t):**

Se utiliza para gestionar eficientemente a los monstruos.

- Los monstruos inician en un estado MONSTRUO_DORMIDO y se ponen a "esperar" (pthread_cond_wait) en esta variable.

- Cuando un heroe se mueve, o un monstruo detecta a un heroe, se envia una señal (pthread_cond_broadcast).

- Esta señal "despierta" a todos los monstruos dormidos, quienes entonces revisan si un heroe ha entrado en su rango de visión.

---

## Logica de Fin de Juego (Condiciones de Victoria)

Las dos versiones del proyecto tienen logicas diferentes para determinar cuando finaliza la simulación.

- Parte 1 (doom.c): El programa es simple y se centra en un solo heroe. La simulación termina inmediatamente si ocurre una de dos cosas:

1. El Heroe muere.
2. El Heroe llega a la última coordenada de su camino (meta).

- Parte 2 (doom2.c): Esta versión es más compleja y actúa como un "árbitro". Se añade un nuevo estado HEROE_TERMINO para marcar a los heroes que han llegado a la meta sin morir. La función main revisa el estado del juego cada segundo y termina el programa solo si se cumple una de las siguientes tres condiciones:

1. Derrota Total: Todos los heroes han sido eliminados (HEROE_MUERTO).
2. Victoria por Exterminio: Todos los monstruos han sido eliminados (MONSTRUO_MUERTO).
3. Victoria por Objetivo: Todos los heroes que quedan vivos han llegado a la meta (estado HEROE_TERMINO).

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

## Salida de ejemplo en la terminal 

La simulación imprime en consola todos los movimientos, ataques, alertas y muertes.

```
Héroe 1 atacando en (36,27)
Monstruo 8 ataca héroe 1 con daño 20, HP del héroe: 74
Monstruo 12 ataca héroe 1 con daño 22, HP del héroe: 52
Monstruo 13 se mueve hacia héroe 1: (41,27)
Héroe 1 atacando en (36,27)
Monstruo 8 muerto por héroe 1 en (36,27)
Monstruo 12 ataca héroe 1 con daño 22, HP del héroe: 30
Monstruo 13 se mueve hacia héroe 1: (40,27)
Héroe 1 atacando en (36,27)
Monstruo 12 ataca héroe 1 con daño 22, HP del héroe: 8
Monstruo 13 se mueve hacia héroe 1: (39,27)
Héroe 1 atacando en (36,27)
Monstruo 12 muerto por héroe 1 en (36,27)
Monstruo 13 se mueve hacia héroe 1: (38,27)
Héroe 1 atacando en (36,27)
Monstruo 13 se mueve hacia héroe 1: (37,27)
Héroe 1 atacando en (36,27)
Monstruo 13 muerto por héroe 1 en (36,27)
Héroe 1 se mueve a (37,27)
```

---

##  Autor
