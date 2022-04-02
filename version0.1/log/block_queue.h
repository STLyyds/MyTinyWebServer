/**
 * 利用循环数组模拟阻塞队列：m_back = (m_back + 1) % m_max_size;
 * 线程安全通过互斥锁保证
 */

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"

using namespace std;
/* 模板类需要把声明和定义一起写在头文件里 */
template <class T>
class block_queue
{
public:
    block_queue(int max_size = 1000)
    {
        if(max_size < 0)
        {
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    ~block_queue()
    {
        m_mutex.lock();
        if(m_array!=NULL)
        {
            delete [] m_array;
        }
        m_mutex.unlock();
    }
    bool full()
    {
        m_mutex.lock();
        if(m_size >= m_max_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool empty()
    {
        m_mutex.lock();
        if(m_size == 0)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool front(T& value)
    {
        m_mutex.lock();
        if(m_size == 0)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    bool back(T& value)
    {
        m_mutex.lock();
        if(m_size == 0)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    int size()
    {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_size;
        m_mutex.unlock();
        return tmp;
    }
    int max_size()
    {
        int tmp = 0;
        m_mutex.lock();
        tmp = m_max_size;
        m_mutex.unlock();
        return tmp;
    }
    // 往队列添加元素，需要先将所有使用队列的线程唤醒，告诉所有等待的工作线程有资源了
    // 当有元素加入队列时，相当于生产者生产了一个元素
    // 若当前没有线程等待条件变量，则唤醒无意义
    bool push(const T& item)
    {
        m_mutex.lock();
        // 当队列满时，添加元素的线程会被挂起
        if(m_size >= m_max_size)
        {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_back = (m_back+1) % m_max_size;
        m_array[m_back] = item;
        m_size++;
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    // pop时，如果当前队列没有元素，则pop的线程将会挂起，等待条件变量
    bool pop(T& item)
    {
        m_mutex.lock();
        // 有多个消费者时，就需要用while而不是if
        while(m_size <= 0)
        {
            // 当重新抢到互斥锁，phtread_cond_wait返回0
            if(!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }
        // 取出队首元素
        m_front = (m_front+1) % m_max_size;
        item = m_array[m_front];
        --m_size;
        m_mutex.unlock();
        return true;
    }
    // 增加超时机制的pop
    bool pop(T& value, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if(m_size <= 0)
        {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if(!m_cond.timewait(m_mutex.get(), t))
            {
                m_mutex.unlock();
                return false;
            }
        }
        if(m_size <= 0)
        {
            m_mutex.unlock();
            return false;
        }
        m_front = (m_front+1) % m_max_size;
        value = m_array[m_front];
        --m_size;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;

    T* m_array;     // 循环数组模拟队列
    int m_size;     // 数组当前大小 
    int m_max_size; // 数组最大大小
    int m_front;    // 队首指针
    int m_back;     // 队尾指针
};

#endif