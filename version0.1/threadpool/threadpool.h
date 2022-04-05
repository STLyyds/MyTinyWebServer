#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGIMysql/sql_connection_pool.h"
using namespace std;

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    // 向请求队列添加任务
    bool append(T* request);
private:
    /*
    工作线程运行的函数，它不断从工作队列中取出任务并执行之
    静态成员函数没有this指针。非静态数据成员为对象单独维护，但静态成员函数为共享函数，无法区分是哪个对象，因此不能直接访问普通变量成员，也没有this指针
    pthread_create的陷阱
        函数原型中的第三个参数，为函数指针，指向处理线程函数的地址。该函数，要求为静态函数。如果处理线程函数为类成员函数时，需要将其设置为静态成员函数
        pthread_create的函数原型中第三个参数的类型为函数指针，指向的线程处理函数参数类型为(void *),若线程函数为类成员函数，则this指针会作为默认的参数被传进函数中，从而和线程函数参数(void*)不能匹配，不能通过编译。
        静态成员函数就没有这个问题，里面没有this指针
    */
   static void *worker(void* arg);
   void run();
private:
    int m_thread_number;        // 线程池中的线程数
    int m_max_requests;         // 请求队列中允许的最大请求数
    pthread_t *m_threads;       // 描述线程池的数组，其大小为m_thread_number
    list<T*> m_workqueue;       // 请求队列
    locker m_queuelocker;       // 保护请求队列的互斥锁
    sem m_queuestat;            // 用信号量来反映是否有任务需要处理
    bool m_stop;                // 是否结束线程
    connection_pool *m_connPool;// 数据库
};

template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests):
    m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_connPool(connPool)
{
    if(thread_number <= 0 || max_requests <= 0) 
    {
        throw exception();
    }
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads)
    {
        throw exception();
    }
    for(int i=0;i<thread_number;i++) 
    {
        if(pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw exception();
        }
        // 分离线程
        if(pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}

template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
/*
工作线程的执行函数：
    参数是一个线程池指针
    在函数中用指针来调用run函数真正从工作队列中取出任务并执行之
*/
template <typename T>
void* threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run()
{
    while(!m_stop)
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if(!request)
        {
            continue;
        }
        // connectionRAII mysqlcon(&request->mysql, m_connPool);

        // request->process();
    }
}

#endif