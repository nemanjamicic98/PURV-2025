#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/ioctl.h>
#include <stdint.h>

#define GOL_N     10
#define GOL_CELLS (GOL_N * GOL_N)

typedef struct __attribute__((packed)) {
    uint32_t n;                    /* mora biti 10 */
    uint8_t  cells[GOL_CELLS];     /* 0/1, row-wise */
} FRAME;

typedef struct __attribute__((packed)) {
    uint64_t generations;
    uint64_t total_live;
    uint64_t born;
    uint64_t died;
} STATISTICS;

#define WR_VALUE _IOW('a','a', FRAME*)
#define RD_VALUE _IOR('a','b', STATISTICS*)


#define N 10   // veličina tabele 10x10

typedef struct {
    uint8_t stanje_trenutno;  /* 1 = živa, 0 = mrtva */
    uint8_t stanje_naredno;   /* naredno stanje nakon evolucije */
    sem_t   celija_semafor;   /* zadržano radi istog interfejsa */
} CELIJA;


static CELIJA tabla[N][N];


static sem_t sve_gotovo_semafor;
static int zavrsene_celije = 0;
static pthread_mutex_t brojac_mutex = PTHREAD_MUTEX_INITIALIZER;


static int turn_index = 0;
static pthread_mutex_t turn_mx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  turn_cv = PTHREAD_COND_INITIALIZER;


static pthread_t niti[N][N];

static volatile sig_atomic_t stop_requested = 0;

static void inicijalizuj_tabelu(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            tabla[i][j].stanje_trenutno = rand() % 2;
            tabla[i][j].stanje_naredno  = 0;
            sem_init(&tabla[i][j].celija_semafor, 0, 1);
        }
}


static void prikazi_tabelu(void)
{
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf("%s", tabla[i][j].stanje_trenutno ? "[X]" : "[.]");
        printf("\n");
    }
    printf("\n");
}

static inline int u_okviru(int x, int y)
{
    return x >= 0 && x < N && y >= 0 && y < N;
}

static int prebroj_zive_susjede(int x, int y)
{
    int zivi = 0;
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (u_okviru(nx, ny))
                zivi += (tabla[nx][ny].stanje_trenutno != 0);
        }
    return zivi;
}

static void premjesti_generaciju(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            tabla[i][j].stanje_trenutno = tabla[i][j].stanje_naredno;
}


typedef struct { int x, y; } args_t;

static void* evoluiraj(void* arg)
{
    args_t* a = (args_t*)arg;
    int x = a->x, y = a->y;
    free(a);

    const int my_pos = x * N + y;

    while (!stop_requested)
    {
        
        pthread_mutex_lock(&turn_mx);
        while (turn_index != my_pos && !stop_requested)
            pthread_cond_wait(&turn_cv, &turn_mx);
        pthread_mutex_unlock(&turn_mx);
        if (stop_requested) break;

        
        int zivi = prebroj_zive_susjede(x, y);
        if (tabla[x][y].stanje_trenutno) {
            tabla[x][y].stanje_naredno = (zivi == 2 || zivi == 3) ? 1 : 0;
        } else {
            tabla[x][y].stanje_naredno = (zivi == 3) ? 1 : 0;
        }

        
        int ja_sam_poslednja = 0;
        pthread_mutex_lock(&brojac_mutex);
        zavrsene_celije++;
        if (zavrsene_celije == N * N) ja_sam_poslednja = 1;
        pthread_mutex_unlock(&brojac_mutex);

       
        pthread_mutex_lock(&turn_mx);
        turn_index++;
        pthread_cond_broadcast(&turn_cv);
        pthread_mutex_unlock(&turn_mx);

        if (ja_sam_poslednja) {
            sem_post(&sve_gotovo_semafor);
        }
    }

    return NULL;
}

static void kreiraj_niti(void)
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            args_t* a = (args_t*)malloc(sizeof(args_t));
            if (!a) { perror("malloc"); exit(EXIT_FAILURE); }
            a->x = i; a->y = j;
            if (pthread_create(&niti[i][j], NULL, evoluiraj, a) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
}


static void oslobodi_resurse(int print_msg)
{
    if (print_msg) printf("\nOslobađanje resursa...\n");

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            sem_destroy(&tabla[i][j].celija_semafor);

    sem_destroy(&sve_gotovo_semafor);
    pthread_mutex_destroy(&brojac_mutex);
    pthread_mutex_destroy(&turn_mx);
    pthread_cond_destroy(&turn_cv);
}


static void signal_handler(int sig)
{
    (void)sig;
    stop_requested = 1;

    pthread_mutex_lock(&turn_mx);
    pthread_cond_broadcast(&turn_cv);
    pthread_mutex_unlock(&turn_mx);

    sem_post(&sve_gotovo_semafor);
}


static void push_and_print_stats(int dd) {
    if (dd < 0) return;

    FRAME f = { .n = GOL_N };
    int k = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j, ++k)
            f.cells[k] = tabla[i][j].stanje_trenutno ? 1 : 0;

    if (ioctl(dd, WR_VALUE, &f) < 0) 
	{
        perror("ioctl WR_VALUE");
        return;
    }

    STATISTICS st;
    if (ioctl(dd, RD_VALUE, &st) == 0) 
	{
        printf("Stats: gen=%llu live=%llu born=%llu died=%llu\n",
               (unsigned long long)st.generations,
               (unsigned long long)st.total_live,
               (unsigned long long)st.born,
               (unsigned long long)st.died);
    } 
	else 
	{
        perror("ioctl RD_VALUE");
    }
}


int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Upotreba: %s <pauza_u_sekundama>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int vrijeme_pauze = atoi(argv[1]);
    if (vrijeme_pauze < 0) {
        fprintf(stderr, "ERROR: Vrijeme pauze mora biti >= 0.\n");
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL));

    if (sem_init(&sve_gotovo_semafor, 0, 0) != 0) {
        perror("sem_init");
        return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    
    int dd = open("/dev/etx_device", O_RDWR);
    if (dd < 0) perror("open /dev/etx_device");

    inicijalizuj_tabelu();
    kreiraj_niti();

    int _tmp = system("clear"); (void)_tmp;
    printf("Početno (generacija #0):\n");
    prikazi_tabelu();

    int generacija = 0;

    while (!stop_requested)
    {
        /* Pokreni novu generaciju — redoslijed 0..N*N-1 */
        pthread_mutex_lock(&turn_mx);
        turn_index = 0;
        pthread_cond_broadcast(&turn_cv);
        pthread_mutex_unlock(&turn_mx);

        /* Čekaj da sve ćelije završe izračun narednog stanja */
        while (sem_wait(&sve_gotovo_semafor) == -1 && errno == EINTR) {
            if (stop_requested) break;
        }
        if (stop_requested) break;

        /* Reset brojača završetaka */
        pthread_mutex_lock(&brojac_mutex);
        zavrsene_celije = 0;
        pthread_mutex_unlock(&brojac_mutex);

        /* Prelaz u novu generaciju */
        premjesti_generaciju();

        /* Ispis */
        generacija++;
        int _tmp2 = system("clear"); (void)_tmp2;
        printf("=== Nova generacija #%d ===\n", generacija);
        prikazi_tabelu();


        push_and_print_stats(dd);

        if (vrijeme_pauze > 0) sleep((unsigned)vrijeme_pauze);
    }

    /* Uredan završetak: zaustavi niti i očisti resurse */
    stop_requested = 1;
    pthread_mutex_lock(&turn_mx);
    pthread_cond_broadcast(&turn_cv);
    pthread_mutex_unlock(&turn_mx);

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            pthread_join(niti[i][j], NULL);

    if (dd >= 0) close(dd);

    oslobodi_resurse(0);
    return 0;
}