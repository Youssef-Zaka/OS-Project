#include <stdlib.h>
#include <time.h>
int main(int argc, char * argv[])
{
    
    while(clock() < atoi(argv[1]) * CLOCKS_PER_SEC);
    return 0;
}