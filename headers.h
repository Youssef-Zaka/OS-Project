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
    status Status;
    int PID;
    int Wait;
    int TA;
    float WTA;
    //Used to calculate Wait time
    int StoppedAt;
    int MemSize;
    int index;
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

    MyProcess *process; //here we edited the value from int to MyPrcess*
    struct Qnode *next;
    int prty;

} Qnode, *QnodePtr;

typedef struct Queue
{

    QnodePtr top; //head
    QnodePtr tail;

} Queuetype, *Queue;

Queue initQueue();
int isEmpty(Queue q);
void enqueue(Queue q, MyProcess *process);
void pEnqueue(Queue q, MyProcess *process, int prty);
MyProcess *dequeue(Queue q);

Queue initQueue() //allocates memory for the queue
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

void enqueue(Queue q, MyProcess *process) //normal enqueue adds to the end of the queue
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

void pEnqueue(Queue q, MyProcess *process, int prty) //Priority Enqueue where it has been edited so that lower values have higher priority
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

MyProcess *dequeue(Queue q) //takes the head of the queue changes the head to the next node
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
void ReadFromFile(MyProcess *, int, FILE *);
void GetChosenAlgo(int *, int *);
void CreateClock();
void CreateScheduler(int, int, int);

void ReadFromFile(MyProcess *procs, int count, FILE *fp)
{

    rewind(fp); // to start the file from the beginning
    fscanf(fp, "%*[^\n]\n"); //skip first line

    for (int i = 0; i < count; i++)
    {
        int nums[5];
        for (int j = 0; j < 5; j++)
        {
            fscanf(fp, "%d", &nums[j]);
        }
        (procs)[i].ID = nums[0];
        (procs)[i].Arrival = nums[1];
        (procs)[i].RunTime = nums[2];
        (procs)[i].Priority = nums[3];
        (procs)[i].MemSize = nums[4];
    }
}
void GetChosenAlgo(int *Algo, int *Q) // to ge the chosen algorithm from the user
{

    int ChosenAlgorithm = 0;
    printf("1-HPF   2-SRTN  3-RR 4-FCFS 5-SJF \n");
    printf("Choose The Desired Scheduling Algorith(1,2,3,4,5):  ");
    scanf("%d", &ChosenAlgorithm);
    while (ChosenAlgorithm < 1 || ChosenAlgorithm > 5) // range from 1~5
    {
        printf("Incorrect input, please choose between 1  and 5: ");
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
        sprintf(n, "%d", count); // to cast from integer to char[]
        char ca[20];
        sprintf(ca, "%d", ChosenAlgorithm);
        char QuantumString[20];
        sprintf(QuantumString, "%d", Quantum);
        char *arguments[] = {"scheduler.out", n, ca, QuantumString, NULL};
        int isFailure = execv("scheduler.out", arguments); // to create schedluer process
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
void HPF(Queue *, FILE *);
void SRTN(Queue *, MyProcess **, int *, FILE *, int *);
void RR(Queue *, Queue *, FILE *, int, int *);
void CalculatePerfs(MyProcess **, int);

void HPF(Queue *Q, FILE *f) // File and Queue are inputs
{

    MyProcess *Process = dequeue(*Q);//Dequeue process one by one Note:Pointer as it sended by reference

    Process->RemainingTime = Process->RunTime;//at first enter ,Remaining time =Runtime
    Process->Status = Running;
    Process->StartTime = getClk();
    Process->Wait += Process->StartTime - Process->Arrival;
	fprintf(f, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), Process->ID, Process->Arrival, Process->RunTime,
            Process->RemainingTime, Process->Wait);//file print
    int pid = fork();


    if (pid == 0)
    {
        char RemainingTime[20];//Var to get Remianing time of the process
        sprintf(RemainingTime, "%d", Process->RemainingTime);
        char *arguments[] = {"process.out", RemainingTime, NULL};
        printf("At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), Process->ID, Process->Arrival, Process->RunTime,
               Process->RemainingTime, Process->Wait);
        int isFailure = execv("process.out", arguments); //Making copy of schedular  and replace it with a process
        if (isFailure)
        {
            printf("Error No: %d \n", errno);
            exit(-1);
        }
    }
    Process->PID = pid;//Don't use unless for PCB
    sleep(__INT_MAX__);// ^-^sleeping....until process end and send sigchild
    Process->Status = Finished;
    Process->TA = getClk() - Process->Arrival;
    Process->WTA = ((float)getClk() - (float)Process->Arrival) / (float)Process->RunTime;
    fprintf(f, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), Process->ID, Process->Arrival,
            Process->RunTime, 0, Process->Wait, Process->TA, Process->WTA);
    printf("At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), Process->ID, Process->Arrival,
           Process->RunTime, 0, Process->Wait, Process->TA, Process->WTA);
}

void SRTN(Queue *Q, MyProcess **CurrentP, int *CurrentRemaining, FILE *f, int *signalPid)
{
    //We enter this If when a process has just arrived and its RunTime is less than the Remaining Time of the current process
    if ((!isEmpty(*Q) && (*CurrentP) != NULL) && ((*Q)->top->process->RemainingTime < *CurrentRemaining))
    {
        (*CurrentP)->StoppedAt = getClk(); //We save the time it stopped here to calculate its wait later
        kill((*CurrentP)->PID, SIGSTOP);
        fprintf(f, "At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
        printf("At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
               (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
        (*CurrentP)->Status = Ready;                           //Ready to be run again
        *CurrentRemaining = (*Q)->top->process->RemainingTime; //set the Remaining time of the running process to the new process

        MyProcess *NewP = dequeue(*Q);
        pEnqueue(*Q, (*CurrentP), (*CurrentP)->RemainingTime); //Priority queue with the remaining time as the priority
        (*CurrentP) = NewP;                                    //sets the current process to the new process
        (*CurrentP)->RemainingTime = (*CurrentP)->RunTime;
        (*CurrentP)->Status = Running;
        int pid = fork();
        if (pid == 0) //create process
        {
            char RemainingTime[20];
            sprintf(RemainingTime, "%d", (*CurrentP)->RemainingTime);
            char *arguments[] = {"process.out", RemainingTime, NULL};
            int isFailure = execv("process.out", arguments);
            if (isFailure)
            {
                printf("Error No: %d \n", errno);
                exit(-1);
            }
        }
        (*CurrentP)->PID = pid;
        (*CurrentP)->StartTime = getClk();
        (*CurrentP)->Wait += (*CurrentP)->StartTime - (*CurrentP)->Arrival;
        printf("At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
               (*CurrentP)->RemainingTime, (*CurrentP)->Wait);

        fprintf(f, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                (*CurrentP)->RemainingTime, (*CurrentP)->Wait);

        sleep(1); //sleep for a cycle then decrement the Remaining time by 1
        (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
        *CurrentRemaining = (*CurrentP)->RemainingTime;
    }
    //If no process is running
    else if ((*CurrentP) == NULL)
    {
        if (!isEmpty(*Q))
        {
            (*CurrentP) = dequeue(*Q);
            //If to know whether it was a stopped process or one that just arrived
            if ((*CurrentP)->Status == NotCreated)
            {
                (*CurrentP)->RemainingTime = (*CurrentP)->RunTime;
                (*CurrentP)->Status = Running;
                int pid = fork();
                if (pid == 0) //create process
                {
                    char RemainingTime[20];
                    sprintf(RemainingTime, "%d", (*CurrentP)->RemainingTime);
                    char *arguments[] = {"process.out", RemainingTime, NULL};
                    int isFailure = execv("process.out", arguments);
                    if (isFailure)
                    {
                        printf("Error No: %d \n", errno);
                        exit(-1);
                    }
                }
                (*CurrentP)->PID = pid;
                (*CurrentP)->StartTime = getClk();
                (*CurrentP)->Wait += (*CurrentP)->StartTime - (*CurrentP)->Arrival;
                printf("At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                       (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                        (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
                sleep(1); //sleep for a cycle then decrement the Remaining time by 1
                (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
                *CurrentRemaining = (*CurrentP)->RemainingTime;
            }
            else if ((*CurrentP)->Status == Ready) //If it was a stopped function
            {
                kill((*CurrentP)->PID, SIGCONT);
                (*CurrentP)->Wait += getClk() - (*CurrentP)->StoppedAt;
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                        (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
                printf("At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival, (*CurrentP)->RunTime,
                       (*CurrentP)->RemainingTime, (*CurrentP)->Wait);
                sleep(1); //sleep for a cycle then decrement the Remaining time by 1
                (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
                *CurrentRemaining = (*CurrentP)->RemainingTime;

                if (*CurrentRemaining == 0) //this condition is mainly here for the last process to make sure we dont exit the scheduler before printing that it finished
                    sleep(1);
            }
        }
        //If there's nothing to run sleep a cycle
        else
        {
            sleep(1);
        }
    }
    //If the arrived process' Run Time > Current Remaining Time
    else
    {
        sleep(1); //sleep for a cycle then decrement the Remaining time by 1
        (*CurrentP)->RemainingTime = (*CurrentP)->RemainingTime - 1;
        *CurrentRemaining = (*CurrentP)->RemainingTime;
    }

    //When a proces terminates
    if (*signalPid)
    {
        (*CurrentP)->Status = Finished;
        (*CurrentP)->RemainingTime = 0;
        *CurrentRemaining = __INT_MAX__; // to enter the next if condition
        (*CurrentP)->TA = getClk() - (*CurrentP)->Arrival;
        (*CurrentP)->WTA = ((float)getClk() - (float)(*CurrentP)->Arrival) / (float)(*CurrentP)->RunTime;
        fprintf(f, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival,
                (*CurrentP)->RunTime, 0, (*CurrentP)->Wait, (*CurrentP)->TA, (*CurrentP)->WTA);
        printf("At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), (*CurrentP)->ID, (*CurrentP)->Arrival,
               (*CurrentP)->RunTime, 0, (*CurrentP)->Wait, (*CurrentP)->TA, (*CurrentP)->WTA);
        (*CurrentP) = NULL;
        *signalPid = 0;
    }
}

void RR(Queue *Q, Queue *RRQ, FILE *f, int Quantum, int *signalPid)
{
    //If the queue is not empty, dequeue the process that is in the end of the queue to run
    if (!isEmpty(*Q))
    {
        MyProcess *p = dequeue(*Q);
        if (p->Status == NotCreated) //If we get a new process
        {
            p->RemainingTime = p->RunTime; //Set it's reamining time with the current run time
            p->Status = Running; //Change it's status to running
            p->StartTime = getClk(); //Setting its starting time with the current time
            int pid = fork();
            p->PID = pid;
            p->Wait = p->StartTime - p->Arrival; //Get the Wait of the process
            fprintf(f, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                    p->RemainingTime, p->Wait);
            if (pid == 0)
            {
                char RemainingTime[20]; //Char array to send remaning time
                sprintf(RemainingTime, "%d", p->RemainingTime);
                char *arguments[] = {"process.out", RemainingTime, NULL};
                printf("At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->Wait);
                int isFailure = execv("process.out", arguments);
                if (isFailure)
                {
                    printf("Error No: %d \n", errno);
                    exit(-1);
                }
            }
            sleep(Quantum); //Sleep a quantum of time or till the process finishes
            p->RemainingTime -= Quantum; //Setting the remaining time of the process after working for a qantum of time
            if (p->RemainingTime > 0) //If the process did't finish
            {
                kill(p->PID, SIGSTOP);
            }
            else if (*signalPid == 0) //If the process hasn't finished, let it finish
            {
                sleep(__INT_MAX__);
                p->Status = Finished;
            }
            if (*signalPid) //If the process finished
            {
                p->Status = Finished;
                p->TA = getClk() - p->Arrival; //Calculating the turnaround time
                p->WTA = ((float)getClk() - (float)p->Arrival) / (float)p->RunTime; //Calculating the weighted turnaround time
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), p->ID, p->Arrival,
                        p->RunTime, 0, p->Wait, p->TA, p->WTA);
                printf("At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), p->ID, p->Arrival,
                       p->RunTime, 0, p->Wait, p->TA, p->WTA);
            }
            else
            { //If the process didn't finish, Set it's status to ready, and save the time it stopped at
                p->Status = Ready;
                p->StoppedAt = getClk();
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                        p->RemainingTime, p->Wait);
                printf("At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->Wait);
            }
            *signalPid = 0; //Reset signal pid
            if (p->Status != Finished) //If the status os the process is not Finished, enqueue it in the second queue (RRQ), to run with the other processes in the right order
            {
                kill(p->PID, SIGSTOP);
                enqueue(*RRQ, p);
            }
        }
        else
        { //If we get a process that was already created before
            p->Status = Running; //Set it's status to running
            p->Wait += getClk() - p->StoppedAt; //Calculate it's waiting time
            fprintf(f, "At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                    p->RemainingTime, p->Wait);
            printf("At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                   p->RemainingTime, p->Wait);
            kill(p->PID, SIGCONT);
            sleep(Quantum); //Sleep a quantum of time or till the process finishes
            p->RemainingTime -= Quantum; //Setting the remaining time of the process after working for a qantum of time
            if (p->RemainingTime > 0) //If the process did't finish
            {
                kill(p->PID, SIGSTOP);
            }
            else if (*signalPid == 0) //If the process hasn't finished, let it finish
            {
                sleep(__INT_MAX__);
                p->Status = Finished;
            }
            if (*signalPid) //If the process finished
            {
                p->Status = Finished;
                p->TA = getClk() - p->Arrival;
                p->WTA = ((float)getClk() - (float)p->Arrival) / (float)p->RunTime;
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), p->ID, p->Arrival,
                        p->RunTime, 0, p->Wait, p->TA, p->WTA);
                printf("At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\t\n", getClk(), p->ID, p->Arrival,
                       p->RunTime, 0, p->Wait, p->TA, p->WTA);
            }
            else
            {
                p->Status = Ready; //If the process didn't finish, Set it's status to ready
            }
            *signalPid = 0; //Reset signal pid
            if (p->Status != Finished) //If the status os the process is not Finished, enqueue it in the second queue (RRQ), to run with the other processes in the right order
            {
                fprintf(f, "At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                        p->RemainingTime, p->Wait);
                printf("At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                       p->RemainingTime, p->Wait);
                p->StoppedAt = getClk();
                kill(p->PID, SIGSTOP);
                enqueue(*RRQ, p);
            }
        }
    }
}

void CalculatePerfs(MyProcess **PCB, int N)
{

    //assuming Scheduler starts at time 1
    //assuming that if there are no processes running, it is time wasted
    int TotalTime = getClk() - 1;
    int FinishTime = getClk();
    int SumRunTime = 0;
    float SumWTA = 0;
    int SumWait = 0;
    for (int i = 0; i < N; i++)
    {
        SumRunTime += PCB[i]->RunTime;
        SumWTA += PCB[i]->WTA;
        SumWait += PCB[i]->Wait;
    }
    int WastedTime = FinishTime - SumRunTime - PCB[0]->Arrival + 1;
    if (PCB[0]->Arrival == 1)
    {
        WastedTime--;
    }
    float CPU_Util = (float)((float)(TotalTime - WastedTime) / TotalTime) * 100;
    float AvgWTA = SumWTA / N;
    float AvgWait = (float)SumWait / N;

    float SumStdDiv = 0;
    for (int i = 0; i < N; i++)
    {
        SumStdDiv += pow((PCB[i]->WTA - AvgWTA), 2);
        free(PCB[i]);
    }
    float StdDiv = sqrt(SumStdDiv / N);

    //Open output file stream
    FILE *f;
    f = fopen("scheduler.perf", "w");
    fprintf(f, "CPU\tutilization\t=\t%.2f%%\nAvg\tWTA\t=\t%.2f\nAvg\tWaiting\t=\t%.2f\nStd\tWTA\t=\t%.2f", CPU_Util, AvgWTA, AvgWait, StdDiv);
    fclose(f);
    return;
}
