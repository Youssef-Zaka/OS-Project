#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

#include <errno.h>
#include <time.h>
typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

#define qid 60

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}

//The status of any process at any point in time
typedef enum status
{
    NotCreated,
    Ready,
    Running,
    Blocked,
    Finished
} status;

//Data structure for process
typedef struct MyProcess
{
    int ID;
    int Arrival;
    int RunTime;
    int Priority;
    int RemainingTime;
    int StartTime;
    int StoppedAt;
    status Status;
    int PID;
} MyProcess;

//Message buffer used for IPC
struct msgbuff
{
    long mtype;
    MyProcess process;
};


/////////////////////////////////////////////////////////////////////////
//                      Queues & thier Functions
//
//Retrieved from https://github.com/corbosiny/C-Tutorials
//..Edited to fit our needs
/////////////////////////////////////////////////////////////////////////
const int MAX_PRIORITY = 999;

typedef struct Qnode
{

    MyProcess *process;
    struct Qnode *next;
    int prty;

} Qnode, *QnodePtr;

typedef struct Queue
{

    QnodePtr top;
    QnodePtr tail;

} Queuetype, *Queue;

Queue initQueue();
int isEmpty(Queue q);
void enqueue(Queue q, MyProcess *process);
void pEnqueue(Queue q, MyProcess *process, int prty);
MyProcess *dequeue(Queue q);

Queue initQueue()
{
    Queue newQueue = malloc(sizeof(Queuetype));
    newQueue->top = NULL;
    newQueue->tail = NULL;
    return newQueue;
}

int isEmpty(Queue q)
{
    return q->top == NULL;
}

void enqueue(Queue q, MyProcess *process)
{
    QnodePtr newNode = malloc(sizeof(Qnode));
    newNode->process = process;
    newNode->next = NULL;
    newNode->prty = MAX_PRIORITY;

    if (isEmpty(q))
    {
        q->top = newNode;
        q->tail = newNode;
    }
    else
    {
        q->tail->next = newNode;
        q->tail = newNode;
    }
}

void pEnqueue(Queue q, MyProcess *process, int prty)
{

    QnodePtr newNode = malloc(sizeof(Qnode));
    newNode->process = process;
    newNode->next = NULL;
    newNode->prty = prty;

    if (isEmpty(q))
    {
        q->top = newNode;
        q->tail = newNode;
    }
    else
    {
        // q->tail->next = newNode;
        // q->tail = newNode;

        QnodePtr temp = q->top;
        if (temp->prty > newNode->prty)
        {
            q->top = newNode;
            newNode->next = temp;
            return;
        }

        while (temp->next != NULL && temp->next->prty < newNode->prty)
        {
            temp = temp->next;
        }

        if (temp->next != NULL)
        {
            newNode->next = temp->next;
        }
        temp->next = newNode;
    }
}

MyProcess *dequeue(Queue q)
{
    QnodePtr temp = q->top;
    MyProcess *tempNum = q->top->process;
    q->top = q->top->next;
    free(temp);
    return tempNum;
}



/////////////////////////////////////////////////////////////////////////
//                   Process Generator Functions
/////////////////////////////////////////////////////////////////////////
void GetChosenAlgo(int *, int*);
void CreateClock();
void CreateScheduler(int,int,int);


void GetChosenAlgo(int *Algo, int* Q){

    int ChosenAlgorithm = 0;
    printf("1-HPF   2-SRTN  3-RR \n");
    printf("Choose The Desired Scheduling Algorith(1,2,3):  ");
    scanf("%d", &ChosenAlgorithm);
    while (ChosenAlgorithm != 1 && ChosenAlgorithm != 2 && ChosenAlgorithm != 3)
    {
        printf("Incorrect input, please choose between 1 ,2 and 3: ");
        scanf("%d", &ChosenAlgorithm);
    }
    printf("Chosen Algo was : %d \n", ChosenAlgorithm);

    int Quantum = 0;
    if (ChosenAlgorithm == 3)
    {
        printf("Enter Round Robin Quantum:   ");
        
        scanf("%d", &Quantum);
        while (Quantum <= 0)
        {
            printf("Incorrect input, please choose a positive integer:  ");
            scanf("%d", &Quantum);
        }
        printf("\n");
    }


    *Algo = ChosenAlgorithm;
    *Q = Quantum;
    return;
}

void CreateClock(){
    // fork a child, then call execv to replace this child with a clock process
    int pid = fork();
    if (pid == 0)
    {
        //execv argv , a null terminated list of strings
        char *arguments[] = {"clk.out", NULL};
        int isFailure = execv("clk.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d", errno);
            exit(-1);
        }
    }
}
void CreateScheduler(int count, int ChosenAlgorithm, int Quantum){
    // fork a child, then call execv to replace this child with a scheduler process
    int pid = fork();
    if (pid == 0)
    {
        // execv argv , a null terminated list of strings
        char n[20];
        sprintf(n, "%d", count);
        char ca[20];
        sprintf(ca, "%d", ChosenAlgorithm);
        char QuantumString[20];
        sprintf(QuantumString, "%d", Quantum);
        char *arguments[] = {"scheduler.out",n,ca,QuantumString,NULL};
        // printf("\nArg sent is : %s", n);
        int isFailure = execv("scheduler.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d \n", errno);
            exit(-1);
        }
    }
}