#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int SchedulerPid;
int x;
void Cont( int signum){

    x = getClk();
}
int main(int agrc, char * argv[])
{
    signal(SIGCONT, Cont);
    initClk();
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
    remainingtime = atoi(argv[1]);
    SchedulerPid = atoi(argv[2]);
    int ID = atoi(argv[3]);

    x = getClk();

    printf("Process with ID %d was created at time %d \n", ID, getClk());
    
    while (remainingtime > 0)
    {
        while (x == getClk())
        {
            
        }
        
        x = getClk();
        remainingtime--;
        printf ("Process with ID %d has Remaining time %d At Time %d\n", ID,remainingtime,getClk());
    }
    printf ("Process with ID %d exiting at Time %d \n", ID,getClk());
    //Notify Scheduler 
    destroyClk(false);
    return 0;
}


