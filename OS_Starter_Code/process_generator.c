#include "headers.h"

void clearResources(int);


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

    int nums[count][4];

    rewind(fp);
    fscanf(fp, "%*[^\n]\n");

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            fscanf(fp, "%d", &nums[i][j]);
        }
    }

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int ChosenAlgorithm = 0;
    printf("1-HPF   2-SRTN  3-RR \n");
    printf("Choose The Desired Scheduling Algorith(1,2,3):  ");

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
        }
    }
    // fork a child, then call execv to replace this child with a scheduler process
    pid = fork();
    if (pid == 0)
    {
        //execv argv , a null terminated list of strings
        char *arguments[] = {"scheduler.out", NULL};
        int isFailure = execv("scheduler.out", arguments);
        if (isFailure)
        {
            printf("Error No: %d", errno);
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
    bool AllSent = false;
    while (!AllSent)
    {
        
    }
    


    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    destroyClk(true);
    exit(0);
}
