#include "headers.h"

int QTR = -1;

Queue Q;
Queue RRQ;
int signalPid = -1;
int Index = 1;
int countFinished = 0;
int countRecieved = 0;
int ProcNum;
void EndOfProcess(int sig, siginfo_t *info, void *context)
{
    signalPid = info->si_pid;
    countFinished++;
    printf("Count finished is %d \n", countFinished);
    if (countFinished == ProcNum)
    {
        printf("scheduler EXITING\n");
        destroyClk(true);
        exit(0);
    }
}
int main(int argc, char *argv[])
{

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = EndOfProcess;
    sigaction(SIGCHLD, &sa, NULL);

    int msgqid_id, recval, sendval;

    msgqid_id = msgget(qid, 0644 | IPC_CREAT);

    if (msgqid_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    //Array Size , or number of expected processes.
    ProcNum = atoi(argv[1]);
    int ChosenAlgorithm = atoi(argv[2]);
    int Quantum = atoi(argv[3]);
    // printf("(scheduler) CA is %d\n",ChosenAlgorithm);
    printf("ProcNum Sent to Scheduler is : %d\n", ProcNum);

    MyProcess *PCB[ProcNum];

    Q = initQueue();
    RRQ = initQueue();
    MyProcess proc;
    initClk();
    while (true)
    {
        recval = -1;
        if (countRecieved < ProcNum)
        {
            recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
        }

        while (recval != -1)
        {
            MyProcess *P = malloc(sizeof(MyProcess));
            proc.Status = NotCreated;
            P->ID = proc.ID;
            P->Arrival = proc.Arrival;
            P->PID = proc.PID;
            P->Priority = proc.Priority;
            P->RemainingTime = proc.RemainingTime;
            P->RunTime = proc.RunTime;
            P->Status = proc.Status;
            PCB[proc.ID - 1] = P;
            countRecieved++;
            switch (ChosenAlgorithm)
            {
            case 1:
                // printf("enqueuing Process with ID %d at time %d\n", P->ID, getClk());
                pEnqueue(Q, P, P->Priority);
                break;
            case 2:

                break;
            case 3:
                if (isEmpty(Q))
                {
                    enqueue(RRQ, &proc);
                }
                else
                {
                    enqueue(Q, &proc);
                }
                break;

            default:
                break;
            }

            // printf("ID is %d\n",proc.ID);
            // printf("PCB ID is %d\n",PCB[proc.ID - 1].ID);
            // if (proc.ID > 1)
            // {
            //      printf("PCB ID is %d\n",PCB[proc.ID - 2].ID);
            // }
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
            // printf("Dequeueing Process with ID %d at time %d\n", Process->ID, getClk());
            // printf("Status is %d and ID %d \n", Process->Status, Process->ID);

            Process->RemainingTime = Process->RunTime;
            Process->Status = Running;
            int pid = fork();
            if (pid == 0)
            {
                // printf("Forking Process \n at time %d \n", getClk());
                char RemainingTime[20];
                sprintf(RemainingTime, "%d", Process->RemainingTime);
                char SchedulerPid[20];
                sprintf(SchedulerPid, "%d", getpid());
                char ProcessID[20];
                sprintf(ProcessID, "%d", Process->ID);
                char *arguments[] = {"process.out", RemainingTime, SchedulerPid, ProcessID, NULL};
                // printf("ExecV Process with ID %d at time %d\n", Process->ID, getClk());
                // printf("\nArg sent is : %s", n);
                int isFailure = execv("process.out", arguments);
                // printf("Forking Process with isFailure %d\n", isFailure);
                if (isFailure)
                {
                    printf("Error No: %d \n", errno);
                    exit(-1);
                }
            }
            Process->PID = pid;

            Process->Status = Running;
            sleep(__INT_MAX__);
            // printf("exited From Child\n");
            PCB[Index - 1]->Status = Finished;
            Index++;
            break;
        case 2:

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
                if (QTR == -1)
                {
                    QTR = getClk() + Quantum;
                    MyProcess *p = dequeue(Q);
                    Index = p->ID;
                    if (p->Status == NotCreated)
                    {
                        p->RemainingTime = p->RunTime;
                        p->Status = Running;
                        int pid = fork();
                        if (pid == 0)
                        {
                            char RemainingTime[20];
                            sprintf(RemainingTime, "%d", Process->RemainingTime);
                            char SchedulerPid[20];
                            sprintf(SchedulerPid, "%d", getpid());
                            char ProcessID[20];
                            sprintf(ProcessID, "%d", Process->ID);
                            char *arguments[] = {"process.out", RemainingTime, SchedulerPid, ProcessID, NULL};
                            // printf("ExecV Process with ID %d at time %d\n", Process->ID, getClk());
                            // printf("\nArg sent is : %s", n);
                            int isFailure = execv("process.out", arguments);
                            // printf("Forking Process with isFailure %d\n", isFailure);
                            if (isFailure)
                            {
                                printf("Error No: %d \n", errno);
                                exit(-1);
                            }
                        }
                        else
                        {
                            p->PID = pid;
                        }
                    }
                    else
                    {
                        kill(p->PID, SIGCONT);
                    }
                    p->Status = Running;

                    //Psoudo : Dequeue and start process
                }
            }
            if (signalPid != -1)
            {

                for (int i = 0; i < ProcNum; i++)
                {
                    if (PCB[i]->PID == signalPid)
                    {
                        PCB[i]->Status = Finished;
                        countFinished++;
                        printf("Process with ID %d Finsished at Time %d", PCB[i]->ID, getClk());
                        QTR = getClk();
                        signalPid = -1;
                    }
                }
            }

            if (countFinished == ProcNum)
            {
                for (int i = 0; i < ProcNum; i++)
                {
                    free(PCB[i]);
                }

                exit(0);
            }

            if (QTR == getClk())
            {
                //enqueue the process back to Q,
                if (PCB[Index - 1]->Status != Finished)
                {
                    PCB[Index - 1]->Status = Ready;
                    kill(PCB[Index - 1]->PID, SIGSTOP);
                    enqueue(RRQ, PCB[Index - 1]);
                }

                //QTR back to -1
                QTR = -1;
                Index = -1;
            }

            break;

        default:
            break;
        }
    }

    //TODO implement the scheduler :)
    //upon termination release the clock resources

    destroyClk(true);
}
