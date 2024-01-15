#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 锁资源 */
pthread_mutex_t g_mutex;

/* 条件变量 */
pthread_cond_t g_notEmpty;//生产者给消费者发
pthread_cond_t g_notFull;//消费者给生产者发

/* 链表结点 */
typedef struct Node
{
    int val;
    struct Node *next;
}Node;

/* 链表的头指针 */
Node *head = NULL;

/*  生产者/消费者模型 */

/* 生产者 */
void * produceFunc(void *arg)
{
    int count = 0;
    while(1)
    {
        Node *newNode = (Node *)malloc(sizeof(Node));
        if(newNode == NULL)
        {
            perror("malloc error");
            pthread_exit(NULL);
        }
        memset(newNode, 0, sizeof(Node));

        newNode->val = count++;

        /* 加锁 */
        pthread_mutex_lock(&g_mutex);

        /* 头插 */
        newNode->next = head;
        head = newNode;

        printf("====produce:%ld\t%d\n", pthread_self(), newNode->val);
        pthread_mutex_unlock(&g_mutex);

        /* 信号通知 */
        pthread_cond_signal(&g_notEmpty);

        /*  休眠1-3s不等 */
        usleep(3000);
    }
    /* 线程退出 */
    pthread_exit(NULL);
}

/* 消费者 */
void * consumeFunc(void *arg)
{
    while(1)
    {
        /* 加锁 */
        pthread_mutex_lock(&g_mutex);

        /* 当没有数据的时候，就等待条件 */
        while(head == NULL)//多消费者竞争
        {   
            /* 等待条件变量 */
            pthread_cond_wait(&g_notEmpty, &g_mutex);
        }
        /* 消费数据 */
        Node *delNode = head;
        head = head->next;

        printf("comsue:%ld, \t%d\n", pthread_self(), delNode->val);

        /* 释放内存 */
        if(delNode)
        {
            free(delNode);
            delNode = NULL;
        }
        pthread_mutex_unlock(&g_mutex);
        usleep(5000);
    }
}

int main()
{
    pthread_mutex_init(&g_mutex, NULL);//初始化锁
    pthread_cond_init(&g_notEmpty,NULL);//初始化条件变量
    pthread_cond_init(&g_notFull, NULL);//初始化条件变量

    pthread_t produce_tid;//生产者
    pthread_t consume_tid;//消费者

    int ret = pthread_create(&produce_tid, NULL, produceFunc, NULL);//创建生产者线程
    if(ret != 0)
    {
        printf("thread create error\n");
        exit(-1);
    }

    pthread_create(&produce_tid, NULL, consumeFunc, NULL);//创建生产者线程
    if(ret != 0)
    {
        printf("thread create error\n");
        exit(-1);
    }

    pthread_join(produce_tid, NULL);  /* 线程回收 */
    pthread_join(consume_tid, NULL);

    pthread_mutex_destroy(&g_mutex);//
    pthread_cond_destroy(&g_notEmpty);
    pthread_cond_destroy(&g_notFull);


    return 0;
} 