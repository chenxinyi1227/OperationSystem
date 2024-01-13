#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    pthread_detach(pthread_self());//线程分离

    while(1)
    {
        printf("i am thread_func...\n");
        sleep(1); 
    }
    pthread_exit(NULL);
}

int main()
{
    pthread_t tid;   
    int ret = pthread_create(&tid, NULL, thread_func, NULL);
    if(ret != 0)
    {
        printf("thread create error");
        exit(-1);
    }
    sleep(3);
    pthread_cancel(tid);

    while (1)
    {
        sleep(3);
    }
}