#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#define SIZE 5
int main()
{
    for(int idx = 0; idx < SIZE; idx++)
    {
        if(idx == 5)
        {
            /* 给当前进程发送Ctrl + C */
            raise(SIGINT);
            /* 等价于：kill */
            kill(getpid());
        }
        printf("idx = %d\n", idx);
        sleep(1);
    }
    return 0;
}