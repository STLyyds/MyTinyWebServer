#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

// 封装信号量类，其中包括初始化，销毁，wait等待资源，post释放资源等
class sem{
public:
    // 初始化信号量
    sem()
    {
        if(sem_init(&m_sem, 0, 0) != 0)
        {
            throw exception();
        }
    }
    // 初始化值为num的信号量
    sem(int num)
    {
        if(sem_init(&m_sem, 0, num) != 0)
        {
            throw exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem);
    }
    bool post()
    {
        return sem_post(&m_sem);
    }

private:
    sem_t m_sem;
};
// 封装互斥锁类，其中包括初始化，销毁，上锁，解锁，获得锁变量等
class locker{
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex);
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex);
    }
    pthread_mutex_t* get()
    {
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

// 封装条件变量，包括初始化、销毁、阻塞等待、带有超时时间的阻塞等待、唤醒阻塞线程、唤醒所有阻塞等待的线程
class cond{
public:
    cond()
    {
        if(pthread_cond_init(&m_cond, NULL)!=0)
        {
            throw exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t* mutex)
    {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, mutex);
        // pthread_mutex_unlock(&m_mutex);
        return ret;
    }
    bool timewait(pthread_mutex_t* mutex, struct timespec t)
    {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        pthread_cond_timedwait(&m_cond, mutex, &t);
        // pthread_mutex_unlock(&m_mutex);
        return ret;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond)==0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond)==0;
    }
private:
    // pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif