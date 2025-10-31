#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 


#define MAX_LINE_LENGTH 256
#define MAX_PATH 100
#define MAX_MONSTERS 50

pthread_mutex_t mutex;
pthread_cond_t alertar;

Grid grid = {0};
HeroeInfo hero = {0};
MonstruoInfo monsters[MAX_MONSTERS] = {0};
int contador_monstruo = 0;

typedef struct {
    int x, y;
} Coord;

typedef struct {
    int width, height;
} Grid;

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

void distancia (Pos a, Pos b){
    return abs(a.x - b.x) + abs(a.y - b.y)

}

void mover_hacia_heroe(MounstruoIndo *m){
    pthread_mutex_lock(&mutex);
    if (m->pos.x < heroe.pos.x){
        m->pos.x++;
    } 
    else if (m->pos.x > heroe.pos.x){
       m->pos.x--; 
    } 
    if (m->pos.y < heroe.pos.y){
        m->pos.y++;
    } 
    else if (m->pos.y > heroe.pos.y){
       m->pos.y--; 
    } 
    pthread_mutex_unlock(&mutex);
    printf("Monstruo %d se mueve hacia (%d,%d) \n", m->id, m->pos.x, m->pos.y);

}

void alertar_otro_monstruo(MonstruoInfo *emisor){

    pthread_mutex_lock(&mutex);

    for(int i = 0, i< contador_monstruo, i++){
        MonstruoInfo *m = &monsters[i];

        if(m-> estado = MONSTRUO_DORMIDO){
            int d = distancia(emisor->pos, m->pos);

            if(d <= emisor->vision_range){
                m->estado = MONSTRUO_ALERTADO;

                printf("Monstruo %d alertado \n", m->id)
            }
        }


    }

    pthread_cond_broadcast(&alertar);
    phtread_mutex_unlock(&mutex);


}

void atacar_heroe(MonstruoInfo *m, HeroeInfo *h){

    pthread_mutex_lock(&mutex);
    if(h->hp>0){
        h->hp -= m->attack_damage;
        printf("Mounstruo %d ataca con un daño de %d, HP del heroe: %d\n", m->id, m->attack_damage, h->hp);
    }
    pthread_mutex_unlock(&mutex);

    if(h->hp<0){
        h->estado= HEROE_MUERTO;
        printf("Heroe eliminado")
    }

}

void *monstruo(monstruo *arg){
    MonstruoInfo *m = (MonstruoInfo *)arg;

     while (m->estado != MONSTRUO_MUERTO && heroe.estado != HEROE_MUERTO) {
        pthread_mutex_lock(&mutex);
        while (m->estado == MONSTRUO_DORMIDO)
            pthread_cond_wait(&cond_alerta, &mutex);
        pthread_mutex_unlock(&mutex);

        if (m->estado == MONSTRUO_ALERTADO) {
            int dist = distancia(m->pos, heroe.pos);
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
            atacar_heroe(m);
            if (distancia(m->pos, heroe.pos) > m->attack_range)
                m->estado = MONSTRUO_ALERTADO;
        }

        sleep(1);
    }


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
    char buffer[MAX_LINE_LENGTH]; // Un buffer para guardar cada línea

    // fgets lee una linea (o MAX_LINE_LENGTH-1 caracteres) del archivo
    // y la guarda en 'buffer'. Retorna NULL cuando llega al final del archivo.
    while (fgets(buffer, MAX_LINE_LENGTH, configFile) != NULL) {
        
        
     // GRID_SIZE
        if (sscanf(buffer, "GRID_SIZE %d %d", &grid.width, &grid.height) == 2) {
            printf("GRID_SIZE: %d x %d\n", grid.width, grid.height);
            continue;
        }

        // HERO_HP
        if (sscanf(buffer, "HERO_HP %d", &hero.hp) == 1) {
            printf("HERO_HP: %d\n", hero.hp);
            continue;
        }

        // HERO_ATTACK_DAMAGE
        if (sscanf(buffer, "HERO_ATTACK_DAMAGE %d", &hero.attack_damage) == 1) {
            printf("HERO_ATTACK_DAMAGE: %d\n", hero.attack_damage);
            continue;
        }

        // HERO_ATTACK_RANGE
        if (sscanf(buffer, "HERO_ATTACK_RANGE %d", &hero.attack_range) == 1) {
            printf("HERO_ATTACK_RANGE: %d\n", hero.attack_range);
            continue;
        }

        // HERO_START
        if (sscanf(buffer, "HERO_START %d %d", &hero.start.x, &hero.start.y) == 2) {
            printf("HERO_START: (%d, %d)\n", hero.start.x, hero.start.y);
            continue;
        }

        // HERO_PATH
        if (strncmp(buffer, "HERO_PATH", 9) == 0) {
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

        // CONTADOR_MONSTRUO
        if (sscanf(buffer, "MONSTER_COUNT %d", &contador_monstruo) == 1) {
            printf("MONSTER_COUNT: %d\n", contador_monstruo);
            continue;
        }

        // MONSTRUOS
        int id;
        if (sscanf(buffer, "MONSTER_%d_HP %d", &id, &monsters[id-1].hp) == 2) continue;
        if (sscanf(buffer, "MONSTER_%d_ATTACK_DAMAGE %d", &id, &monsters[id-1].attack_damage) == 2) continue;
        if (sscanf(buffer, "MONSTER_%d_VISION_RANGE %d", &id, &monsters[id-1].vision_range) == 2) continue;
        if (sscanf(buffer, "MONSTER_%d_ATTACK_RANGE %d", &id, &monsters[id-1].attack_range) == 2) continue;
        if (sscanf(buffer, "MONSTER_%d_COORDS %d %d", &id, &monsters[id-1].start.x, &monsters[id-1].start.y) == 3) continue;

    }

    // --- 4. Cerrar el archivo ---
    fclose(configFile);

     printf("\n===== CONFIGURACIÓN CARGADA =====\n");
    printf("GRID: %d x %d\n", grid.width, grid.height);
    printf("HERO -> HP=%d, ATTACK=%d, RANGE=%d, START=(%d,%d)\n",
           hero.hp, hero.attack_damage, hero.attack_range,
           hero.start.x, hero.start.y);
    printf("Ruta del héroe: %d pasos\n", hero.path_length);

    printf("MONSTRUOS: %d\n", contador_monstruo);
    for (int i = 0; i < contador_monstruo; i++) {
        printf("M%d -> HP=%d, ATTACK=%d, VISION=%d, RANGE=%d, COORDS=(%d,%d)\n",
               i+1,
               monsters[i].hp,
               monsters[i].attack_damage,
               monsters[i].vision_range,
               monsters[i].attack_range,
               monsters[i].start.x,
               monsters[i].start.y);
    }


    return 0; 
}
