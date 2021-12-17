#include "headers.h"


//Functions
void clearResources(int);


//Message queue ID 
int msgqid_id;

int main(int argc, char *argv[])
{
    //set handlers
    signal(SIGINT, clearResources);
    signal(SIGCHLD, clearResources);

    

    
    // TODO Initialization
    // 1. Read the input files.
    FILE *fp;

    int count = -1; // for getting num of lines
    char c;

    fp = fopen("processes.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    for (c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c == '\n') // Increment count if this character is newline
            count += 1;
    }

    MyProcess procs[count];

    ReadFromFile(procs,count,fp);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int ChosenAlgorithm = 0;
    int Quantum = 0;
    GetChosenAlgo(&ChosenAlgorithm,&Quantum);
    CreateClock();
    
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // 3. Initiate and create the scheduler and clock processes.
    CreateScheduler(count,ChosenAlgorithm,Quantum);
   
    // To get time use this
    int x = getClk() ;

    //Create Message Queue, and any needed initializations
    int sendval, recval;
    msgqid_id = msgget(qid , 0644 | IPC_CREAT);
    if(msgqid_id == -1)
    {
        perror("Error in creating queue");
        exit(-1);
    }
    int Iteration =0;
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time. 

    bool isFirstSent = false;
    while(Iteration < count)
    {
        while (Iteration < count && procs[Iteration].Arrival <= x)
        {
            // printf("Clock is %d\n",x);
            sendval = msgsnd(msgqid_id, &procs[Iteration] , sizeof(procs[Iteration]) , !IPC_NOWAIT);
            if(!isFirstSent) isFirstSent = true;
            if(sendval == -1)
                perror("Error in send");
            Iteration++;   
        }
        if(isFirstSent) sleep(1);
        x = getClk();
    }
    //sleep untill scheduler exits
    sleep(__INT_MAX__);
    return(0);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(msgqid_id , IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}

