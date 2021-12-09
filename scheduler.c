#include "headers.h"


int main(int argc, char * argv[])
{
    initClk();


    key_t key_id;
    int msgqid_id , recval ,sendval;

    msgqid_id = msgget(qid , 0644 | IPC_CREAT);

    if(msgqid_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    //Array Size , or number of expected processes.
    int ProcNum = atoi(argv[1]);
    // printf("Size Sent to Scheduler is : %d\n", ProcNum);
    
    MyProcess PCB [ProcNum];

    while (true)
    {
        MyProcess proc;
        recval = msgrcv(msgqid_id, &proc ,sizeof(proc), 0 ,!IPC_NOWAIT);

        if(recval == -1)
            perror("Error in receive");
        else
        {
            PCB[proc.ID - 1] = proc;
            printf("ID is %d\n",proc.ID);
            // printf("PCB ID is %d\n",PCB[proc.ID - 1].ID);
        }
    }
    

    
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}
