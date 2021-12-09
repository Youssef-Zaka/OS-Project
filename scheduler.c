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


    while (1)
    {
        MyProcess proc;
        recval = msgrcv(msgqid_id, &proc ,sizeof(proc), 0 ,!IPC_NOWAIT);

        if(recval == -1)
            perror("Error in receive");
        else
            printf("ID is %d\n",proc.ID);
    }
    

    
    //TODO implement the scheduler :)
    //upon termination release the clock resources
    
    destroyClk(true);
}
