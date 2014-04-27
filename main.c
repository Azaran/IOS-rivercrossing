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

typedef struct sh_var{ // struct for saving id of shvar and pointer to data
    int shmid;
    int *data;
} sh_var;

void catHackers(int ammount, int delay);
void catSerfs(int ammount, int delay);
int verifyArg(int argc, char **argv);
int outputMsg(int argc, char **argv);
void makeShVar(char *path, char id, int size, int rights, sh_var *shvar);
void removeShVar(sh_var *shvar, int parent);

int main(int argc, char **argv)
{
    int msg_code;
    int status[2];

    if ((msg_code = outputMsg(argc, argv)) > 0)
        return 1;
    else if (msg_code < 0)
        return 0;
    sh_var count;
    count.data = NULL;
    count.shmid = 0;
    //////printf("1\n");
    makeShVar("/tmp", 'V', 2048, 0600, &count);
    *(count.data) = 0;
    //printf("2\n");
    printf("count: %d\n",*(count.data));
    //printf("15\n");
    pid_t child[2] = {1,1};
    child[0] = fork();
    
    //printf("13\n");
    if (child[0] == 0)
    {
        catHackers(atoi(argv[1]),atoi(argv[2]));
    //printf("14\n");
        child[0]++;
    }    
    else
        child[1] = fork();
    
    if (child[1] == 0)
        catSerfs(atoi(argv[1]),atoi(argv[3]));
    else
    {
        waitpid(child[1],&status[1],0);
        waitpid(child[0],&status[0],0);
    }
    removeShVar(&count,1);
    return 0;
    
}

void catHackers(int ammount, int delay)
{
    sh_var count;
    makeShVar("/tmp",'V', 2048, 0600, &count);

    //printf("3\n");
    for (count.data = count.data; *(count.data) < ammount; (*(count.data))++)
    {
        printf("H: %d\n ",*(count.data));
        usleep(delay);
    }
    //printf("4\n");
    removeShVar(&count,0);
    exit(0);
    //printf("5\n");
}

void catSerfs(int ammount, int delay)
{
    key_t key;
    key = ftok("/tmp",'V');
    int shmid;
    int *count;
    shmid = shmget(key, 1024, 0600);
    count = shmat(shmid, NULL, 0);
    //printf("6\n");
    for (*count = *count; *count < ammount; *count= *count+1)
    {
        printf("S: %d\n",*count*10);
        usleep(delay);
    }
    //printf("7\n");
    shmdt(count);
    exit(0);
}

void makeShVar(char *path, char id, int size, int rights, sh_var *shvar)
{
    key_t key;
    if ((key = ftok(path,id)) == -1)
    {
        perror("ftok");
        exit(2);
    }
   
    //printf("8\n");
    if ((shvar->shmid = shmget(key, size, rights | IPC_CREAT)) == -1)
    {
        perror("shmget");
        exit(2);
    }
    
    //printf("9\n");
    if (*(shvar->data = shmat(shvar->shmid, NULL, 0)) == -1)
    {
        perror("shmat");
        exit(2);
    }
    //printf("10\n");
}

void removeShVar(sh_var *shvar, int parent)
{
    if ((shmdt(shvar->data)) == -1)
    {
        perror("shmdt");
        exit(2);
    }
    
    //printf("11\n");
    if (parent == 1)
    {
        if ((shmctl(shvar->shmid,IPC_RMID, NULL)) == -1)
        {
            perror("shmctl");
            exit(2);
        }
        printf("12\n");
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
