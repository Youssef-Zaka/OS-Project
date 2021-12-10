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
typedef struct MyProcess
{
    int ID;
    int Arrival;
    int RunTime;
    int Priority;
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

  MyProcess process;
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
void enqueue(Queue q, MyProcess process);
void pEnqueue(Queue q, MyProcess process, int prty);
MyProcess dequeue(Queue q);


Queue initQueue()
{
    Queue newQueue = malloc(sizeof(Queuetype));
    newQueue->top = NULL;
    newQueue->tail = NULL;
    return newQueue;

}

int isEmpty(Queue q)
{return q->top == NULL;}

void enqueue(Queue q, MyProcess process)
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

void pEnqueue(Queue q, MyProcess process, int prty)
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

MyProcess dequeue(Queue q)
{
    QnodePtr temp = q->top;
    MyProcess tempNum = q->top->process;
    q->top = q->top->next;
    free(temp);
    return tempNum;
}


int main(){

    MyProcess p1 = {.Arrival = 0, .ID=1,.Priority = 1, .RunTime = 10};
    MyProcess p2 = {.Arrival = 1, .ID=2,.Priority = 5, .RunTime = 10};
    MyProcess p3 = {.Arrival = 2, .ID=3,.Priority = 7, .RunTime = 120};

    Queue Q = initQueue();
    enqueue(Q, p2 );
    enqueue(Q, p1);
    enqueue(Q, p3);


    while (Q->top)
    {
       MyProcess pTest = dequeue(Q);
       printf("Element of ID %d was dequed first with priority %d \n", pTest.ID, pTest.Priority);
    }
    
    


}