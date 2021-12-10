#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();


    int msgqid_id , recval ,sendval;

    msgqid_id = msgget(qid , 0644 | IPC_CREAT);

    if(msgqid_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    //Array Size , or number of expected processes.
    int ProcNum = atoi(argv[1]);
    int ChosenAlgorithm = atoi(argv[2]);
    //printf("(scheduler) CA is %d\n",ChosenAlgorithm);
    // printf("Size Sent to Scheduler is : %d\n", ProcNum);
    
    MyProcess PCB [ProcNum];

    Queue pq = initQueue();
     MyProcess proc;
    while (true)
    {
       
        recval = msgrcv(msgqid_id, &proc ,sizeof(proc), 0 ,IPC_NOWAIT);

        if(recval != -1) 
        {
            pEnqueue(pq, proc, proc.RunTime);
            PCB[proc.ID - 1] = proc;
            // printf("ID is %d\n",proc.ID);
            // printf("PCB ID is %d\n",PCB[proc.ID - 1].ID);
            // if (proc.ID > 1)
            // {
            //      printf("PCB ID is %d\n",PCB[proc.ID - 2].ID);
            // }
            
        }
    }
    

    
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}
