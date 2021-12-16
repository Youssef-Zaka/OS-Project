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
void GetChosenAlgo(int *, int *);
void CreateClock();
void CreateScheduler(int, int, int);

void GetChosenAlgo(int *Algo, int *Q)
{

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

void CreateClock()
{
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
void CreateScheduler(int count, int ChosenAlgorithm, int Quantum)
{
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
        char *arguments[] = {"scheduler.out", n, ca, QuantumString, NULL};
        // printf("\nArg sent is : %s", n);
        int isFailure = execv("scheduler.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d \n", errno);
            exit(-1);
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//                      Scheduler Functions
/////////////////////////////////////////////////////////////////////////
void RecieveProcess(Queue *, MyProcess, int, int, int, MyProcess **, int *);
void HPF(Queue *, FILE *, MyProcess **, int *);
void SRTN(Queue *, MyProcess **, int *, FILE *, int *);
void RR(Queue *, Queue *, FILE *, int, int *, int *);

void RecieveProcess(Queue *Q, MyProcess proc, int recval, int msgq_id, int ChosenAlgorithm, MyProcess **PCB, int *countRecieved)
{
    while (recval != -1)
    {
        MyProcess *P = malloc(sizeof(MyProcess));
        proc.Status = NotCreated;
        P->ID = proc.ID;
        P->Arrival = proc.Arrival;
        P->PID = proc.PID;
        P->Priority = proc.Priority;
        P->RemainingTime = proc.RunTime;
        P->RunTime = proc.RunTime;
        P->Status = proc.Status;
        P->StartTime = proc.StartTime;
        PCB[proc.ID - 1] = P;
        *countRecieved = *countRecieved + 1;
        switch (ChosenAlgorithm)
        {
        case 1:
            pEnqueue(*Q, P, P->Priority);
            break;
        case 2:
            pEnqueue(*Q, P, P->RunTime);
            break;
        case 3:
            enqueue(*Q, P);
            break;

        default:
            break;
        }
        recval = msgrcv(msgq_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
    }
    return;
}

void HPF(Queue *Q, FILE *f, MyProcess **PCB, int *Index)
{

    MyProcess *Process = dequeue(*Q);

    Process->RemainingTime = Process->RunTime;
    Process->Status = Running;
    Process->StartTime = getClk();
    int pid = fork();
    fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), Process->ID, Process->Arrival, Process->RunTime,
            Process->RemainingTime, Process->StartTime - Process->Arrival);

    if (pid == 0)
    {
        char RemainingTime[20];
        sprintf(RemainingTime, "%d", Process->RemainingTime);
        char SchedulerPid[20];
        sprintf(SchedulerPid, "%d", getpid());
        char ProcessID[20];
        sprintf(ProcessID, "%d", Process->ID);
        char *arguments[] = {"process.out", RemainingTime, SchedulerPid, ProcessID, NULL};
        printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), Process->ID, Process->Arrival, Process->RunTime,
               Process->RemainingTime, Process->StartTime - Process->Arrival);
        int isFailure = execv("process.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d \n", errno);
            exit(-1);
        }
    }
    Process->PID = pid;

    Process->Status = Running;
    sleep(__INT_MAX__);
    PCB[*Index - 1]->Status = Finished;
    fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), Process->ID, Process->Arrival,
            Process->RunTime, 0, Process->StartTime - Process->Arrival, getClk() - Process->Arrival,
            ((float)getClk() - (float)Process->Arrival) / (float)Process->RunTime);
    printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), Process->ID, Process->Arrival,
           Process->RunTime, 0, Process->StartTime - Process->Arrival, getClk() - Process->Arrival,
           ((float)getClk() - (float)Process->Arrival) / (float)Process->RunTime);
    *Index = *Index + 1;
}

void SRTN(Queue *Q, MyProcess **CurrentP, int *CurrentRemaining, FILE *f, int *signalPid)
{

    if ((!isEmpty(*Q) && (*CurrentP) != NULL) && ((*Q)->top->process->RemainingTime < *CurrentRemaining))
    {
        (*CurrentP)->StoppedAt = getClk();
        kill((*CurrentP)->PID, SIGSTOP);
        fprintf(f, "At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
        printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
               (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
        (*CurrentP)->Status = Ready;
        *CurrentRemaining = (*Q)->top->process->RemainingTime;

        MyProcess *NewP = dequeue(*Q);
        pEnqueue(*Q, (*CurrentP), (*CurrentP)->RemainingTime);
        (*CurrentP) = NewP;
        (*CurrentP)->RemainingTime = (*CurrentP)->RunTime;
        (*CurrentP)->Status = Running;
        int pid = fork();
        if (pid == 0)
        {
            char RemainingTime[20];
            char ProcessID[20];
            sprintf(ProcessID, "%d", (*CurrentP)->ID);
            sprintf(RemainingTime, "%d", (*CurrentP)->RemainingTime);
            char *arguments[] = {"process.out", RemainingTime, ProcessID, NULL};
            printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                   (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
            int isFailure = execv("process.out", arguments);
            if (isFailure)
            {
                printf("Error No: %d \n", errno);
                exit(-1);
            }
        }
        (*CurrentP)->PID = pid;
        (*CurrentP)->StartTime = getClk();
        fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
        sleep(1);
        (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
        *CurrentRemaining = (*CurrentP)->RemainingTime;
    }
    else if ((*CurrentP) == NULL)
    {
        if (!isEmpty(*Q))
        {
            (*CurrentP) = dequeue(*Q);
            if ((*CurrentP)->Status == NotCreated)
            {
                (*CurrentP)->RemainingTime = (*CurrentP)->RunTime;
                (*CurrentP)->Status = Running;
                int pid = fork();
                if (pid == 0)
                {
                    char RemainingTime[20];
                    char ProcessID[20];
                    sprintf(ProcessID, "%d", (*CurrentP)->ID);
                    sprintf(RemainingTime, "%d", (*CurrentP)->RemainingTime);
                    char *arguments[] = {"process.out", RemainingTime, ProcessID, NULL};
                    printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                           (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
                    int isFailure = execv("process.out", arguments);
                    if (isFailure)
                    {
                        printf("Error No: %d \n", errno);
                        exit(-1);
                    }
                }
                (*CurrentP)->PID = pid;
                (*CurrentP)->StartTime = getClk();
                fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                        (*CurrentP)->RemainingTime, (*CurrentP)->StartTime - (*CurrentP)->Arrival);
                sleep(1);
                //printf("Current P is now : %d \n", (*CurrentP)->ID);
                (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
                *CurrentRemaining = (*CurrentP)->RemainingTime;
            }
            else if ((*CurrentP)->Status == Ready)
            {
                kill((*CurrentP)->PID, SIGCONT);
                fprintf(f, "At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                        (*CurrentP)->RemainingTime, getClk() - (*CurrentP)->StoppedAt);
                printf("At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                       (*CurrentP)->RemainingTime, getClk() - (*CurrentP)->StoppedAt);
                sleep(1);
                (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
                *CurrentRemaining = (*CurrentP)->RemainingTime;
                if(*CurrentRemaining == 0)
                    sleep(1);
            }
        }
        else
        {
            sleep(1);
        }
    }
    else
    {
        sleep(1);
        (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
        *CurrentRemaining = (*CurrentP)->RemainingTime;
    }

    if (*signalPid)
    {
        (*CurrentP)->Status = Finished;
        (*CurrentP)->RemainingTime = 0;
        *CurrentRemaining = __INT_MAX__;
        fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival,
                (*CurrentP)->RunTime, 0, (*CurrentP)->StartTime - (*CurrentP)->Arrival, getClk() - (*CurrentP)->Arrival,
                ((float)getClk() - (float)(*CurrentP)->Arrival) / (float)(*CurrentP)->RunTime);
        printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival,
               (*CurrentP)->RunTime, 0, (*CurrentP)->StartTime - (*CurrentP)->Arrival, getClk() - (*CurrentP)->Arrival,
               ((float)getClk() - (float)(*CurrentP)->Arrival) / (float)(*CurrentP)->RunTime);
        (*CurrentP) = NULL;
        *signalPid = 0;
    }
}

void RR(Queue *Q, Queue *RRQ, FILE *f, int Quantum, int *Index, int *signalPid)
{
    if (!isEmpty(*Q))
    {
        MyProcess *p = dequeue(*Q);
        *Index = p->ID;
        if (p->Status == NotCreated)
        {
            p->RemainingTime = p->RunTime;
            p->Status = Running;
            p->StartTime = getClk();
            int pid = fork();
            p->PID = pid;
            fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                    p->RemainingTime, p->StartTime - p->Arrival);
            if (pid == 0)
            {
                char SchedulerPid[20];
                char RemainingTime[20];
                char ProcessID[20];
                sprintf(SchedulerPid, "%d", getpid());
                sprintf(ProcessID, "%d", p->ID);
                sprintf(RemainingTime, "%d", p->RemainingTime);
                char *arguments[] = {"process.out", RemainingTime, SchedulerPid, ProcessID, NULL};
                printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->StartTime - p->Arrival);
                int isFailure = execv("process.out", arguments);
                if (isFailure)
                {
                    printf("Error No: %d \n", errno);
                    exit(-1);
                }
            }
            sleep(Quantum);
            p->RemainingTime -= Quantum;
            if (p->RemainingTime > 0)
            {
                kill(p->PID, SIGSTOP);
            }
            else if (*signalPid == 0)
            {
                sleep(__INT_MAX__);
                p->Status = Finished;
            }
            if (*signalPid)
            {
                p->Status = Finished;
                fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                        p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                        ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
                printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                       p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                       ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
            }
            else
            {
                p->Status = Ready;
                p->StoppedAt = getClk();
                fprintf(f, "At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                        p->RemainingTime, p->StartTime - p->Arrival);
                printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->StartTime - p->Arrival);
            }
            *signalPid = 0;
            if (p->Status != Finished)
            {
                kill(p->PID, SIGSTOP);
                enqueue(*RRQ, p);
            }
        }
        else
        {
            p->Status = Running;
            fprintf(f, "At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                    p->RemainingTime, getClk() - p->StoppedAt);
            printf("At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                   p->RemainingTime, p->StartTime - p->Arrival);
            kill(p->PID, SIGCONT);
            sleep(Quantum);
            p->RemainingTime -= Quantum;
            if (p->RemainingTime > 0)
            {
                kill(p->PID, SIGSTOP);
            }
            else if (*signalPid == 0)
            {
                sleep(__INT_MAX__);
                p->Status = Finished;
            }
            if (*signalPid)
            {
                p->Status = Finished;
                fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                        p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                        ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
                printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                       p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                       ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
            }
            else
            {
                p->Status = Ready;
            }
            *signalPid = 0;
            if (p->Status != Finished)
            {
                fprintf(f, "At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                        p->RemainingTime, p->StartTime - p->Arrival);
                printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->StartTime - p->Arrival);
                p->StoppedAt = getClk();
                kill(p->PID, SIGSTOP);
                enqueue(*RRQ, p);
            }
        }
    }
}