#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int frame_id;      // Número de marco físico
    int process_id;    // Qué proceso lo está usando (-1 si está libre)
    int page_num;      // Qué página lógica del proceso es
    int last_access;   // Timestamp para algoritmo LRU 
} frame;

typedef struct {
    int page_id;
    int present;       // 1 = en RAM, 0 = en Swap / no cargada
    int frame_number;  // índice en ram_memory (si present=1) o en swap_memory (si present=0 y está en swap)
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

    float factor = 1.5f + ((float)rand() / RAND_MAX) *(4.5f - 1.5f);
    virtual_memory_mb = (int)(ram_size_mb * factor);

    total_virtual_pages = (virtual_memory_mb * 1024) / page_size_kb;

    total_ram_frames = (ram_size_mb * 1024) / page_size_kb;
    total_swap_frames = total_ram_frames;

    ram_memory = malloc(total_ram_frames * sizeof(frame));
    swap_memory = malloc(total_swap_frames * sizeof(frame));

    if (!ram_memory || !swap_memory) {
        printf("No se pudo asignar memoria.\n");
        exit(1);
    }

// Inicializar arrays para marcar como libre

    for (int i = 0; i < total_ram_frames; i++) {
        ram_memory[i].frame_id = i;
        ram_memory[i].process_id = -1;
        ram_memory[i].page_num = -1;
        ram_memory[i].last_access = 0;
    }

    for (int i = 0; i < total_swap_frames; i++) {
        swap_memory[i].frame_id = i;
        swap_memory[i].process_id = -1;
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

process *lista_procesos = NULL;
int global_time = 0;
int next_pid = 1;

void tick() {
    sleep(1);
    global_time++;
}

int encontrar_frame_LRU() {
    int victima = -1;
    for (int i = 0; i < total_ram_frames; i++) {
        if (ram_memory[i].process_id != -1) {
            if (victima == -1 || ram_memory[i].last_access < ram_memory[victima].last_access) {
                victima = i;
            }
        }
    }
    return victima;
}

void manejar_page_fault(process *proc, int page_index);




void asignar_pagina_ram_swap(process *proc, int page_index) {
    int frame_index;
    buscar_frame_libre_ram(&frame_index);

    if (frame_index != -1) {
        ram_memory[frame_index].process_id = proc->pid;
        ram_memory[frame_index].page_num = page_index;
        ram_memory[frame_index].last_access = time(NULL);

        proc->page_table[page_index].present = 1;
        proc->page_table[page_index].frame_number = frame_index;

        printf("Pagina %d del proceso %d asignada a RAM en frame %d.\n", page_index, proc->pid, frame_index);
    } else {
        buscar_frame_libre_swap(&frame_index);
        if (frame_index != -1) {
            swap_memory[frame_index].process_id = proc->pid;
            swap_memory[frame_index].page_num = page_index;
            swap_memory[frame_index].last_access = time(NULL);

            proc->page_table[page_index].present = 0;
            proc->page_table[page_index].frame_number = frame_index;

            printf("Pagina %d del proceso %d asignada a Swap en el frame %d.\n", page_index, proc->pid, frame_index);
        } else {
            printf("No hay espacio disponible en RAM o Swap para la pagina %d del proceso %d.\n", page_index, proc->pid);
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

    int count = 0;
    process *tmp = lista_procesos;
    while (tmp) { count++; tmp = tmp->next; }

    int idx = rand() % count;
    process *p = lista_procesos;
    while (idx-- > 0 && p->next) {
        p = p->next;
    }

    int page_index = rand() % p->num_pages;

    if (p->page_table[page_index].present == 1) {
        int frame_num = p->page_table[page_index].frame_number;
        ram_memory[frame_num].last_access = time(NULL);
        printf("Acceso a pagina %d del proceso %d en frame %d (RAM)\n", page_index, p->pid, frame_num);
    } else {
        manejar_page_fault(p, page_index);
    }
}

void crear_proceso_random() {
    int size_kb = (rand() % 512) + 64; // Tamaño entre 64KB y 575KB
    int num_pages = (size_kb + page_size_kb - 1) / page_size_kb;

    process *new_proc = malloc(sizeof(process));
    if (!new_proc) {
        printf("No se pudo asignar memoria para nuevo proceso.\n");
        return;
    }

    new_proc->pid = next_pid++;
    new_proc->size = size_kb;
    new_proc->num_pages = num_pages;
    new_proc->page_table = malloc(num_pages * sizeof(page));
    new_proc->next = NULL;

    if (!new_proc->page_table) {
        printf("No se pudo asignar memoria para tabla de paginas del proceso %d.\n", new_proc->pid);
        free(new_proc);
        return;
    }

    for (int i = 0; i < num_pages; i++) {
        new_proc->page_table[i].page_id = i;
        new_proc->page_table[i].present = 0;
        new_proc->page_table[i].frame_number = -1;
    }

    new_proc->next = lista_procesos;
    lista_procesos = new_proc;

    printf("\nNuevo proceso creado: PID=%d, tamaño=%dKB, páginas=%d\n",
           new_proc->pid, new_proc->size, new_proc->num_pages);
}

void finalizar_proceso_random() {
    if (!lista_procesos) return;

    int count = 0;
    process *tmp = lista_procesos;
    while (tmp) { count++; tmp = tmp->next; }

    int idx = rand() % count;
    process *p = lista_procesos;
    process *prev = NULL;
    while (idx-- > 0 && p->next) {
        prev = p;
        p = p->next;
    }

    // Liberar marcos en RAM
    for (int i = 0; i < total_ram_frames; i++) {
        if (ram_memory[i].process_id == p->pid) {
            ram_memory[i].process_id = -1;
            ram_memory[i].page_num = -1;
            ram_memory[i].last_access = 0;
        }
    }

    // Liberar marcos en Swap
    for (int i = 0; i < total_swap_frames; i++) {
        if (swap_memory[i].process_id == p->pid) {
            swap_memory[i].process_id = -1;
            swap_memory[i].page_num = -1;
            swap_memory[i].last_access = 0;
        }
    }

    // Eliminar proceso de la lista
    if (prev) {
        prev->next = p->next;
    } else {
        lista_procesos = p->next;
    }

    free(p->page_table);
    free(p);

    printf("\nProceso %d finalizado y recursos liberados.\n", p->pid);
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
            
            for (int i = 0; i < 1000; i++) {
                acceso_virtual_aleatorio();
            }
        }

        if (global_time % 5 == 0) print_status();
    }
    return 0;
}
