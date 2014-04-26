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

void catHackers(int ammount, int delay);
void catSerfs(int ammount, int delay);
int verifyArg(int argc, char **argv);
int outputMsg(int argc, char **argv);

int main(int argc, char **argv)
{
    int msg_code;
    if ((msg_code = outputMsg(argc, argv)) > 0)
        return 1;
    else if (msg_code < 0)
        return 0;
    pid_t child[2] = {1,1};
    child[0] = fork();
    if (child[0] == 0)
    {
        catHackers(atoi(argv[1]),atoi(argv[2]));
        child[0]++;
    }    
    else
        child[1] = fork();
    if (child[1] == 0)
        catSerfs(atoi(argv[1]),atoi(argv[3]));

    return 0;
    
    
}
void catHackers(int ammount, int delay)
{
    int i =0;
    for(i = 0; i < ammount; i++)
    {
        printf("H: %d\n ",i);
        usleep(delay);
    }
}
void catSerfs(int ammount, int delay)
{
    int i =0;
    for(i = 0; i < ammount; i++)
    {
        printf("S: %d\n",i*10);
        usleep(delay);
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
