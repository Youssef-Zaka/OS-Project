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
    //printf("Handler called\n");
    // printf("si code is %d , CLD_EXITED is %d ,CLDStopped is %d\n" , info->si_code , CLD_EXITED,CLD_STOPPED);
    if (info->si_code == CLD_EXITED)
    {
        signalPid = info->si_pid;
        countFinished++;
        //printf("Count finished is %d \n", countFinished);
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
    // Quantum++;
    // printf("(scheduler) CA is %d\n",ChosenAlgorithm);
    //printf("ProcNum Sent to Scheduler is : %d\n", ProcNum);

    MyProcess *PCB[ProcNum];

    Q = initQueue();
    RRQ = initQueue();
    MyProcess proc;

    FILE *f;
    f= fopen("scheduler.log","w");
    fprintf(f, "#At time x process y state arr w total z remain y wait k\n");
    

    initClk();
    while (true)
    {
        // if (countFinished == ProcNum)
        // {

        //     //Printing logic

        //     printf("scheduler EXITING\n");
        //     destroyClk(true);
        //     for (int i = 0; i < ProcNum; i++)
        //     {
        //         free(PCB[i]);
        //     }
        //     exit(0);
        // }
        recval = -1;
        if (countRecieved < ProcNum)
        {
            recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
        }
        else if(countFinished == ProcNum)
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
            P->RemainingTime = proc.RemainingTime;
            P->RunTime = proc.RunTime;
            P->Status = proc.Status;
            P->StartTime = proc.StartTime;
            PCB[proc.ID - 1] = P;
            countRecieved++;
            switch (ChosenAlgorithm)
            {
            case 1:
                // printf("enqueuing Process with ID %d at time %d\n", P->ID, getClk());
                pEnqueue(Q, P, P->Priority);
                break;
            case 2:
                pEnqueue(Q, P, P->RemainingTime);
                break;
            case 3:
                // if (isEmpty(Q))
                // {
                //     enqueue(RRQ, P);
                // }
                // else
                // {
                //     enqueue(Q, P);
                // }
                enqueue(Q,P);
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
            Process->StartTime = getClk();
            int pid = fork();
            fprintf(f, "At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , Process->ID , Process->Arrival , Process->RunTime ,
                Process->RemainingTime,Process->StartTime - Process->Arrival);
             
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
                printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , Process->ID , Process->Arrival , Process->RunTime ,
                Process->RemainingTime,Process->StartTime - Process->Arrival);   
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
            fprintf(f, "At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n",getClk() , Process->ID , Process->Arrival ,
            Process->RunTime , 0,Process->StartTime - Process->Arrival , getClk() - Process->Arrival ,
            ((float)getClk() - (float)Process->Arrival)/(float)Process->RunTime);
            printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n",getClk() , Process->ID , Process->Arrival ,
            Process->RunTime , 0,Process->StartTime - Process->Arrival , getClk() - Process->Arrival ,
            ((float)getClk() - (float)Process->Arrival)/(float)Process->RunTime);
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
                MyProcess *p = dequeue(Q);
                Index = p->ID;
                if (p->Status == NotCreated)
                {
                    //printf("Forking ID %d at time %d\n", Index, getClk());
                    p->RemainingTime = p->RunTime;
                    p->Status = Running;
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
                        // printf("Executing ID %d at time %d\n", Index, getClk());
                        char *arguments[] = {"process.out", RemainingTime, SchedulerPid, ProcessID, NULL};
                        // printf("ExecV Process with ID %d at time %d\n", Process->ID, getClk());
                        // printf("\nArg sent is : %s", n);
                        // printf("Executing ID %d at time %d\n", Index, getClk());
                        printf("At time %d\t process %d\t started arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , p->ID , p->Arrival , p->RunTime ,
                        p->RemainingTime,p->StartTime - p->Arrival);   
                        int isFailure = execv("process.out", arguments);
                        // printf("Forking Process with isFailure %d\n", isFailure);
                        if (isFailure)
                        {
                            printf("Error No: %d \n", errno);
                            exit(-1);
                        }
                    }
                    sleep(Quantum);
                    //printf("Stopping Signal with ID: %d \n", p->ID);
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
                    // printf("After Sleeping Quantum : %d \n", Quantum);
                    // printf("Signal Pid is  : %d for ID %d\n", signalPid, p->ID);
                    if (signalPid)
                    {
                        p->Status = Finished;
                        printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n",getClk() , p->ID , p->Arrival ,
                    p->RunTime , 0,p->StartTime - p->Arrival , getClk() - p->Arrival ,
                    ((float)getClk() - (float)p->Arrival)/(float)p->RunTime);
                    }
                    else
                    {
                        p->Status = Ready;
                        printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , p->ID , p->Arrival , p->RunTime ,
                        p->RemainingTime,p->StartTime - p->Arrival);   
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
                    printf("At time %d\t process %d\t Continued arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , p->ID , p->Arrival , p->RunTime ,
                        p->RemainingTime,p->StartTime - p->Arrival);   
                    kill(p->PID, SIGCONT);
                    // sleep(0);
                    // sleep(0);
                    // sleep(0);
                    // sleep(0);
                    // printf("Sleeping Quantum : %d \n", Quantum);
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
                    // printf("Signal Pid is  : %d \n", signalPid);
                    if (signalPid)
                    {
                        p->Status = Finished;
                         printf("At time %d\t process %d\t finished arr \t%d\t total %d\t remain %d\t wait %d\t TA %d\t WTA %.2f\t\n",getClk() , p->ID , p->Arrival ,
                    p->RunTime , 0,p->StartTime - p->Arrival , getClk() - p->Arrival ,
                    ((float)getClk() - (float)p->Arrival)/(float)p->RunTime);
                    }
                    else
                    {
                        p->Status = Ready;
                    }
                    signalPid = 0;
                    if (p->Status != Finished)
                    {
                        printf("At time %d\t process %d\t Stopped arr \t%d\t total %d\t remain %d\t wait %d\t\n",getClk() , p->ID , p->Arrival , p->RunTime ,
                        p->RemainingTime,p->StartTime - p->Arrival);   
                        kill(p->PID, SIGSTOP);
                        enqueue(RRQ, p);
                    }
                }

                // Psoudo : Dequeue and start process
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
