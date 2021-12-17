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
            RR(&Q, &RRQ, f, Quantum, &Index, &signalPid);
            break;
        case 4:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }
            HPF(&Q, f, PCB, &Index);
            break;
        case 5:
            if (isEmpty(Q))
            {
                sleep(1);
                break;
            }

            HPF(&Q, f, PCB, &Index);
            break;

        default:
            break;
        }
    }
    // TODO implement the scheduler :)
    // upon termination release the clock resources

    destroyClk(true);
}
