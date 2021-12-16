#include "headers.h"

int QTR = -1;

Queue Q;
Queue RRQ;
int signalPid = 0;
int signalID = 0;
int Index = 1;
int countFinished = 0;
int countRecieved = 0;
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
    struct sigaction sa;
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    // sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = EndOfProcess;
    sigaction(SIGCHLD, &sa, NULL);

    int msgqid_id, recval, sendval;

    msgqid_id = msgget(qid, 0644 | IPC_CREAT);

    if (msgqid_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    // Array Size , or number of expected processes.
    ProcNum = atoi(argv[1]);
    int ChosenAlgorithm = atoi(argv[2]);
    int Quantum = atoi(argv[3]);

    MyProcess *PCB[ProcNum];

    Q = initQueue();
    RRQ = initQueue();
    MyProcess proc;

    FILE *f;
    f = fopen("scheduler.log", "w");
    fprintf(f, "#At time x process y state arr w total z remain y wait k\n");

    int CurrentRemaining = __INT_MAX__;
    MyProcess *CurrentP = NULL;
    initClk();

    while (true)
    {
        recval = -1;
        if (countRecieved < ProcNum)
        {
            recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
        }
        else if (countFinished == ProcNum)
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
            countRecieved++;
            switch (ChosenAlgorithm)
            {
            case 1:
                pEnqueue(Q, P, P->Priority);
                break;
            case 2:
                pEnqueue(Q, P, P->RunTime);
                break;
            case 3:
                enqueue(Q, P);
                break;

            default:
                break;
            }
            recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
        }
        switch (ChosenAlgorithm)
        {
        case 1:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }

            MyProcess *Process = dequeue(Q);

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
            PCB[Index - 1]->Status = Finished;
            fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), Process->ID, Process->Arrival,
                    Process->RunTime, 0, Process->StartTime - Process->Arrival, getClk() - Process->Arrival,
                    ((float)getClk() - (float)Process->Arrival) / (float)Process->RunTime);
            printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), Process->ID, Process->Arrival,
                   Process->RunTime, 0, Process->StartTime - Process->Arrival, getClk() - Process->Arrival,
                   ((float)getClk() - (float)Process->Arrival) / (float)Process->RunTime);
            Index++;
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
                fprintf(f,"At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), CurrentP->ID, CurrentP->Arrival, CurrentP->RunTime,
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
            if (!isEmpty(Q))
            {
                MyProcess *p = dequeue(Q);
                Index = p->ID;
                if (p->Status == NotCreated)
                {
                    p->RemainingTime = p->RunTime;
                    p->Status = Running;
                    p->StartTime = getClk();
                    int pid = fork();
                    p->PID = pid;
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
                    else
                    {
                        sleep(__INT_MAX__);
                        p->Status = Finished;
                    }
                    if (signalPid)
                    {
                        p->Status = Finished;
                        printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                               p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                               ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
                    }
                    else
                    {
                        p->Status = Ready;
                        printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                               p->RemainingTime, p->StartTime - p->Arrival);
                    }
                    signalPid = 0;
                    if (p->Status != Finished)
                    {
                        kill(p->PID, SIGSTOP);
                        enqueue(RRQ, p);
                    }
                }
                else
                {
                    p->Status = Running;
                    printf("At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                           p->RemainingTime, p->StartTime - p->Arrival);
                    kill(p->PID, SIGCONT);
                    sleep(Quantum);
                    p->RemainingTime -= Quantum;
                    if (p->RemainingTime > 0)
                    {
                        kill(p->PID, SIGSTOP);
                    }
                    else
                    {
                        sleep(__INT_MAX__);
                        p->Status = Finished;
                    }
                    if (signalPid)
                    {
                        p->Status = Finished;
                        printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n", getClk(), p->ID, p->Arrival,
                               p->RunTime, 0, p->StartTime - p->Arrival, getClk() - p->Arrival,
                               ((float)getClk() - (float)p->Arrival) / (float)p->RunTime);
                    }
                    else
                    {
                        p->Status = Ready;
                    }
                    signalPid = 0;
                    if (p->Status != Finished)
                    {
                        printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n", getClk(), p->ID, p->Arrival, p->RunTime,
                               p->RemainingTime, p->StartTime - p->Arrival);
                        kill(p->PID, SIGSTOP);
                        enqueue(RRQ, p);
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    // TODO implement the scheduler :)
    // upon termination release the clock resources

    destroyClk(true);
}
