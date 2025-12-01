#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int frame_id;      // Número de marco físico
    int process_id;    // Qué proceso lo está usando (-1 si está libre)
    int page_num;      // Qué página lógica del proceso es
    int last_access;   // Timestamp para algoritmo LRU (si eliges esa política) [cite: 21]
} frame;


typedef struct {
    int page_id;
    int present;       // 1 = en RAM, 0 = en Swap (Page Fault si accedes y es 0)
    int frame_number;  // Si present=1, índice en ram_memory. Si present=0, índice en swap.
} page;

typedef struct Proceso{
    int pid;
    int size;
    page *page_table;       // Array dinámico de páginas
    int num_pages;
    struct Proceso *next;   // Puntero al siguiente proceso
} process;

frame *ram_memory = NULL;  
frame *swap_memory = NULL;

int total_ram_frames = 0;
int total_swap_frames = 0;

int virtual_memory_mb = 0;
int total_virtual_pages = 0;
int page_size_kb = 0;

void init_memory(int ram_size_mb, int page_size_kb_param) {

    page_size_kb = page_size_kb_param;

    srand(time(NULL));

    float factor = 1.5 + ((float)rand() / RAND_MAX) *(4.5 - 1.5);
    virtual_memory_mb = (int)(ram_size_mb * factor);

    total_virtual_pages = (virtual_memory_mb * 1024) / page_size_kb;


    total_ram_frames = (ram_size_mb * 1024) / page_size_kb;
    
    total_swap_frames = total_ram_frames * 2; 

    ram_memory = malloc(total_ram_frames * sizeof(frame));
    swap_memory = malloc(total_swap_frames * sizeof(frame));

    if (!ram_memory || !swap_memory) {
        printf("No se pudo asignar memoria.\n");
        exit(1);
    }

    // Inicializar arrays para marcar como libre

    for (int i = 0; i < total_ram_frames; i++) {
        ram_memory[i].frame_id = i;
        ram_memory[i].process_id = -1; // Libre
        ram_memory[i].page_num = -1;
        ram_memory[i].last_access = 0;
    }

    for (int i = 0; i < total_swap_frames; i++) {
        swap_memory[i].frame_id = i;
        swap_memory[i].process_id = -1; // Libre
        swap_memory[i].page_num = -1;
        swap_memory[i].last_access = 0;
    }
}

void buscar_frame_libre(frame *memory, int total_frames, int *frame_index) {
    for (int i = 0; i < total_frames; i++) {
        if (memory[i].process_id == -1) {
            *frame_index = i;
            return;
        }
    }
    *frame_index = -1; // No hay frame libre
}

void buscar_frame_libre_swap(int *frame_index) {
    buscar_frame_libre(swap_memory, total_swap_frames, frame_index);
}

void buscar_frame_libre_ram(int *frame_index) {
    buscar_frame_libre(ram_memory, total_ram_frames, frame_index);
}

void acceder_direccion_virtual(process *proc, int virtual_address) {
    int page_size_kb = (virtual_memory_mb * 1024) / total_virtual_pages;
    int page_number = virtual_address / (page_size_kb * 1024);

    if (page_number < 0 || page_number >= proc->num_pages) {
        printf("Error, fuera de rango para el proceso %d\n", proc->pid);
        return;
    }

    page *pg = &proc->page_table[page_number];

    if (pg->present) {
        int frame_num = pg->frame_number;
        ram_memory[frame_num].last_access = time(NULL);
        printf("Acceso exitoso a la dirección virtual %d del proceso %d en el frame %d\n", virtual_address, proc->pid, frame_num);
    } else {
        printf("Page Fault en la dirección virtual %d del proceso %d\n", virtual_address, proc->pid);
    }
}

void crear_proceso(process **proc_list, int pid, int size_kb) {
    process *new_proc = malloc(sizeof(process));

    if(!new_proc) {
        printf("No se pudo asignar memoria para el nuevo proceso.\n");
        return;
    }

    new_proc->pid = pid;
    new_proc->size = size_kb;

    if(page_size_kb == 0) {
        printf("Tamaño de página no inicializado.\n");
        free(new_proc);
        return;
    }

    new_proc->num_pages = (size_kb + page_size_kb - 1) / page_size_kb; // Redondeo hacia arriba
    new_proc->page_table = malloc(new_proc->num_pages * sizeof(page));

    if (!new_proc->page_table) {
        printf("No se pudo asignar memoria para la tabla de páginas del proceso %d.\n", pid);
        free(new_proc);
        return;
    }

    for (int i = 0; i < new_proc->num_pages; i++) {
        new_proc->page_table[i].page_id = i;
        new_proc->page_table[i].present = 0; // Inicialmente en Swap
        new_proc->page_table[i].frame_number = -1;
    }

    new_proc->next = *proc_list;
    *proc_list = new_proc;

    printf("Proceso %d creado con %d páginas.\n", pid, new_proc->num_pages);


}

void finalizar_proceso(process **proc_list, int pid) {
    process *prev = NULL;
    process *curr = *proc_list;

    while (curr) {
        if (curr->pid == pid) {
            for (int i = 0; i < curr->num_pages; i++) {
                int frame_num = curr->page_table[i].frame_number;
                if (curr->page_table[i].present) {
                    ram_memory[frame_num].process_id = -1; // Liberar frame en RAM
                    ram_memory[frame_num].page_num = -1;
                    ram_memory[frame_num].last_access = 0;
                } else {
                    swap_memory[frame_num].process_id = -1; // Liberar frame en Swap
                    swap_memory[frame_num].page_num = -1;
                    swap_memory[frame_num].last_access = 0;
                }
            }

            free(curr->page_table);

            if (prev) {
                prev->next = curr->next;
            } else {
                *proc_list = curr->next;
            }

            free(curr);
            printf("Proceso %d finalizado y memoria liberada.\n", pid);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    printf("Proceso %d no encontrado.\n", pid);
}

void asignar_pagina_ram_swap(process *proc, int page_index) {
    int frame_index;
    buscar_frame_libre_ram(&frame_index);

    if (frame_index != -1) {
        ram_memory[frame_index].process_id = proc->pid;
        ram_memory[frame_index].page_num = page_index;
        ram_memory[frame_index].last_access = time(NULL);

        proc->page_table[page_index].present = 1;
        proc->page_table[page_index].frame_number = frame_index;

        printf("Página %d del proceso %d asignada a RAM en el frame %d.\n", page_index, proc->pid, frame_index);
    } else {
        buscar_frame_libre_swap(&frame_index);
        if (frame_index != -1) {
            swap_memory[frame_index].process_id = proc->pid;
            swap_memory[frame_index].page_num = page_index;
            swap_memory[frame_index].last_access = time(NULL);

            proc->page_table[page_index].present = 0;
            proc->page_table[page_index].frame_number = frame_index;

            printf("Página %d del proceso %d asignada a Swap en el frame %d.\n", page_index, proc->pid, frame_index);
        } else {
            printf("No hay espacio disponible en RAM o Swap para la página %d del proceso %d.\n", page_index, proc->pid);
        }
    }
}
void manejar_page_fault(process *proc, int page_index) {
    printf("Page Fault en proceso %d, pagina %d\n", proc->pid, page_index);

    int frame_ram;
    buscar_frame_libre_ram(&frame_ram);

    if (frame_ram == -1) {
        int victima = encontrar_frame_LRU();
        if (victima == -1) {
            printf("No hay paginas en RAM para expulsar (inconsistencia)\n");
            exit(1);
        }

        int pid_victima = ram_memory[victima].process_id;
        int pag_victima = ram_memory[victima].page_num;

        printf("RAM llena → Reemplazo LRU: saco pagina %d del proceso %d (frame %d)\n",
               pag_victima, pid_victima, victima);

        int swap_index;
        buscar_frame_libre_swap(&swap_index);

        if (swap_index == -1) {
            printf("No hay espacio en swap → FIN DEL PROGRAMA\n");
            exit(1);
        }

        swap_memory[swap_index].process_id = pid_victima;
        swap_memory[swap_index].page_num = pag_victima;
        swap_memory[swap_index].last_access = time(NULL);

        process *p = lista_procesos;
        while (p) {
            if (p->pid == pid_victima) {
                if (pag_victima >= 0 && pag_victima < p->num_pages) {
                    p->page_table[pag_victima].present = 0;
                    p->page_table[pag_victima].frame_number = swap_index;
                }
                break;
            }
            p = p->next;
        }

        frame_ram = victima;
    }

    ram_memory[frame_ram].process_id = proc->pid;
    ram_memory[frame_ram].page_num = page_index;
    ram_memory[frame_ram].last_access = time(NULL);

    proc->page_table[page_index].present = 1;
    proc->page_table[page_index].frame_number = frame_ram;

    printf("Pagina %d del proceso %d cargada a RAM en frame %d\n", page_index, proc->pid, frame_ram);
}

void acceso_virtual_aleatorio() {
    if (!lista_procesos) return;

    // Elegir proceso aleatorio
    int count = 0;
    process *tmp = lista_procesos;
    while (tmp) { count++; tmp = tmp->next; }

    int idx = rand() % count;
    process *p = lista_procesos;
    while (idx-- > 0 && p->next) p = p->next;

    if (p->num_pages <= 0) return;

    // Elegir página válida aleatoria
    int page = rand() % p->num_pages;

    // Convertir a dirección virtual válida dentro de esa página
    int addr = page * page_size_kb * 1024;

    printf("\nAccediendo a direccion virtual %d del proceso %d (pagina %d)\n",
           addr, p->pid, page);

    acceder_direccion_virtual(p, addr);
}


void finalizar_proceso_random() {
    if (!lista_procesos) return;

    int count = 0;
    process *tmp = lista_procesos;
    while (tmp) { count++; tmp = tmp->next; }

    int idx = rand() % count;
    process *p = lista_procesos;
    while (idx-- > 0 && p->next) p = p->next;

    printf("\nFinalizando proceso %d...\n", p->pid);
    finalizar_proceso(&lista_procesos, p->pid);
}

// --- NUEVA versión segura de crear_proceso_random() ---
void print_status() {
    int libres_ram = 0, libres_swap = 0;
    for (int i=0;i<total_ram_frames;i++) if (ram_memory[i].process_id == -1) libres_ram++;
    for (int i=0;i<total_swap_frames;i++) if (swap_memory[i].process_id == -1) libres_swap++;
    int proc_cnt = 0; process *tmp = lista_procesos; while(tmp){ proc_cnt++; tmp = tmp->next; }
    printf("[t=%d] procesos=%d, frames_libres_ram=%d/%d, swap_libres=%d/%d\n",
           global_time, proc_cnt, libres_ram, total_ram_frames, libres_swap, total_swap_frames);
}

void crear_proceso_random() {
    static int pid_counter = 1;  // PID autoincremental

    // Limitar número de procesos simultáneos
    int max_procesos = 25;
    int cnt = 0;
    process *t = lista_procesos;
    while (t) { cnt++; t = t->next; }

    if (cnt >= max_procesos) {
        return;
    }

    // Tamaño del proceso entre 200 y 600 páginas
    int paginas = 200 + rand() % 401; // 200..600
    int size_kb = paginas * page_size_kb;

    crear_proceso(&lista_procesos, pid_counter++, size_kb);

    printf("Proceso aleatorio creado. PID=%d, %d paginas (%d KB)\n",
            pid_counter - 1, paginas, size_kb);
}


int main(){

    int ram;
    int page;

    printf("Tamaño RAM (MB): ");
    if (scanf("%d", &ram) != 1) return 0;

    printf("Tamaño pagina (KB): ");
    if (scanf("%d", &page) != 1) return 0;

    init_memory(ram, page);

    while (1) {
        tick();

        if (global_time % 2 == 0) crear_proceso_random();

        if (global_time > 30 && global_time % 5 == 0) {
            finalizar_proceso_random();
            acceso_virtual_aleatorio();
        }

        if (global_time % 5 == 0) print_status();
    }
    return 0;
}
