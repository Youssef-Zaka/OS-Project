#include <stdio.h>      //if you don't use scanf/printf change this include
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
int * shmaddr;                 //
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
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
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



typedef enum status {
    NotCreated,
    Ready,
    Running,
    Blocked,
    Finished
}status;
//create Data structure for process

typedef struct MyProcess
{
    int ID;
    int Arrival;
    int RunTime;
    int Priority;
    int RemainingTime;
    status Status;
    int PID;
}MyProcess;

struct msgbuff
{
    long mtype;
    MyProcess process;
};


int GetDigitsOfInt(int i)  
{  
    int digitCount=0;  
    while(i!=0)  
    {  
        i=i/10;  
        digitCount++;  
    }  
    return digitCount;  
}  

//https://github.com/corbosiny/C-Tutorials
const int MAX_PRIORITY = 999;

typedef struct Qnode
{

  MyProcess * process;
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
void enqueue(Queue q, MyProcess * process);
void pEnqueue(Queue q, MyProcess * process, int prty);
MyProcess * dequeue(Queue q);


Queue initQueue()
{
    Queue newQueue = malloc(sizeof(Queuetype));
    newQueue->top = NULL;
    newQueue->tail = NULL;
    return newQueue;

}

int isEmpty(Queue q)
{return q->top == NULL;}

void enqueue(Queue q, MyProcess * process)
{
    QnodePtr newNode = malloc(sizeof(Qnode));
    newNode->process = process;
    newNode->next = NULL;
    newNode->prty = MAX_PRIORITY;

    if(isEmpty(q)) {q->top = newNode; q->tail = newNode;}
    else
    {
            q->tail->next = newNode;
            q->tail = newNode;

    }

}

void pEnqueue(Queue q, MyProcess * process, int prty)
{

    QnodePtr newNode = malloc(sizeof(Qnode));
    newNode->process = process;
    newNode->next = NULL;
    newNode->prty = prty;

    if(isEmpty(q)) {q->top = newNode; q->tail = newNode;}
    else
    {
            // q->tail->next = newNode;
            // q->tail = newNode;

            QnodePtr temp = q->top;
            if(temp->prty > newNode->prty)
            {
                q->top = newNode;
                newNode->next = temp;
                return;

            }

            while(temp->next != NULL && temp->next->prty < newNode->prty)
                {temp = temp->next;}

            if(temp->next != NULL) {newNode->next = temp->next;}
            temp->next = newNode;

    }

}

MyProcess * dequeue(Queue q)
{
    QnodePtr temp = q->top;
    MyProcess * tempNum = q->top->process;
    q->top = q->top->next;
    free(temp);
    return tempNum;
}

// QnodePtr dequeue(Queue q)
// {

//     if(isEmpty(q)) {printf("Queue is already empty, no nodes to remove"); return NULL;}
//     QnodePtr temp = q->top;
//     MyProcess tempNum = q->top->process;
//     q->top = q->top->next;
//     free(temp);
//     return temp;

// }


