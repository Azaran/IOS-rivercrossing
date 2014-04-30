/*
 * Soubor:  main.c
 * Datum:   26.4.2013
 * Autor:   Vojtech Vecera, xvecer18@stud.fit.vutbr.cz
 * Projekt: Synchronizace procesu
 * Popis:   Rivercrossing
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#define OUT_FILE "./rivercrossing.out"

typedef struct sh_var{ // struct for saving id of shvar and pointer to data
    int shmid;
    int *data;
} sh_var;

void processH(int hid, sem_t **out, sh_var *count);
void processS(int sid, sem_t **out, sh_var *count);
void catHackers(int ammount, int delay, sem_t **out, sh_var *count);
void catSerfs(int ammount, int delay, sem_t **out, sh_var *count);
void errexit(char *func_err);
int verifyArg(int argc, char **argv);
int outputMsg(int argc, char **argv);
int randomN(int max);
void initShVar(sh_var *shvar);
void dtShVar(sh_var *shvar);
void makeShVar(char *path, char id, int size, int rights, sh_var *shvar);
void removeShVar(sh_var *shvar);


int main(int argc, char **argv)
{
    int msg_code;
    int status[2];
    FILE *output_f = fopen(OUT_FILE,"w");
    fclose(output_f);
    sem_t *out[2];
    srand(time(NULL));
    out[0] = sem_open("/xvecer18_out0", O_CREAT | O_EXCL, 0644, 0);
    out[1] = sem_open("/xvecer18_out1", O_CREAT | O_EXCL, 0644, 0);
    if ((msg_code = outputMsg(argc, argv)) > 0)
        return 1;
    else if (msg_code < 0)
        return 0;
    sh_var count;
    count.data = NULL;
    count.shmid = 0;
    makeShVar(argv[0], 'V', 4096, 0600, &count);
    initShVar(&count);
    *(count.data) = 1;
    pid_t child[2] = {1,1};
    child[0] = fork();
    
    if (child[0] == 0)
    {
        catHackers(atoi(argv[1]), atoi(argv[2]), out, &count);
        child[0]++;
    }    
    else
        child[1] = fork();
    
    if (child[1] == 0)
        catSerfs(atoi(argv[1]), atoi(argv[3]), out, &count);
    else
    {
        waitpid(child[1],&status[1],0);
        waitpid(child[0],&status[0],0);
    }
    dtShVar(&count);
    removeShVar(&count);
    sem_close(out[0]);
    sem_close(out[1]);
    sem_unlink("/xvecer18_out0");
    sem_unlink("/xvecer18_out1");
    return 0;
    
}

void catHackers(int ammount, int delay, sem_t **out, sh_var *count)
{
    int h;
    pid_t pidH[ammount]; 
    int status[ammount];
    int rantime; 
    initShVar(count);
    sem_post(out[0]);
    for (h=1; h <= ammount; h++)
    {
        pidH[h] = fork();
        if (pidH[h] > 0)
        {
            rantime = randomN(delay);
            printf("H sleeping for %d\n",rantime);
            usleep(rantime);
        }
        else
            processH(h, out, count);
    }

    for (h=1; h <= ammount; h++)
    {
        waitpid(pidH[h],&status[h],0);
    }
        
    sem_close(out[0]);
    dtShVar(count);
    exit(0);
}

void processH(int hid, sem_t **out, sh_var *count)
{
    sem_wait(out[0]);
    initShVar(count);
    FILE *output_f;
    output_f = fopen(OUT_FILE,"a");
    fprintf(output_f,"%d : H: %d\n", (*(count->data))++,hid);
    fclose(output_f);
    sem_post(out[1]);
    sem_close(out[0]);
    sem_close(out[1]);
    dtShVar(count);
    exit(0);
}

void catSerfs(int ammount, int delay, sem_t **out, sh_var *count)
{
    int s;
    pid_t pidS[ammount];
    int status[ammount];
    int rantime;
    initShVar(count);
    for (s=1; s <= ammount; s++)
    {
        pidS[s] = fork();
        if (pidS[s] > 0)
        {
            rantime = randomN(delay);
            printf("S sleeping for %d\n",rantime);
            usleep(rantime);
        }
        else
            processS(s, out, count);
    }
    
    for (s=1; s <= ammount; s++)
    {
        waitpid(pidS[s],&status[s],0);
    }
    
    dtShVar(count);
    exit(0);
}

void processS(int sid, sem_t **out, sh_var *count)
{
    initShVar(count);
    FILE *output_f;
    sem_wait(out[1]);
    output_f = fopen(OUT_FILE,"a");
    fprintf(output_f,"%d : S : %d\n",(*(count->data))++,sid);
    fclose(output_f);
    sem_post(out[0]);
    sem_close(out[0]);
    sem_close(out[1]);
    dtShVar(count);
    exit(0);
}

int randomN(int max)
{
    return (rand()%max);
}

void errexit(char *func_err)
{
    perror(func_err);
    exit(2);
}

void makeShVar(char *path, char id, int size, int rights, sh_var *shvar)
{
    key_t key;
    if ((key = ftok(path,id)) == -1)
        errexit("ftok");
    if ((shvar->shmid = shmget(key, size, rights | IPC_CREAT)) == -1)
        errexit("shmget");
}

void initShVar(sh_var *shvar)
{
    if ((shvar->data = shmat(shvar->shmid, NULL, 0)) == (void *) -1)
        errexit("shmat");
    printf("init: %lu\n",(long)shvar->data);
}

void dtShVar(sh_var *shvar)
{
    if ((shmdt(shvar->data)) == -1)
        errexit("shmdt");
    printf("end: %lu\n",(long)shvar->data);
}
void removeShVar(sh_var *shvar)
{
    if ((shmctl(shvar->shmid,IPC_RMID, NULL)) == -1)
        errexit("shmctl");
    printf("rusim shmid %d\n",shvar->shmid);
}


int outputMsg(int argc, char **argv)
{
    char *err_warn[2];
    err_warn[1] = "Invalid number of arguments. Use ./rivercrossing --help\n";
    err_warn[0] = "Invalid values of argument. Use ./rivercrossing --help\n";
    char *help = " Usage ./rivercrossing P H S R\n"
                 "     P - number of members in each category (hackers, serfs)\n"
                 "     H - maximal time between creating of two hacker processes (in miliseconds), 0 - 5001\n"
                 "     S - maximal time between creating of two serf processes (in miliseconds), 0 - 5001\n"
                 "     R - maximal time used by boat to reach other side of the river (in miliseconds), 0 - 5001\n"
                 " All parameters are represented as integers!\n";
    int err_code;
    
    if ((err_code = verifyArg(argc,argv)) != 0)
    {
        if (err_code > 0)
        {
            fprintf(stderr, "%s", err_warn[err_code-1]);
            return 1;
        }
        else
        {
            printf("%s", help);
            return -1;
        }
    }
    return 0;
}

int verifyArg(int argc, char **argv) 
{
    if (argc == 2 && strcmp(argv[1],"--help") == 0)
        return -1;
    else if (argc != 5)
        return 2;
    else if (atoi(argv[1]) <= 0 || atoi(argv[1])%2 != 0)
        return 1;
    else if ((atoi(argv[2]) <= 0 || atoi(argv[2]) >= 5001) && (strcmp(argv[2],"0") != 0))
        return 1;
    else if ((atoi(argv[3]) <= 0 || atoi(argv[3]) >= 5001) && (strcmp(argv[3],"0") != 0))
        return 1;
    else if ((atoi(argv[4]) <= 0 || atoi(argv[4]) >= 5001) && (strcmp(argv[4],"0") != 0))
        return 1;
    else 
        return 0;
}
