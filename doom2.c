#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <pthread.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256
#define MAX_PATH 100
#define MAX_MONSTERS 50
#define MAX_HEROES 10

pthread_mutex_t mutex;
pthread_cond_t alertar;

typedef struct { int x, y; } Coord;
typedef struct { int width, height; } Grid;

typedef enum { HEROE_MOVIENDOSE, HEROE_ATACANDO, HEROE_MUERTO, HEROE_TERMINO } EstadoHeroe;

typedef struct {
    int hp;
    int attack_damage;
    int attack_range;
    Coord pos;
    Coord path[MAX_PATH];
    int path_length;
    EstadoHeroe estado;
    int id;
} HeroeInfo;

typedef enum { MONSTRUO_DORMIDO, MONSTRUO_ALERTADO, MONSTRUO_ATACANDO, MONSTRUO_MUERTO } EstadoMonstruo;

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
HeroeInfo heroes[MAX_HEROES] = {0};
int contador_heroes = 0;

MonstruoInfo monsters[MAX_MONSTERS] = {0};
int contador_monstruo = 0;

int distancia(Coord a, Coord b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

//funciones mostruos
void mover_hacia_heroe(MonstruoInfo *m, HeroeInfo *h){
    if (m->pos.x < h->pos.x) m->pos.x++;
    else if (m->pos.x > h->pos.x) m->pos.x--;

    if (m->pos.y < h->pos.y) m->pos.y++;
    else if (m->pos.y > h->pos.y) m->pos.y--;

    printf("Monstruo %d se mueve hacia héroe %d: (%d,%d)\n", m->id, h->id, m->pos.x, m->pos.y);
}

void alertar_otros_monstruos(MonstruoInfo *emisor){
    for(int i = 0; i < contador_monstruo; i++){
        MonstruoInfo *m = &monsters[i];
        if(m->estado == MONSTRUO_DORMIDO){
            int d = distancia(emisor->pos, m->pos);
            if(d <= emisor->vision_range){
                m->estado = MONSTRUO_ALERTADO;
                printf("Monstruo %d alertado por monstruo %d\n", m->id, emisor->id);
            }
        }
    }
    pthread_cond_broadcast(&alertar);
}

void atacar_heroe(MonstruoInfo *m, HeroeInfo *h){
    if(h->hp > 0){
        h->hp -= m->attack_damage;
        printf("Monstruo %d ataca héroe %d con daño %d, HP del héroe: %d\n",
               m->id, h->id, m->attack_damage, h->hp);
        if(h->hp <= 0){
            h->estado = HEROE_MUERTO;
            printf("Héroe %d eliminado\n", h->id);
        }
    }
}

HeroeInfo* heroe_mas_cercano(MonstruoInfo *m) {
    HeroeInfo *h_cercano = NULL;
    int min_dist = 100000;
    for(int i = 0; i < contador_heroes; i++){
        if(heroes[i].estado != HEROE_MUERTO){
            int d = distancia(m->pos, heroes[i].pos);
            if(d < min_dist){
                min_dist = d;
                h_cercano = &heroes[i];
            }
        }
    }
    return h_cercano;
}

void *monstruo(void *arg){
    MonstruoInfo *m = (MonstruoInfo *)arg;

    while (m->estado != MONSTRUO_MUERTO) {
        pthread_mutex_lock(&mutex);

        while (m->estado == MONSTRUO_DORMIDO) {
            pthread_cond_wait(&alertar, &mutex);
            HeroeInfo *h = heroe_mas_cercano(m);
            if(h && distancia(m->pos, h->pos) <= m->vision_range){
                m->estado = MONSTRUO_ALERTADO;
                printf("Monstruo %d detecta héroe %d en (%d,%d)\n", m->id, h->id, h->pos.x, h->pos.y);
            }
        }

        if(m->estado == MONSTRUO_ALERTADO){
            HeroeInfo *h = heroe_mas_cercano(m);
            if(!h) break;

            int dist = distancia(m->pos, h->pos);
            if(dist <= m->attack_range){
                m->estado = MONSTRUO_ATACANDO;
            } else if(dist <= m->vision_range){
                alertar_otros_monstruos(m);
                mover_hacia_heroe(m, h);
            } else {
                mover_hacia_heroe(m, h);
            }
        }

        if(m->estado == MONSTRUO_ATACANDO){
            HeroeInfo *h = heroe_mas_cercano(m);
            if(h) atacar_heroe(m, h);

            if(!h || distancia(m->pos, h->pos) > m->attack_range)
                m->estado = MONSTRUO_ALERTADO;
        }

        pthread_mutex_unlock(&mutex);
        sleep(1);
    }

    pthread_exit(NULL);
}

// funciones heroes
void atacar_monstruos_cercanos(HeroeInfo *h) {
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < contador_monstruo; i++){
        MonstruoInfo *m = &monsters[i];
        if(m->estado != MONSTRUO_MUERTO){
            int d = distancia(h->pos, m->pos);
            if(d <= h->attack_range){
                m->hp -= h->attack_damage;
                if(m->hp <= 0){
                    m->estado = MONSTRUO_MUERTO;
                    printf("Monstruo %d muerto por héroe %d en (%d,%d)\n",
                           m->id, h->id, h->pos.x, h->pos.y);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

int hay_monstruos_en_rango(HeroeInfo *h){
    for(int i = 0; i < contador_monstruo; i++){
        MonstruoInfo *m = &monsters[i];
        if(m->estado != MONSTRUO_MUERTO){
            int d = distancia(h->pos, m->pos);
            if(d <= h->attack_range) return 1;
        }
    }
    return 0;
}

void *heroes_thread(void *arg){
    HeroeInfo *h = (HeroeInfo *)arg;
    h->estado = HEROE_MOVIENDOSE;

    
    while(hay_monstruos_en_rango(h) && h->estado != HEROE_MUERTO){
        h->estado = HEROE_ATACANDO;
        printf("Héroe %d atacando en POS INICIAL (%d,%d)\n", h->id, h->pos.x, h->pos.y);
        atacar_monstruos_cercanos(h);
        pthread_cond_broadcast(&alertar);
        sleep(1);
    }

  
    for(int paso = 1; paso < h->path_length && h->estado != HEROE_MUERTO; paso++){
        
        h->estado = HEROE_MOVIENDOSE;
        pthread_mutex_lock(&mutex);
        h->pos = h->path[paso];
        printf("Héroe %d se mueve a (%d,%d)\n", h->id, h->pos.x, h->pos.y);
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&alertar); 
        
        sleep(1); 

        
        while(hay_monstruos_en_rango(h) && h->estado != HEROE_MUERTO){
            h->estado = HEROE_ATACANDO;
            printf("Héroe %d atacando en (%d,%d)\n", h->id, h->pos.x, h->pos.y);
            atacar_monstruos_cercanos(h);
            pthread_cond_broadcast(&alertar);
            sleep(1);
        }
    }

    // 3. Finalizar el hilo
    if (h->estado != HEROE_MUERTO){
        printf("Héroe %d llegó a la meta en (%d,%d)\n", h->id, h->pos.x, h->pos.y);
        
        pthread_mutex_lock(&mutex); 
        h->estado = HEROE_TERMINO; 
        pthread_mutex_unlock(&mutex);

    } else {
        printf("Héroe %d ha muerto.\n", h->id);
    }
    
    pthread_exit(NULL);
}

int contar_heroes_vivos() {
    int vivos = 0;
    pthread_mutex_lock(&mutex); 
    for (int i = 0; i < contador_heroes; i++) {
        if (heroes[i].estado != HEROE_MUERTO) {
            vivos++;
        }
    }
    pthread_mutex_unlock(&mutex); 
    return vivos;
}

int contar_monstruos_vivos() {
    int vivos = 0;
    pthread_mutex_lock(&mutex); 
    for (int i = 0; i < contador_monstruo; i++) {
        if (monsters[i].estado != MONSTRUO_MUERTO) {
            vivos++;
        }
    }
    pthread_mutex_unlock(&mutex); 
    return vivos;
}

int todos_heroes_vivos_terminaron() {
    pthread_mutex_lock(&mutex);
    int heroes_jugando = 0;
    for (int i = 0; i < contador_heroes; i++) {

        if (heroes[i].estado == HEROE_MOVIENDOSE || heroes[i].estado == HEROE_ATACANDO) {
            heroes_jugando++;
            break; 
        }
    }
    pthread_mutex_unlock(&mutex);
    
    
    return (heroes_jugando == 0);
}

// leer archivo configuracion
int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Uso: %s <archivo_configuracion>\n", argv[0]);
        return 1;
    }

    char *config_filename = argv[1];
    FILE *configFile = fopen(config_filename, "r");
    if(!configFile){ perror("error al abrir el archivo"); return 1; }

    printf("Archivo de configuración '%s' abierto.\n\n", config_filename);

    char buffer[MAX_LINE_LENGTH];
    int leyendo_path = 0;
    int path_heroe_actual = -1;

    while(fgets(buffer, MAX_LINE_LENGTH, configFile)){
        int id, val1, val2;

        if(sscanf(buffer, "GRID_SIZE %d %d", &grid.width, &grid.height) == 2){ 
            printf("GRID_SIZE: %d x %d\n", grid.width, grid.height); continue; 
        }

        // Leer héroes
        for(int i=0;i<MAX_HEROES;i++){
            char tag[64];

            sprintf(tag, "HERO_%d_HP", i+1);
            if(strncmp(buffer, tag, strlen(tag)) == 0) {
                sscanf(buffer + strlen(tag), " %d", &val1);
                heroes[i].hp = val1;
                heroes[i].id = i+1;
                if(i+1 > contador_heroes) contador_heroes = i+1;
                continue;
            }

            sprintf(tag, "HERO_%d_ATTACK_DAMAGE", i+1);
            if(strncmp(buffer, tag, strlen(tag)) == 0) {
                sscanf(buffer + strlen(tag), " %d", &val1);
                heroes[i].attack_damage = val1;
                continue;
            }

            sprintf(tag, "HERO_%d_ATTACK_RANGE", i+1);
            if(strncmp(buffer, tag, strlen(tag)) == 0) {
                sscanf(buffer + strlen(tag), " %d", &val1);
                heroes[i].attack_range = val1;
                continue;
            }

            sprintf(tag, "HERO_%d_START", i+1);
            if(strncmp(buffer, tag, strlen(tag)) == 0) {
                sscanf(buffer + strlen(tag), " %d %d", &val1, &val2);
                heroes[i].pos.x = val1;
                heroes[i].pos.y = val2;
                heroes[i].path[0] = heroes[i].pos;
                heroes[i].path_length = 1;
                continue;
            }

            sprintf(tag, "HERO_%d_PATH", i+1);
            if(strncmp(buffer, tag, strlen(tag)) == 0){
                leyendo_path = 1;
                path_heroe_actual = i;
                heroes[i].path_length = 0;

                char *token = strtok(buffer + strlen(tag), " ");
                while(token != NULL){
                    if(strchr(token,'(')){
                        int x,y;
                        sscanf(token,"(%d,%d)",&x,&y);
                        heroes[i].path[heroes[i].path_length].x = x;
                        heroes[i].path[heroes[i].path_length].y = y;
                        heroes[i].path_length++;
                    }
                    token = strtok(NULL," ");
                }
                continue;
            }
        }

        // Continuar leyendo PATH en líneas siguientes
        if(leyendo_path && path_heroe_actual>=0){
            if(strchr(buffer,'(')){
                char *token = strtok(buffer," ");
                while(token != NULL){
                    if(strchr(token,'(')){
                        int x,y;
                        sscanf(token,"(%d,%d)",&x,&y);
                        heroes[path_heroe_actual].path[heroes[path_heroe_actual].path_length].x = x;
                        heroes[path_heroe_actual].path[heroes[path_heroe_actual].path_length].y = y;
                        heroes[path_heroe_actual].path_length++;
                    }
                    token = strtok(NULL," ");
                }
            } else {
                leyendo_path = 0;
                path_heroe_actual = -1;
            }
        }

        // MONSTRUOS
        if(sscanf(buffer, "MONSTER_COUNT %d", &contador_monstruo)==1){ 
            if(contador_monstruo>MAX_MONSTERS) contador_monstruo=MAX_MONSTERS;
            continue; 
        }
        if(sscanf(buffer, "MONSTER_%d_HP %d", &id, &val1)==2){ if(id>=1 && id<=MAX_MONSTERS) monsters[id-1].hp=val1; continue; }
        if(sscanf(buffer, "MONSTER_%d_ATTACK_DAMAGE %d", &id, &val1)==2){ if(id>=1 && id<=MAX_MONSTERS) monsters[id-1].attack_damage=val1; continue; }
        if(sscanf(buffer, "MONSTER_%d_VISION_RANGE %d", &id, &val1)==2){ if(id>=1 && id<=MAX_MONSTERS) monsters[id-1].vision_range=val1; continue; }
        if(sscanf(buffer, "MONSTER_%d_ATTACK_RANGE %d", &id, &val1)==2){ if(id>=1 && id<=MAX_MONSTERS) monsters[id-1].attack_range=val1; continue; }
        if(sscanf(buffer, "MONSTER_%d_COORDS %d %d", &id, &val1, &val2)==3){ if(id>=1 && id<=MAX_MONSTERS){ monsters[id-1].pos.x=val1; monsters[id-1].pos.y=val2; } continue; }

    }

    fclose(configFile);

    printf("\n===== CONFIGURACIÓN CARGADA =====\n");
    printf("GRID: %d x %d\n", grid.width, grid.height);
    for(int i=0;i<contador_heroes;i++){
        printf("Héroe %d -> HP=%d, ATTACK=%d, RANGE=%d, START=(%d,%d), PATH_LENGTH=%d\n",
               i+1, heroes[i].hp, heroes[i].attack_damage, heroes[i].attack_range,
               heroes[i].pos.x, heroes[i].pos.y, heroes[i].path_length);
    }
    printf("MONSTRUOS: %d\n", contador_monstruo);
    for(int i=0;i<contador_monstruo;i++){
        printf("M%d -> HP=%d, ATTACK=%d, VISION=%d, RANGE=%d, COORDS=(%d,%d)\n",
               i+1, monsters[i].hp, monsters[i].attack_damage,
               monsters[i].vision_range, monsters[i].attack_range,
               monsters[i].pos.x, monsters[i].pos.y);
    }

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&alertar,NULL);

    // Crear hilos de héroes
    pthread_t hilos_heroes[MAX_HEROES];
    for(int i=0;i<contador_heroes;i++){
        pthread_create(&hilos_heroes[i], NULL, heroes_thread, &heroes[i]);
    }

    // Crear hilos de monstruos
    pthread_t hilos_monstruos[MAX_MONSTERS];
    for(int i=0;i<contador_monstruo;i++){
        monsters[i].id=i+1;
        monsters[i].estado=MONSTRUO_DORMIDO;
        pthread_create(&hilos_monstruos[i], NULL, monstruo, &monsters[i]);
    }

    pthread_cond_broadcast(&alertar);

   while (1) {
        sleep(1); 

        int heroes_vivos = contar_heroes_vivos();
        int monstruos_vivos = contar_monstruos_vivos();

      
        if (heroes_vivos == 0) {
            printf("\n===================================\n");
            printf("TODOS LOS HEROES HAN MUERTO, Los monstruos ganan.\n");
            printf("===================================\n");
            exit(1); 
        }

        
        if (monstruos_vivos == 0) {
            printf("\n===================================\n");
            printf("TODOS LOS MONSTRUOS HAN SIDO ELIMINADOS Los heroes ganan.\n");
            printf("===================================\n");
            exit(0);
        }

      
        if (todos_heroes_vivos_terminaron()) {
            printf("\n===================================\n");
            printf("TODOS LOS HEROES VIVOS LLEGARON A LA META, Los heroes ganan.\n");
            printf("===================================\n");
            exit(0); 
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&alertar);

    return 0;
}
