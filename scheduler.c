#include "headers.h"

int QTR = -1;

Queue Q;
Queue RRQ;
int signalPid = 0;
int signalID = 0;
int Index = 1;
int countFinished = 0;
int ProcNum;
void EndOfProcess(int sig, siginfo_t *info, void *context)
{
    signalPid = 0;
    if (info->si_code == CLD_EXITED)
    {
        signalPid = info->si_pid;
        countFinished++;
    }
}
int main(int argc, char *argv[])
{
    //Signal Action handler
    struct sigaction sa;
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    sa.sa_sigaction = EndOfProcess;
    sigaction(SIGCHLD, &sa, NULL);

    int msgqid_id, recval, sendval;

    msgqid_id = msgget(qid, 0644 | IPC_CREAT);
    if (msgqid_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    //Number of expected processes.
    ProcNum = atoi(argv[1]);
    //The Algorithm the user picked
    int ChosenAlgorithm = atoi(argv[2]);
    //always 0 unless algo is RR
    int Quantum = atoi(argv[3]);

    //Initially no process is recieved
    int countRecieved = 0;

    //Process Table
    MyProcess *PCB[ProcNum];

    //Ready Queue
    Q = initQueue();
    //Helper Queue used in RR
    RRQ = initQueue();

    //Process recieved from Message queue
    MyProcess proc;

    //Open output file stream
    FILE *f;
    f = fopen("scheduler.log", "w");
    //First comment in the log file
    fprintf(f, "#At time x process y state arr w total z remain y wait k\n");

    //Remaining time of the running process, used In SRTN
    int CurrentRemaining = __INT_MAX__;
    //The running process, used in SRTN
    MyProcess *CurrentP = NULL;

    //initialize clock
    initClk();

    //main scheduler loop
    //
    //-Check for input
    //-If a process is recieved, Enqueue it.
    //-execute and manage processes
    //-Dequeue and set processes status if they terminate
    //- If no more processes to recieve, Exit.
    //
    while (true)
    {

        recval = -1;
        if (countRecieved < ProcNum)
        {
            recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
        }
        else if (countFinished >= ProcNum)
        {
            printf("scheduler EXITING\n");
            fclose(f);
            destroyClk(true);
            for (int i = 0; i < ProcNum; i++)
            {
                free(PCB[i]);
            }
            exit(0);
        }

        //recieve all other processes that arrived, enqueue them.
        RecieveProcess(&Q, proc, recval, msgqid_id, ChosenAlgorithm, PCB, &countRecieved);

        switch (ChosenAlgorithm)
        {
        case 1:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }

            HPF(&Q, f, PCB, &Index);
            break;
        case 2:

            if (!isEmpty(Q) && CurrentP != NULL && Q->top->process->RemainingTime < CurrentRemaining)
            {
                CurrentP->StoppedAt = getClk();
                kill(CurrentP->PID, SIGSTOP);
                fprintf(f, "At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                        CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                       CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                CurrentP->Status = Ready;
                CurrentRemaining = Q->top->process->RemainingTime;

                MyProcess *NewP = dequeue(Q);
                pEnqueue(Q, CurrentP, CurrentP->RemainingTime);
                CurrentP = NewP;
                CurrentP->RemainingTime = CurrentP->RunTime;
                CurrentP->Status = Running;
                int pid = fork();
                CurrentP->PID = pid;
                CurrentP->StartTime = getClk();
                fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                        CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                if (pid == 0)
                {
                    char RemainingTime[20];
                    char ProcessID[20];
                    sprintf(ProcessID, "%d", CurrentP->ID);
                    sprintf(RemainingTime, "%d", CurrentP->RemainingTime);
                    char *arguments[] = {"process.out", RemainingTime, ProcessID, NULL};
                    printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                           CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                    int isFailure = execv("process.out", arguments);
                    if (isFailure)
                    {
                        printf("Error No: %d \n", errno);
                        exit(-1);
                    }
                }
                sleep(1);
                CurrentP->RemainingTime--;
                CurrentRemaining = CurrentP->RemainingTime;
            }
            else if (CurrentP == NULL)
            {
                if (!isEmpty(Q))
                {
                    CurrentP = dequeue(Q);
                    if (CurrentP->Status == NotCreated)
                    {
                        CurrentP->RemainingTime = CurrentP->RunTime;
                        CurrentP->Status = Running;
                        int pid = fork();
                        CurrentP->PID = pid;
                        CurrentP->StartTime = getClk();
                        fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                                CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                        if (pid == 0)
                        {
                            char RemainingTime[20];
                            char ProcessID[20];
                            sprintf(ProcessID, "%d", CurrentP->ID);
                            sprintf(RemainingTime, "%d", CurrentP->RemainingTime);
                            char *arguments[] = {"process.out", RemainingTime, ProcessID, NULL};
                            printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                                   CurrentP->RemainingTime, CurrentP->StartTime - CurrentP->Arrival);
                            int isFailure = execv("process.out", arguments);
                            if (isFailure)
                            {
                                printf("Error No: %d \n", errno);
                                exit(-1);
                            }
                        }
                        sleep(1);
                        CurrentP->RemainingTime--;
                        CurrentRemaining = CurrentP->RemainingTime;
                    }
                    else if (CurrentP->Status == Ready)
                    {
                        kill(CurrentP->PID, SIGCONT);
                        fprintf(f, "At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                                CurrentP->RemainingTime, getClk() - CurrentP->StoppedAt);
                        printf("At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
                               CurrentP->RemainingTime, getClk() - CurrentP->StoppedAt);
                        sleep(1);
                        CurrentP->RemainingTime--;
                        CurrentRemaining = CurrentP->RemainingTime;
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
                CurrentP->RemainingTime--;
                CurrentRemaining = CurrentP->RemainingTime;
            }

            if (signalPid)
            {
                CurrentP->Status = Finished;
                CurrentP->RemainingTime = 0;
                CurrentRemaining = __INT_MAX__;
                fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), CurrentP->ID, CurrentP->Arrival,
                        CurrentP->RunTime, 0, CurrentP->StartTime - CurrentP->Arrival, getClk() - CurrentP->Arrival,
                        ((float)getClk() - (float)CurrentP->Arrival) / (float)CurrentP->RunTime);
                printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), CurrentP->ID, CurrentP->Arrival,
                       CurrentP->RunTime, 0, CurrentP->StartTime - CurrentP->Arrival, getClk() - CurrentP->Arrival,
                       ((float)getClk() - (float)CurrentP->Arrival) / (float)CurrentP->RunTime);
                CurrentP = NULL;
                signalPid = 0;
            }
            // SRTN(&Q,CurrentP,&CurrentRemaining,f,&signalPid);
            break;
        case 3:
            if (isEmpty(Q))
            {
                if (isEmpty(RRQ))
                {
                    break;
                }
                while (!isEmpty(RRQ))
                {
                    enqueue(Q, dequeue(RRQ));
                }
            }
            RR(&Q, &RRQ, f, Quantum, &Index, &signalPid);
            break;

        default:
            break;
        }
    }
    // TODO implement the scheduler :)
    // upon termination release the clock resources

    destroyClk(true);
}
