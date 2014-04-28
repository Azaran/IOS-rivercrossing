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
#define OUT_FILE "./rivercrossing.out"

typedef struct sh_var{ // struct for saving id of shvar and pointer to data
    int shmid;
    int *data;
} sh_var;

void catHackers(char *path, int ammount, int delay, sem_t **out);
void catSerfs(char *path, int ammount, int delay, sem_t **out);
void errexit(char *func_err);
int verifyArg(int argc, char **argv);
int outputMsg(int argc, char **argv);
void makeShVar(char *path, char id, int size, int rights, sh_var *shvar);
void removeShVar(sh_var *shvar, int parent);


int main(int argc, char **argv)
{
    int msg_code;
    int status[2];
    FILE *output_f = fopen(OUT_FILE,"w");
    fclose(output_f);
    sem_t *out[2];
    out[0] = sem_open("/xvecer18_out0", O_CREAT | O_EXCL, 0644, 0);
    out[1] = sem_open("/xvecer18_out1", O_CREAT | O_EXCL, 0644, 0);
    if ((msg_code = outputMsg(argc, argv)) > 0)
        return 1;
    else if (msg_code < 0)
        return 0;
    sh_var count;
    count.data = NULL;
    count.shmid = 0;
    makeShVar(argv[0], 'V', 2048, 0600, &count);
    *(count.data) = 0;
    printf("count: %d\n",*(count.data));
    pid_t child[2] = {1,1};
    child[0] = fork();
    
    if (child[0] == 0)
    {
        catHackers(argv[0], atoi(argv[1]), atoi(argv[2]),out);
        child[0]++;
    }    
    else
        child[1] = fork();
    
    if (child[1] == 0)
        catSerfs(argv[0], atoi(argv[1]), atoi(argv[3]),out);
    else
    {
        waitpid(child[1],&status[1],0);
        waitpid(child[0],&status[0],0);
    }
    removeShVar(&count,1);
    sem_close(out[0]);
    sem_close(out[1]);
    sem_unlink("/xvecer18_out0");
    sem_unlink("/xvecer18_out1");
    return 0;
    
}

void catHackers(char *path, int ammount, int delay, sem_t **out)
{
    sh_var count;
    makeShVar(path, 'V', 2048, 0600, &count);
    FILE *output_f;
    sem_post(out[1]);
    for (; *(count.data) < ammount;)
    {
        sem_wait(out[0]);
        output_f = fopen(OUT_FILE,"a");
        fprintf(output_f,"H: %d\n",(*(count.data))++);
        fclose(output_f);
        sem_post(out[1]);
        usleep(delay);
    }
    sem_close(out[0]);
    sem_close(out[1]);
    removeShVar(&count,0);
    exit(0);
}

void catSerfs(char *path, int ammount, int delay, sem_t **out)
{
    sh_var count;
    makeShVar(path, 'V', 2048, 0600, &count);
    FILE *output_f;
    for (; *(count.data) < ammount;)
    {
        sem_wait(out[1]);
        output_f = fopen(OUT_FILE,"a");
        fprintf(output_f,"S: %d\n",(*(count.data))++);
        fclose(output_f);
        sem_post(out[0]);
        usleep(delay);
    }
    sem_close(out[0]);
    sem_close(out[1]);
    removeShVar(&count,0);
    exit(0);
}
void errexit(char *func_err)
{
    errexit(func_err);
    exit(2);
}
void makeShVar(char *path, char id, int size, int rights, sh_var *shvar)
{
    key_t key;
    if ((key = ftok(path,id)) == -1)
        errexit("ftok");
   
    if ((shvar->shmid = shmget(key, size, rights | IPC_CREAT)) == -1)
        errexit("shmget");
    
    if (*(shvar->data = shmat(shvar->shmid, NULL, 0)) == -1)
        errexit("shmat");
}

void removeShVar(sh_var *shvar, int parent)
{
    if ((shmdt(shvar->data)) == -1)
        errexit("shmdt");
    
    if (parent == 1)
    {
        if ((shmctl(shvar->shmid,IPC_RMID, NULL)) == -1)
            errexit("shmctl");
    }
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
