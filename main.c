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
#include <limits.h>
#include <time.h>
#define OUT_FILE "./rivercrossing.out"
#define SEMNUM 8 

typedef struct numbers{ //struct for shared data
    int count;
    int hackers;
    int serfs;
    int boarding;    
    int landed;      // how many people landed?
    int size_categ; // how many people will come?
    int hdelay;     //max time for making hacker
    int sdelay;    // max time for making serf
    int bdelay;    // boat crossing time
    FILE *file;
} numbers;

typedef struct sh_var{ // struct for saving id of shvar and pointer to data
    int shmid;
    numbers *data;
} sh_var;

typedef enum {
    START = 0,WFB,BOARD,CAPT,MEMB,LAND,FINISH
} status;

void processH(int id, sem_t **sem, sh_var *shvar);
void processS(int id, sem_t **sem, sh_var *shvar);
void catHackers(sem_t **sem, sh_var *shvar);
void catSerfs(sem_t **sem, sh_var *shvar);
void errexit(char *func_err);
void statusMsg(int id, int selector, status msg_num, sh_var *shvar);
int waiting(sh_var *shvar, sem_t **sem);
int verifyArg(int argc, char **argv);
int outputMsg(int argc, char **argv);
int randomN(int max);
void attachShVar(sh_var *shvar);
void dtShVar(sh_var *shvar);
void makeShVar(char *path, char id, int size, int rights, sh_var *shvar);
void removeShVar(sh_var *shvar);


int main(int argc, char **argv)
{
    int i;
    int msg_code;
    int status[2];
    sem_t *sem[SEMNUM];
    FILE *file;
    srand(time(NULL));
    char *sems[] = {"/xvecer18_file", "/xvecer18_platform", "/xvecer18_boat", "/xvecer18_hacker", "/xvecer18_serf","/xvecer18_captain", "/xvecer18_roles", "/xvecer18_finish"};
    for (i = 0; i < SEMNUM; i++)
        sem[i] = sem_open(sems[i], O_CREAT | O_EXCL, 0644, 0);
    if ((msg_code = outputMsg(argc, argv)) > 0)
        return 1;
    else if (msg_code < 0)
        return 0;
    sh_var shvar;
    shvar.data = NULL;
    shvar.shmid = 0;
    makeShVar(argv[0], 'V', sizeof(numbers), 0600, &shvar);
    attachShVar(&shvar);
    shvar.data->count = 1;
    shvar.data->hackers = 0;
    shvar.data->serfs = 0;
    shvar.data->landed = 0;
    shvar.data->size_categ = atoi(argv[1]);
    shvar.data->hdelay = atoi(argv[2]);
    shvar.data->sdelay = atoi(argv[3]);
    shvar.data->bdelay = atoi(argv[4]);
    file  = fopen(OUT_FILE,"w");
    shvar.data->file = file;
    pid_t child[2] = {1,1};
    sem_post(sem[1]);
    child[0] = fork();
    
    if (child[0] == 0)
    {
        catHackers(sem, &shvar);
        child[0]++;
    }    
    else
        child[1] = fork();
    
    if (child[1] == 0)
        catSerfs(sem, &shvar);
    else
    {
        waitpid(child[1],&status[1],0);
        waitpid(child[0],&status[0],0);
    }
    dtShVar(&shvar);
    removeShVar(&shvar);
    
    for (i = 0; i < SEMNUM; i++)
    {
        sem_close(sem[i]);
        sem_unlink(sems[i]);
    }
    fclose(file);
    return 0;
    
}

void catHackers(sem_t **sem, sh_var *shvar)
{
    int h;
    pid_t pidH[shvar->data->size_categ]; 
    int status[shvar->data->size_categ];
    int rantime; 
    attachShVar(shvar);
    sem_post(sem[0]);
    for (h=1; h <= shvar->data->size_categ; h++)
    {
        pidH[h] = fork();
        if (pidH[h] > 0)
        {
            rantime = randomN(shvar->data->hdelay);
      //      printf("H sleeping for %d\n",rantime);
            usleep(rantime);
        }
        else
            processH(h, sem, shvar);
    }

    for (; h >= 1; h--)
    {
        waitpid(pidH[h],&status[h],0);
    }
        
    sem_close(sem[0]);
    dtShVar(shvar);
    exit(0);
}

void processH(int id, sem_t **sem, sh_var *shvar)
{
    attachShVar(shvar);
    int capt = 0;
    int i;
    
    sem_wait(sem[0]);
    statusMsg(id, 0, START, shvar); 
    sem_post(sem[0]);

    sem_wait(sem[1]);    // can I go on the platform?
    
    sem_wait(sem[0]);
    (shvar->data->hackers)++;
    statusMsg(id, 0, WFB, shvar); 
    
    //printf("Im over here hacker: %d : %d\n", id, capt); 
    
    if ((capt = waiting(shvar, sem)) == 1) // when we can board lock platform and mark captain
    {
        shvar->data->boarding = 0;
    }
    else 
        sem_post(sem[1]);        
    //printf("Oh, I was wrong Im over here hacker: %d : %d\n", id, capt); 
    sem_post(sem[0]);
     
    sem_wait(sem[3]); // can I board as hacker?
    
    sem_wait(sem[0]);
    statusMsg(id, 0, BOARD, shvar); 
    (shvar->data->boarding)++;
    sem_post(sem[0]);
    if ((shvar->data->boarding) == 4)
        sem_post(sem[2]); 
    sem_wait(sem[2]); // everybody boarded? lets asign roles
    
    sem_post(sem[2]); 
    
    if (capt == 1)
    {
        sem_wait(sem[0]);
        statusMsg(id, 0, CAPT, shvar); 
        sem_post(sem[0]);
        for (i = 0; i<3; i++)
            sem_wait(sem[6]);
        usleep(randomN(shvar->data->bdelay));
        sem_post(sem[5]);
    }
    else
    {
        sem_wait(sem[0]);
        statusMsg(id, 0, MEMB, shvar); 
        sem_post(sem[6]);
        sem_post(sem[0]);
    }
    
    sem_wait(sem[5]); // wait for the captain!
    sem_post(sem[5]);

    sem_wait(sem[0]);
    statusMsg(id, 0, LAND, shvar); 
    (shvar->data->landed)++;
    sem_post(sem[0]);
    
    if (shvar->data->landed % 4 == 0) 
    {
      //  printf("So, Im here your loyal serf");
        sem_post(sem[1]);     // open plaftorm after landing
    }
    if (shvar->data->landed == (2*shvar->data->size_categ))
        sem_post(sem[7]);
    sem_wait(sem[7]);     // everybody here? Ok, lets finish
    sem_post(sem[7]);
    
    sem_wait(sem[0]);
    statusMsg(id, 0, FINISH, shvar); 
    sem_post(sem[0]);
    
    for (i = 0; i < SEMNUM; i++)
        sem_close(sem[i]);
    dtShVar(shvar);
    exit(0);
}

void catSerfs(sem_t **sem, sh_var *shvar)
{
    int s;
    pid_t pidS[shvar->data->size_categ];
    int status[shvar->data->size_categ];
    int rantime;
    attachShVar(shvar);
    for (s=1; s <= shvar->data->size_categ; s++)
    {
        pidS[s] = fork();
        if (pidS[s] > 0)
        {
            rantime = randomN(shvar->data->sdelay);
    //        printf("S sleeping for %d\n",rantime);
            usleep(rantime);
        }
        else
            processS(s, sem, shvar);
    }
    
    for (; s >= 1; s--)
    {
        waitpid(pidS[s],&status[s],0);
    }
    
    dtShVar(shvar);
    exit(0);
}

void processS(int id, sem_t **sem, sh_var *shvar)
{
    attachShVar(shvar);
    int capt = 0;
    int i;

    sem_wait(sem[0]);
    statusMsg(id, 1, START, shvar); 
    sem_post(sem[0]);
    
    sem_wait(sem[1]); // can I go on platform?
    
    sem_wait(sem[0]);
    (shvar->data->serfs)++;
    statusMsg(id, 1, WFB, shvar); 
    
    //printf("Im over here serf: %d : %d\n", id, capt); 

    if ((capt = waiting(shvar, sem)) == 1) // when we can board lock platform and mark captain
    {
        shvar->data->boarding = 0;
    }
    else
        sem_post(sem[1]);        
    sem_post(sem[0]);
    
    sem_wait(sem[4]); // can I board as serf?

    //printf("Oh, I was wrong Im over here serf: %d : %d\n", id, capt); 
    sem_wait(sem[0]);
    (shvar->data->boarding)++;
    statusMsg(id, 1, BOARD, shvar); 
    sem_post(sem[0]);
    if ((shvar->data->boarding) == 4)
        sem_post(sem[2]); 
    
    sem_wait(sem[2]); // everybody boarded? lets asign roles
        
    sem_post(sem[2]); 

    if (capt == 1)
    {
        sem_wait(sem[0]);
        statusMsg(id, 1, CAPT, shvar);
        sem_post(sem[0]);
        for (i = 0; i<3; i++)
            sem_wait(sem[6]);
        usleep(randomN(shvar->data->bdelay));
        sem_post(sem[5]);
    }
    else
    {
        sem_wait(sem[0]);
        statusMsg(id, 1, MEMB, shvar); 
        sem_post(sem[6]);
        sem_post(sem[0]);
    }
    
    sem_wait(sem[5]); // wait for the captain!
    sem_post(sem[5]);

    sem_wait(sem[0]);
    statusMsg(id, 1, LAND, shvar); 
    (shvar->data->landed)++;
    sem_post(sem[0]);
    if (shvar->data->landed % 4 == 0) 
    {
      //  printf("So, Im here your loyal serf");
        sem_post(sem[1]);     // open plaftorm after landing
    }
    if (shvar->data->landed == (2*shvar->data->size_categ))
        sem_post(sem[7]);
    sem_wait(sem[7]);     // everybody here? Ok, lets finish
    sem_post(sem[7]);
    
    sem_wait(sem[0]);
    statusMsg(id, 1, FINISH, shvar); 
    sem_post(sem[0]);
    
    for (i = 0; i < SEMNUM; i++)
        sem_close(sem[i]);
 
    dtShVar(shvar);
    exit(0);
}

int waiting(sh_var *shvar, sem_t **sem)
{
    int *hackers = &shvar->data->hackers;
    int *serfs = &shvar->data->serfs;
    if (((*hackers) + (*serfs)) >= 4)
    {
        if ((*hackers) >= 2 && (*serfs) >= 2)
        {
            (*hackers) -= 2;
            (*serfs) -= 2;
            
            sem_post(sem[3]);
            sem_post(sem[3]);
            sem_post(sem[4]);
            sem_post(sem[4]);
   
            return 1;
        }
        else if ((*hackers) == 4)
        {
            while ((*hackers) > 0)
            {
                (*hackers)--;
                sem_post(sem[3]);
            }
            return 1;
        }
        else if ((*serfs) == 4)
        {
            while ((*serfs) > 0)
            {
                (*serfs)--;
                sem_post(sem[4]);
            }
            return 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

void statusMsg(int id, int selector, status msg_num, sh_var *shvar)
{
    int no1 = shvar->data->hackers;
    int no2 = shvar->data->serfs;
    int count = shvar->data->count++;
    FILE *output_f = shvar->data->file;
    char *cat = (selector == 1 ? "serf" : "hacker");
    
    switch (msg_num)
    {
        case 0 : {
                fprintf(output_f, "%d: %s : %d : started\n", count, cat, id); 
                break;
            }
        case 1 : {
                fprintf(output_f, "%d: %s : %d : waiting for boarding : %d : %d\n", count, cat, id, no1, no2);
                break;
            }
        case 2 : {
                fprintf(output_f, "%d: %s : %d : boarding : %d : %d\n", count, cat, id, no1, no2);
                break;}
        case 3 : {
                fprintf(output_f, "%d: %s : %d : captain\n", count, cat, id);
                break;
            }
        case 4 : {
                fprintf(output_f, "%d: %s : %d : member\n", count, cat, id);
                break;}
        case 5 : {
                fprintf(output_f, "%d: %s : %d : landing : %d : %d\n", count, cat, id, no1, no2);
                break;
            }
        case 6 : {
                fprintf(output_f, "%d: %s : %d : finished\n", count, cat, id); 
                break;
            }
    break;
    }
    fflush(output_f);
}

/*
void semFprintf(char *text)
{

}
*/
int randomN(int max)
{
    return (max == 0 ? 0 : rand()%max);
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

void attachShVar(sh_var *shvar)
{
    if ((shvar->data = shmat(shvar->shmid, NULL, 0)) == (void *) -1)
        errexit("shmat");
    //printf("init: %lu\n",(long)shvar->data);
}

void dtShVar(sh_var *shvar)
{
    if ((shmdt(shvar->data)) == -1)
        errexit("shmdt");
    //printf("end: %lu\n",(long)shvar->data);
}
void removeShVar(sh_var *shvar)
{
    if ((shmctl(shvar->shmid,IPC_RMID, NULL)) == -1)
        errexit("shmctl");
    //printf("rusim shmid %d\n",shvar->shmid);
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
                 "     R - maximal time used by boat to reach other ide of the river (in miliseconds), 0 - 5001\n"
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
