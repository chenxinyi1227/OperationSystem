#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

/* 捕捉信号 */
void signalHandler(int sig)
{
    printf("sig:%d\n", sig);
    printf("heihei\n");
    sleep(1);
}

int main()
{
    /* 注册信号 */
    /* 1、默认动作 */
    signal(SIGINT, SIG_DFL);

    /* 2、忽略 */
    signal(SIGINT, SIG_IGN);

    /* 3、自定义处理函数 */
    signal(SIGINT, signalHandler);
    while(1)
    {
        printf("hello world\n");
        sleep(2);
    }
    return 0;
}