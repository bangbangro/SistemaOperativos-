#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h>
#include <unistd.h>


#define MAX_LINE_LENGTH 256
#define MAX_PATH 100
#define MAX_MONSTERS 50

pthread_mutex_t mutex;
pthread_cond_t alertar;

typedef struct { int x, y; } Coord;

typedef struct { int width, height; } Grid;

typedef enum {
    HEROE_MOVIENDOSE,
    HEROE_ATACANDO,
    HEROE_MUERTO
} EstadoHeroe;


typedef struct {
    int hp;
    int attack_damage;
    int attack_range;
    Coord pos;
    Coord path[MAX_PATH];
    int path_length;
    EstadoHeroe estado;
} HeroeInfo;

typedef enum {
    MONSTRUO_DORMIDO,
    MONSTRUO_ALERTADO,
    MONSTRUO_ATACANDO,
    MONSTRUO_MUERTO
} EstadoMonstruo;

typedef struct {
    int id;
    int hp;
    int attack_damage;
    int vision_range;
    int attack_range;
    Coord pos;
    EstadoMonstruo estado;
} MonstruoInfo;

Grid grid = {0};
HeroeInfo hero = {0};
MonstruoInfo monsters[MAX_MONSTERS] = {0};
int contador_monstruo = 0;

int distancia(Coord a, Coord b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

//funciones de los monstruos

void mover_hacia_heroe(MonstruoInfo *m){
    if (m->pos.x < hero.pos.x){
        m->pos.x++;
    } 
    else if (m->pos.x > hero.pos.x){
       m->pos.x--; 
    } 
    if (m->pos.y < hero.pos.y){
        m->pos.y++;
    } 
    else if (m->pos.y > hero.pos.y){
       m->pos.y--; 
    } 
    printf("Monstruo %d se mueve hacia (%d,%d) \n", m->id, m->pos.x, m->pos.y);

}

void alertar_otros_monstruos(MonstruoInfo *emisor){

    for(int i = 0; i < contador_monstruo; i++){
        MonstruoInfo *m = &monsters[i];

        if(m-> estado == MONSTRUO_DORMIDO){
            int d = distancia(emisor->pos, m->pos);

            if(d <= emisor->vision_range){
                m->estado = MONSTRUO_ALERTADO;
                printf("Monstruo %d alertado \n", m->id);
            }
        }

    }
    pthread_cond_broadcast(&alertar);
}

void atacar_heroe(MonstruoInfo *m, HeroeInfo *h){

   
    if(h->hp>0){
        h->hp -= m->attack_damage;
        printf("Mounstruo %d ataca con un daño de %d, HP del heroe: %d\n", m->id, m->attack_damage, h->hp);
    }
    

    if(h->hp<0){
        h->estado= HEROE_MUERTO;
        printf("Heroe eliminado\n");
    }

}

void *monstruo(void *arg){
    MonstruoInfo *m = (MonstruoInfo *)arg;

    while (m->estado != MONSTRUO_MUERTO && hero.estado != HEROE_MUERTO) {
        pthread_mutex_lock(&mutex);

       
        while (m->estado == MONSTRUO_DORMIDO) {
            pthread_cond_wait(&alertar, &mutex);

            
            if (hero.estado != HEROE_MUERTO) {
                int dist = distancia(m->pos, hero.pos);
                if (dist <= m->vision_range) {
                    m->estado = MONSTRUO_ALERTADO;
                    printf("Monstruo %d detecta al héroe en (%d,%d)!\n", m->id, hero.pos.x, hero.pos.y);
                    
                }
            }
            
        }
        

        if (m->estado == MONSTRUO_ALERTADO) {
            int dist = distancia(m->pos, hero.pos);
            if (dist <= m->attack_range) {
                m->estado = MONSTRUO_ATACANDO;
            } else if (dist <= m->vision_range) {
                alertar_otros_monstruos(m); 
                mover_hacia_heroe(m);
            } else {
                
                mover_hacia_heroe(m); 
            }
        }

        if (m->estado == MONSTRUO_ATACANDO) {
            atacar_heroe(m, &hero);
            
            if (distancia(m->pos, hero.pos) > m->attack_range)
                m->estado = MONSTRUO_ALERTADO; 
        }
         pthread_mutex_unlock(&mutex); 
         sleep(1);
    }
    pthread_exit(NULL);
}
//funciones heroes

void atacar_monstruos_cercanos(HeroeInfo *h) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < contador_monstruo; i++) {
        MonstruoInfo *m = &monsters[i];
        if (m->estado != MONSTRUO_MUERTO) {
            int d = abs(h->pos.x - m->pos.x) + abs(h->pos.y - m->pos.y);
            if (d <= h->attack_range) {
                m->hp -= h->attack_damage;
                if (m->hp <= 0) {
                    m->estado = MONSTRUO_MUERTO;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

int hay_monstruos_en_rango(HeroeInfo *h) {
    for (int i = 0; i < contador_monstruo; i++) {
        MonstruoInfo *m = &monsters[i];
        if (m->estado != MONSTRUO_MUERTO) {
            int d = abs(h->pos.x - m->pos.x) + abs(h->pos.y - m->pos.y);
            if (d <= h->attack_range) return 1;
        }
    }
    return 0;
}

void *heroes(void *arg) {
    HeroeInfo *h = (HeroeInfo *)arg;
    h->estado = HEROE_MOVIENDOSE;

    
    while (hay_monstruos_en_rango(h) && h->estado != HEROE_MUERTO) {
        h->estado = HEROE_ATACANDO;
        printf("Héroe ataca en POSICIÓN INICIAL (%d,%d)\n", h->pos.x, h->pos.y);
        atacar_monstruos_cercanos(h);
        sleep(1);
    }
   
    //para atacar a monstruos en rango
    for (int paso = 1; paso < h->path_length && h->estado != HEROE_MUERTO; paso++) {
        while (hay_monstruos_en_rango(h) && h->estado != HEROE_MUERTO) {
            h->estado = HEROE_ATACANDO;
            printf("Héroe ataca en (%d,%d)\n", h->pos.x, h->pos.y); 
            atacar_monstruos_cercanos(h);
            sleep(1);
        }

        // Salir si murió mientras atacaba
        if (h->estado == HEROE_MUERTO) break;

    //seguir moviendose
        h->estado = HEROE_MOVIENDOSE;
        pthread_mutex_lock(&mutex);
        h->pos = h->path[paso];
        printf("Héroe se mueve a (%d,%d)\n", h->pos.x, h->pos.y); 
        pthread_mutex_unlock(&mutex);

        
        pthread_cond_broadcast(&alertar);
        

        sleep(1); 
    }

    
    while (h->estado != HEROE_MUERTO && hay_monstruos_en_rango(h)) {
        h->estado = HEROE_ATACANDO;
        printf("Héroe en posición final (%d,%d), atacando.\n", h->pos.x, h->pos.y);
        atacar_monstruos_cercanos(h);
        pthread_cond_broadcast(&alertar); 
        sleep(1);
    }
    

    if (h->estado != HEROE_MUERTO)
        printf("heroe gano (%d,%d)\n", h->pos.x, h->pos.y);
    else
        printf("heroe ha muerto\n");

    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    
    // --- 1. verificar que el usuario paso el archivo de configuración ---
    if (argc < 2) {
     
        fprintf(stderr, "error: se debe especificar la ruta al archivo de configuracion.\n");
        fprintf(stderr, "uso: %s <archivo_configuracion>\n", argv[0]);
        return 1; 
    }

    char *config_filename = argv[1];

    // --- 2. abrir el archivo ---
    FILE *configFile = fopen(config_filename, "r"); // "r" = modo lectura (read)

    
    if (configFile == NULL) {
        perror("error al abrir el archivo de configuracion");
        return 1;
    }

    printf("archivo de configuracion '%s' abierto exitosamente.\n\n", config_filename);

// --- 3. leer el archivo linea por linea ---
char buffer[MAX_LINE_LENGTH];
int leyendo_path = 0;

while (fgets(buffer, MAX_LINE_LENGTH, configFile) != NULL) {
 
    if (strncmp(buffer, "HERO_PATH", 9) == 0) {
        leyendo_path = 1;
        hero.path_length = 0;

        char *token = strtok(buffer, " ");
        while (token != NULL) {
            if (strchr(token, '(')) {
                int x, y;
                sscanf(token, "(%d,%d)", &x, &y);
                hero.path[hero.path_length].x = x;
                hero.path[hero.path_length].y = y;
                hero.path_length++;
            }
            token = strtok(NULL, " ");
        }
        printf("HERO_PATH con %d pasos cargado.\n", hero.path_length);
        continue;
    }

    if (leyendo_path) {
        if (strchr(buffer, '(')) {
            char *token = strtok(buffer, " ");
            while (token != NULL) {
                if (strchr(token, '(')) {
                    int x, y;
                    sscanf(token, "(%d,%d)", &x, &y);
                    hero.path[hero.path_length].x = x;
                    hero.path[hero.path_length].y = y;
                    hero.path_length++;
                }
                token = strtok(NULL, " ");
            }
            printf("HERO_PATH pasos totales: %d.\n", hero.path_length);
            continue;
        } else {
            leyendo_path = 0; 
        }
    }

    int id, val1, val2;

    // GRID_SIZE
    if (sscanf(buffer, "GRID_SIZE %d %d", &grid.width, &grid.height) == 2) {
        printf("GRID_SIZE: %d x %d\n", grid.width, grid.height);
        continue;
    }

    // HERO
    if (sscanf(buffer, "HERO_HP %d", &hero.hp) == 1) { printf("HERO_HP: %d\n", hero.hp); continue; }
    if (sscanf(buffer, "HERO_ATTACK_DAMAGE %d", &hero.attack_damage) == 1) { printf("HERO_ATTACK_DAMAGE: %d\n", hero.attack_damage); continue; }
    if (sscanf(buffer, "HERO_ATTACK_RANGE %d", &hero.attack_range) == 1) { printf("HERO_ATTACK_RANGE: %d\n", hero.attack_range); continue; }
    if (sscanf(buffer, "HERO_START %d %d", &hero.pos.x, &hero.pos.y) == 2) { printf("HERO_START: (%d,%d)\n", hero.pos.x, hero.pos.y); continue; }

    // MONSTER_COUNT
    if (sscanf(buffer, "MONSTER_COUNT %d", &contador_monstruo) == 1) {
        if (contador_monstruo > MAX_MONSTERS) contador_monstruo = MAX_MONSTERS;
        printf("MONSTER_COUNT: %d\n", contador_monstruo);
        continue;
    }

    // MONSTRUOS
    if (sscanf(buffer, "MONSTER_%d_HP %d", &id, &val1) == 2) {
        if (id >= 1 && id <= MAX_MONSTERS) monsters[id-1].hp = val1;
        if (id > contador_monstruo) contador_monstruo = id;
        continue;
    }

    if (sscanf(buffer, "MONSTER_%d_ATTACK_DAMAGE %d", &id, &val1) == 2) {
        if (id >= 1 && id <= MAX_MONSTERS) monsters[id-1].attack_damage = val1;
        continue;
    }

    if (sscanf(buffer, "MONSTER_%d_VISION_RANGE %d", &id, &val1) == 2) {
        if (id >= 1 && id <= MAX_MONSTERS) monsters[id-1].vision_range = val1;
        continue;
    }

    if (sscanf(buffer, "MONSTER_%d_ATTACK_RANGE %d", &id, &val1) == 2) {
        if (id >= 1 && id <= MAX_MONSTERS) monsters[id-1].attack_range = val1;
        continue;
    }

    if (sscanf(buffer, "MONSTER_%d_COORDS %d %d", &id, &val1, &val2) == 3) {
        if (id >= 1 && id <= MAX_MONSTERS) {
            monsters[id-1].pos.x = val1;
            monsters[id-1].pos.y = val2;
        }
        continue;
    }
}


    // --- 4. Cerrar el archivo ---
    fclose(configFile);

     printf("\n===== CONFIGURACIÓN CARGADA =====\n");
    printf("GRID: %d x %d\n", grid.width, grid.height);
    printf("HERO -> HP=%d, ATTACK=%d, RANGE=%d, START=(%d,%d)\n",
           hero.hp, hero.attack_damage, hero.attack_range,
           hero.pos.x, hero.pos.y);
    printf("Ruta del héroe: %d pasos\n", hero.path_length);

    printf("MONSTRUOS: %d\n", contador_monstruo);
    for (int i = 0; i < contador_monstruo; i++) {
        printf("M%d -> HP=%d, ATTACK=%d, VISION=%d, RANGE=%d, COORDS=(%d,%d)\n",
               i+1,
               monsters[i].hp,
               monsters[i].attack_damage,
               monsters[i].vision_range,
               monsters[i].attack_range,
               monsters[i].pos.x,
               monsters[i].pos.y);
    }


pthread_t hilo_heroe;
pthread_t hilos_monstruos[MAX_MONSTERS];

pthread_mutex_init(&mutex, NULL);
pthread_cond_init(&alertar, NULL);

// thread del héroe
pthread_create(&hilo_heroe, NULL, heroes, &hero);

// thread de monstruos
for (int i = 0; i < contador_monstruo; i++) {
    monsters[i].id = i + 1;
    monsters[i].estado = MONSTRUO_DORMIDO;
    pthread_create(&hilos_monstruos[i], NULL, monstruo, &monsters[i]);
}

printf("Monstruos iniciales alertados:\n");
for (int i = 0; i < contador_monstruo; i++) {
    if (monsters[i].estado == MONSTRUO_ALERTADO) {
        printf(" - Monstruo %d dentro del rango de visión inicial.\n", monsters[i].id);
    }
}

pthread_cond_broadcast(&alertar);

// Esperar que todos terminen
pthread_join(hilo_heroe, NULL);
for (int i = 0; i < contador_monstruo; i++) {
    pthread_join(hilos_monstruos[i], NULL);
}

pthread_mutex_destroy(&mutex);
pthread_cond_destroy(&alertar);


    return 0; 
}
