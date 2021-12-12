#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int SchedulerPid;
int x;
clock_t start_t, end_t;
void Cont( int signum){

    start_t = clock();
    end_t = clock();
    printf("signal CONT\n");
    
}
void Stop( int signum){

    start_t = clock();
    end_t = clock();
    remainingtime--;
    printf("signal STOP\n");
    
}
int main(int agrc, char * argv[])
{
    signal(SIGCONT, Cont);
    signal(SIGSTOP, Stop);
   
    start_t = clock();
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
        while ((double)(end_t - start_t)/CLOCKS_PER_SEC < 1.0)
        {
            end_t = clock();
        }
        start_t = clock();
        // x = getClk();
        remainingtime--;
        printf ("Process with ID %d has Remaining time %d At Time %d\n", ID,remainingtime,getClk());
    }
    // printf ("Process with ID %d exiting at Time %d \n", ID,getClk());
    //Notify Scheduler 
    destroyClk(false);
    exit(0);
}


