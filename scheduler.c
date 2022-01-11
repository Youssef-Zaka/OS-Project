#include "headers.h"

int QTR = -1;

Queue Q;
Queue RRQ;
int signalPid = 0;
int signalID = 0;
int countFinished = 0;
int ProcNum;


int memSizes[9] = {2,4,8,16,32,64,128,256,512};
void EndOfProcess(int sig, siginfo_t *info, void *context)
{
    signalPid = 0;
    if (info->si_code == CLD_EXITED)
    {
        signalPid = info->si_pid;
        countFinished++;
    }
}





//Memory Functions 


int CalculateSize(int memsize){
    for (int i = 0; i < 8; i++)
    {
        if (memsize <= memSizes[i])
        {
            return memSizes[i];
        }   
    }
    return memSizes[8];
}






////////////////////////
int main(int argc, char *argv[])
{
    //Signal Action handler
    struct sigaction sa;
    // SA_NOCLDSTOP will make the sigchld called only upon termination
    // SA_SIGINFO used to get the process ID upon termination
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
    sa.sa_sigaction = EndOfProcess;
    sigaction(SIGCHLD, &sa, NULL);

    int msgqid_id, recval, sendval;
    // create msg queue 
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
    memf =  fopen("memory.log", "w");
    //First comment in the log file
    fprintf(f, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    fprintf(memf, "#At\ttime\tx\tallocated\ty\tbytes\tfor\tprocess\tz\tfrom\ti\tto\tj\n");

    //Remaining time of the running process, used In SRTN
    int CurrentRemaining = __INT_MAX__;
    //The running process, used in SRTN
    MyProcess *CurrentP = NULL;

  
    for (int i = 0; i < MEMSIZE; i++)
    {
        memory[i] = 'f';
    }

    
    

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
            CalculatePerfs(PCB,ProcNum);
            fclose(f);
            fclose(memf);
            destroyClk(true);
            exit(0);
        }

        //recieve all other processes that arrived, enqueue them.

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
        P->Wait = 0;
        P->TA = 0;
        P->WTA = 0;
        PCB[proc.ID - 1] = P;
        countRecieved = countRecieved + 1;
        P->realMemSize = proc.MemSize;
        P->MemSize = CalculateSize(proc.MemSize);
        P->index = -1;
        AllocateMem(P);
        switch (ChosenAlgorithm)
        {
        case 1: // For HPF
            pEnqueue(Q, P, P->Priority);
            break;
        case 2:// For SRTN
            pEnqueue(Q, P, P->RunTime);
            break;
        case 3:// For RR
            enqueue(Q, P);
            break;
        case 4:// For FCFS
            enqueue(Q, P);
            break;
        case 5: // For SJF
            pEnqueue(Q, P, P->RunTime);
            break;

        default:
            break;
        }
        recval = msgrcv(msgqid_id, &proc, sizeof(proc), 0, IPC_NOWAIT);
    }

        //RecieveProcess(&Q, proc, recval, msgqid_id, ChosenAlgorithm, PCB, &countRecieved);

        switch (ChosenAlgorithm)
        {
        case 1:
            if (isEmpty(Q)) // if queue is empty -> check after 1 sec for new events 
            {
                sleep(1);
                break;
            }

            HPF(&Q, f);
            break;
        case 2:
            SRTN(&Q, &CurrentP, &CurrentRemaining, f, &signalPid);
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
            RR(&Q, &RRQ, f, Quantum, &signalPid);
            break;
        case 4:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }
            HPF(&Q, f); // HPF and FCFS and SJF are non-premitive -> can be implemented using 1 function only 
            break;
        case 5:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }

            HPF(&Q, f);
            break;

        default:
            break;
        }
    }
     // TODO implement the scheduler :)
    // upon termination release the clock resources

    destroyClk(true);
    return 0;
}

