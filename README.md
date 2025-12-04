#  Tarea 3 - Sistemas operativos 

Esta tarea implementa una simulación de gestión de memoria virtual de un sistema operativo, donde gestiona la RAM y el Swap cuando varios procesos intentan acceder a sus páginas de memoria. Entonces, en esta tarea se ve la Paginación, Page foults y un algoritmo de reemplazo, que en nuestro caso es LRU (Least Recently Used).

---
##  Descripción del funcionamiento

**Estructura de la Memoria** 

- frame(marco): Representa un espacio real de la memoria.

- page(página): Representa un bloque de memoria virtual de un proceso

- process(proceso): Representa un programa en ejecución 

**Gestión de la memoria**

El código simula el software y hardware del gestor de memoria:

1. **Inicio:**  
   El usuario define el tamaño de la RAM y de las páginas.  
   Luego, el programa calcula cuántos frames caben en la RAM, cuánto mide la memoria virtual (entre 1.5× y 4.5× de la RAM), y reserva espacio adicional para Swap, equivalente al doble de la RAM.

2. **Acceso a memoria:**  
   Cuando un proceso quiere leer una dirección virtual:
   - Se calcula a qué página pertenece la dirección.  
   - Se revisa en la Page Table si la página está cargada en RAM.  
   - Si está en RAM, se actualiza `last_access` para LRU.  
   - Si no está, ocurre un Page Fault.

3. **Manejo de Page Fault:**  
   - Se busca un frame libre en RAM.  
   - Si la RAM está llena, se aplica LRU, seleccionando el frame cuyo `last_access` es más antiguo.  
   - La página víctima se mueve a Swap y se actualizan las Page Tables.  
   - Se carga la nueva página solicitada en el frame liberado.  
   - Si Swap está llena, el simulador termina automáticamente, como exige la guía.

4. **Simulación (main):**

   El `main` simula el paso del tiempo en forma de ciclos:

   - Cada 2 segundos, se crea un proceso con tamaño aleatorio.  
   - Después de 30 segundos, en cada ciclo se realizan accesos aleatorios a memoria, lo que va llenando la RAM y provoca Page Faults.  
   - Cada 5 segundos, se finaliza un proceso aleatorio para liberar memoria.



---

## Como compilar y ejecutar

- 1. Compilar

```bash
gcc paginacion.c -o paginacion
```

- 2. Ejecutar

```bash
./paginacion
```
---

##  Observaciones

- Se usó malloc para manejar estructuras dinamicas, ya que un array estático limitaría el tamaño de RAM/Swap.  
- Para que la simulación avance más rápido y alcance el uso completo de RAM y Swap, se ajustaron dos parámetros 

   - Procesos aleatorios más grandes → RAM se llena antes.  
   - Más accesos aleatorios por ciclo → aumentan los Page Faults → se activa antes el reemplazo LRU.
 
- El programa finaliza correctamente cuando:

  - RAM está llena  
  - Swap está llena  
  - Ocurre un Page Fault sin espacio → fin del programa




---

##  Autor

Proyecto desarrollado por Rocío Sánchez y Avril Peje para la tarea 3 de Sistemas Operativos.
