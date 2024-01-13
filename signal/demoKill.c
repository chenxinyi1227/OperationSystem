#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#define SIZE 5
int main()
{
    pid_t pid = fork();//创建进程
    if(pid == -1)
    {
        perror("fork error");
        _exit(-1);
    }
    else if(pid == 0)
    {
        printf("child id:%d\n", getpid());
        /* 子进程 */
        int idx = 0;
        for(idx = 0; idx < SIZE; idx++)
        {
            printf("i am child process\n");
            sleep(1);
        }
    }
    else
    {
        printf("pid:%d\n", pid);

        /* 父进程 */
        printf("i am parent process id:%d\n",getpid());
        sleep(2);

        printf("kill sub process now\n");
        kill(pid, SIGINT);
    }
    /* 阻塞 */
    while(1)
    {
        sleep(3);
    }

    return 0;
}