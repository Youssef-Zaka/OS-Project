#include "headers.h"

void clearResources(int);

int msgqid_id;

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

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

    rewind(fp);
    fscanf(fp, "%*[^\n]\n"); //skip first line

    for (int i = 0; i < count; i++)
    {
        int nums[4];
        for (int j = 0; j < 4; j++)
        {
            fscanf(fp, "%d", &nums[j]);
        }
        procs[i].ID = nums[0];
        procs[i].Arrival = nums[1];
        procs[i].RunTime = nums[2];
        procs[i].Priority = nums[3];

        //printf("%d %d %d %d\n", procs[i].ID,procs[i].Arrival, procs[i].RunTime ,procs[i].Priority);

    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int ChosenAlgorithm = 0;
    printf("1-HPF   2-SRTN  3-RR \n");
    printf("Choose The Desired Scheduling Algorith(1,2,3):  ");
    scanf("%d", &ChosenAlgorithm);
    while (ChosenAlgorithm != 1 && ChosenAlgorithm != 2 && ChosenAlgorithm != 3)
    {
        printf("Incorrect input, please choose between 1 ,2 and 3: ");
        scanf("%d", &ChosenAlgorithm);
    }
    printf("\n");

    // 3. Initiate and create the scheduler and clock processes.
    // fork a child, then call execv to replace this child with a clock process
    int pid = fork();
    if (pid == 0)
    {
        //execv argv , a null terminated list of strings
        char *arguments[] = {"clk.out", NULL};
        int isFailure = execv("clk.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d", errno);
            exit(-1);
        }
    }
    // fork a child, then call execv to replace this child with a scheduler process
    pid = fork();
    if (pid == 0)
    {
        // execv argv , a null terminated list of strings
        char n[GetDigitsOfInt(count)];
        sprintf(n, "%d", count);
        char ca[GetDigitsOfInt(ChosenAlgorithm)];
        sprintf(ca, "%d", ChosenAlgorithm);
        char *arguments[] = {"scheduler.out",n,ca,NULL};
        // printf("\nArg sent is : %s", n);
        int isFailure = execv("scheduler.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d \n", errno);
            exit(-1);
        }
    }

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);

    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.

    int sendval, recval;

    msgqid_id = msgget(qid , 0644 | IPC_CREAT);

    if(msgqid_id == -1)
    {
        perror("Error in creating queue");
        exit(-1);
    }

    int Iteration =0;
    while(Iteration < count)
    {
        while (procs[Iteration].Arrival == x && Iteration < count)
        {
            printf("Clock is %d\n",x);
            sendval = msgsnd(msgqid_id, &procs[Iteration] , sizeof(procs[Iteration]) , !IPC_NOWAIT);

            if(sendval == -1)
                perror("Error in send");
            Iteration++;   
        }
        x = getClk();
        
    }
    
    


    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(msgqid_id , IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}
