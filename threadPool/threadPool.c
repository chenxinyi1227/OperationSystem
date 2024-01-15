#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>

#define DEFAULT_MIN_THREAD 5
#define DEFAULT_MAX_THREAD 10
#define DEFAULT_QUEUE_CAPACITY 100

#define TIME_INTERVAL 5

/* 扩容/缩容每次增加或减少的默认线程池 */
#define DEFAULT_VARY_THREADS 3

enum STAUS_CODE
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    ACCESS_INVAILD,
    UNKNOWN_ERROR
};

/* 静态函数 */
static int threadExitClrResoures(threadpool_t *pool);
static void * threadHander(void *arg);
static void *managerHander(void *arg);

/* 线程退出清理资源 */
static int threadExitClrResoures(threadpool_t *pool)
{
    for(int idx = 0; idx < pool->maxThreads; idx++)
    {
        if(pool->threadIds[idx] == pthread_self())
        {
            pool->threadIds[idx] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}

/* 本质是一个消费者 */
static void * threadHander(void *arg)
{
    /* 强制类型转换 */
    threadpool_t *pool = (threadpool_t *)arg;
    while(1)//多个竞争用while
    {
        pthread_mutex_lock(&pool->mutexpool);
        while(pool->queueSize == 0 && pool->shutDown != 0)
        {
            /* 等待一个条件变量，由生产者发送 */
            pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);

            if(pool->exitThreadNums > 0)
            {
                pool->exitThreadNums--; //离开数减1
                if(pool->liveThreadNUms > pool->minThreads)
                {
                    threadExitClrResoure(pool);//线程退出
                }
            }
        }

        if(pool->shutDown)
        {
            threadExitClrResoures(pool);//线程退出
        }

        /* 意味着任务队列有任务 */
        task_t tmptask = pool->taskQueue[pool->queueFront];
        pool->queueFront = (pool->queueFront + 1) % (pool->queueCapacity);//循环队列 队尾->队头

        pool->queueSize--; /* 任务数减一 */

        pthread_mutex_unlock(&pool->mutexpool);//解锁
        pthread_cond_signal(&pool->notFull);//发送信号给生产者表示可以继续生成

        /* 为了提升我们性能，在创建一把只维护线程数 */
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNUms++;
        pthread_mutex_unlock(&pool->mutexBusy);

        /* 执行钩子函数 - 回调函数 */
        tmptask.worker_hander(tmptask.arg);
        /* 释放堆空间todo... */

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadNUms--;//
        pthread_mutex_unlock(&pool->mutexBusy);
        /* 释放堆空间todo.. */

    }
    pthread_exit(NULL);
}

/* 管理者线程 */
static void *managerHander(void *arg)
{   
    /* 强制转换类型 */
    threadpool_t *pool = (threadpool_t *)arg;
    
    while(pool->exitThreadNums == 0)
    {
        sleep(TIME_INTERVAL);

        pthread_mutex_lock(&pool->mutexpool);
        /* 任务队列任务数 */
        int taskNUms = pool->queueSize;
        /* 存活的线程数 */
        int liveThreadNUms = pool->liveThreadNUms;
        pthread_mutex_unlock(&pool->mutexpool);

        pthread_mutex_lock(&pool->mutexBusy);
        /* 忙的的线程数 */
        int busyThreadNUms = pool->busyThreadNUms;
        pthread_mutex_unlock(&pool->mutexBusy);

        /* 扩容：扩大线程池里面的线程数（上限不要超过maxthreads） */
        /* 任务数 > 存活的线程数 && 存活的线程数 < maxThreads */
        if(taskNUms > liveThreadNUms && liveThreadNUms < pool->maxThreads)
        {
            pthread_mutex_lock(&pool->mutexpool);
            /* 计数 */
            int count = 0;
            /* 一次扩3个线程 */
            int ret = 0;
            for(int idx = 0; idx < pool->maxThreads && count < DEFAULT_VARY_THREADS && liveThreadNUms <= pool->maxThreads; idx++)
            {
                /* 能够用的索引位置 */
                if(pool->threadIds[idx]  == 0)
                {
                    ret = pthread_create(&pool->threadIds[idx], NULL, threadHander, NULL);
                    if(ret != 0)
                    {
                        perror("thread create error");
                        /* todo... */
                    }
                    count++;                //计数加1
                    pool->liveThreadNUms++;//存活的线程数加1
                }
            }
            pthread_mutex_unlock(&pool->mutexpool);
        }

        /* 缩容:减少线程池里面的线程数（上限不要超过minthreads）*/
        /* 忙的线程数 * 2 < 存活的线程数 && 存活的线程 > minThreads */
        if((busyThreadNUms >> 1) < liveThreadNUms && liveThreadNUms > pool->minThreads)
        {
            pthread_mutex_lock(&pool->mutexpool);
            /* 让摸鱼的自己走 */
            /* 离开的线程数 */
            pool->exitThreadNums = DEFAULT_VARY_THREADS;
            for(int idx = 0; idx < DEFAULT_VARY_THREADS; idx++)
            {
                pthread_cond_signal(&pool->notEmpty);//发一个信号
            }
            pthread_mutex_unlock(&pool->mutexpool);
        }
    }
    /* 线程关闭 */
    pthread_exit(NULL);
}

/* 线程池初始化 */
int threadPoolInit(threadpool_t *pool, int minThreads, int maxThreads, int queueCapacity)
{
    if(pool == NULL)
    {
        return NULL_PTR;
    }

    do
    {
        /* 判断合法性 */
        if(minThreads < 0 || maxThreads < 0 || minThreads >= maxThreads)
        {
            minThreads = DEFAULT_MIN_THREAD;
            maxThreads = DEFAULT_MAX_THREAD;
        }

        /* 更新线程池属性 */
        pool->maxThreads = maxThreads;
        pool->minThreads = minThreads;
        pool->busyThreadNUms = 0;  //初始化时，忙碌的线程数为0

        /* 判断合法性 */
        if(queueCapacity <= 0)
        {
            queueCapacity = DEFAULT_QUEUE_CAPACITY;
        }

        /* 更新线程池 任务队列属性 */
        pool->queueCapacity = queueCapacity;
        pool->taskQueue = (task_t *)malloc(sizeof(task_t) * pool->queueCapacity);
        if(pool->taskQueue == NULL)
        {
            perror("malloc error");
            break;
        }
        memset(pool->taskQueue, 0, sizeof(task_t) * pool->queueCapacity);
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSize = 0;
        
        /* 为线程ID分配堆空间 */
        pool->threadIds = (pthread_t)malloc(sizeof(pthread_t) * pool->maxThreads);
        if(pool->threadIds == NULL)//NULL 判断空闲
        {
            perror("malloc error");
            break;
        }
        memset(pool->threadIds, 0, sizeof(pthread_t) * pool->maxThreads);

        int ret = 0;
        ret = pthread_create(&(pool->magagerThread), NULL, managerHander, pool);
        if (ret != 0)
        {
            break;
        }

        /* 创建线程 */
        for(int idx = 0; idx < pool->maxThreads; idx++)
        {
            /* 如果线程ID号为0. 那个这个位置可以用. */
            if (pool->threadIds[idx] == 0)
            {
                ret = pthread_create(&(pool->threadIds[idx]), NULL, threadHander, NULL);
                if (ret != 0)
                {
                    perror("thread create error");
                    break;
                }
            }
        }
        /* 此ret是创建线程函数的返回值. */
        if (ret != 0)
        {
            break;
        }

        /* 存活的线程数 等于 开辟的线程数 */
        pool->liveThreadNUms = pool->minThreads;

        /* 初始化锁资源 */
        pthread_mutex_init(&pool->mutexpool, NULL);
        pthread_mutex_init(&pool->mutexBusy, NULL);

        #if 0
        /* 初始化条件变量资源 */
        pthread_cond_init(&pool->notEmpty, NULL);
        pthread_cond_init(&pool->notFull, NULL);
        #endif

        /* 初始化条件变量资源 */
        if(pthread_cond_init(&(pool->notEmpty), NULL) != 0 || pthread_cond_init(&(pool->notFull), NULL) != 0)
        {
            perror("thread cond error");
            break;
        }
        return ON_SUCCESS;
    } while (0);
    /* 程序执行到这地方，上面一定有失败 */

    /* 回收堆空间 */
    if (pool->taskQueue != NULL)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }

    /* 回收管理线程资源 */
    pthread_join(pool->magagerThread, NULL);

    /* 回收线程资源 */
    for (int idx = 0; idx < pool->minThreads; idx++)
    {
        if (pool->threadIds[idx] != 0)
        {
            pthread_join(pool->threadIds[idx], NULL);
        }
    }

    if (pool->threadIds != NULL)
    {
        free(pool->threadIds);
        pool->threadIds = NULL;
    }
    /* 释放锁资源 */
    pthread_mutex_destroy(&(pool->mutexpool));
    pthread_mutex_destroy(&(pool->mutexBusy));

    /* 释放条件变量的资源 */
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    return UNKNOWN_ERROR;
}

/* 线程增加任务 */
int threadPoolAddTask(threadpool_t *pool, void *(worker_hander)(void *), void *arg)
{
    if(pool == NULL)
    {
        return NULL_PTR;
    }
    pthread_mutex_lock(&(pool->mutexpool));//上锁
    while(pool->queueSize == pool->queueCapacity)//任务队列满了
    {
        pthread_cond_wait(&(pool->notFull), &(pool->mutexpool));
    }

    /* 任务到这个地方一定有位置可以放任务 */
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;

    /* 队尾向后移动 */
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    /* 任务数加1 */
    pool->queueSize++;

    pthread_mutex_unlock(&(pool->mutexpool));
    /* 发信号 */
    pthread_cond_signal(&pool->notFull);

    return ON_SUCCESS;
}

/* 线程池销毁 */
int threadPoolDestory(threadpool_t *pool)
{
    int ret;
    /* 标志位 */
    pool->shutDown = 1;

    /* 广播 */
    pthread_cond_broadcast(&pool->notEmpty);
    return 1;
}
