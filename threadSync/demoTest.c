#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

pthread_mutex_t g_mutex;

/* 打印机，公共资源 */
void printer(char *str)
{
    /* 加锁 */
    pthread_mutex_lock(&g_mutex);
    while(*str != '\0')
    {
        putchar(*str);
        fflush(stdout);
        str++;
        /* 休眠是为了让出CPU */
        usleep(100);
    }
    /* 解锁 */
    pthread_mutex_unlock(&g_mutex);
    printf("\n");
}

void *thread_fun_1(void *arg)
{
   char *str = "hello";
   printer(str);
}

void *thread_fun_2(void *arg)
{
    char *str = "world";
    printer(str);
  
}
    
int main()
{   
    
    /* 参数2：锁默认属性 */
    pthread_mutex_init(&g_mutex, NULL);

    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, thread_fun_1, NULL);
    pthread_create(&tid2, NULL, thread_fun_2, NULL);
  

    /* 主线程回收子线程的资源 */
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    pthread_mutex_lock(&g_mutex);

    /* 释放锁资源：必须解锁之后才可以操作此函数 */
    int ret = pthread_mutex_destroy(&g_mutex);
    if (ret == EBUSY)
    {
        printf("pthread_mutex_destory error\n");
        exit(-1);
    }
    return 0;
}