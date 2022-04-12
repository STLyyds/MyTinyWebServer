#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include "../log/log.h"
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

class util_timer;
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};
// 定时器类
class util_timer
{
public:
    util_timer():prev(NULL), next(NULL) {}
public:
    time_t expire;                      // 超时时间
    void (*cb_func)(client_data *);     // 回调函数
    client_data *user_data;             // 连接资源
    util_timer *prev;                   // 前向定时器
    util_timer *next;                   // 后向定时器
};
// 升序链表
class sort_timer_lst
{
public:
    sort_timer_lst() : head(NULL), tail(NULL) {}
    ~sort_timer_lst()
    {
        util_timer *tmp = head;
        while(tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    void add_timer(util_timer* timer) 
    {
        // 加入的定时器为空
        if(!timer) 
            return ;
        // 当前链表为空
        if(!head)
        {
            head = tail = timer;
            return ;
        }
        // 插入头部情况
        if(timer->expire < head->expire) 
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return ;
        }
        // 插入链表中和尾部情况,调用私有方法
        add_timer(timer, head);
    }
    // 调整定时器，任务发生变化时，调整定时器在链表中的位置
    // 只考虑被调整的定时器的超时时间被延长的情况
    void adjust_timer(util_timer* timer) 
    {
        if(!timer)
        {
            return ;
        }
        util_timer* tmp = timer->next;
        // 如果被调整的定时器已经在尾部或者调整后的超时时间仍小于next则不改变
        if(!tmp || (timer->expire < tmp->expire))
        {
            return ;
        }
        // 如果被调整的定时器在头部，则取出再插入链表
        if(timer == head)
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer(timer, head);
        }
        // 其他位置情况同样取出被调整的定时器，只是插入时从它的下一个位置开始查找
        else
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer, timer->next);
        }
    }
    void del_timer(util_timer* timer)
    {
        if(!timer) 
            return ;
        // 考虑定时器在头部，尾部情况
        if((timer == head) && (timer == tail))
        {
            delete timer;
            head = NULL;
            tail = NULL;
            return ;
        }
        if(timer == head)
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return ;
        }
        if(timer == tail)
        {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return ;
        }
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    // SIGALRM信号每次被触发就在其信号处理函数（如果使用统一事件源，则是主函数）中执行一次tick函数，以处理链表上到期的任务
    void tick()
    {
        if(!head)
        {
            return ;
        }
        LOG_INFO("%s", "timer tick");
        Log::get_instance()->flush();
        time_t cur = time(NULL); // 获取当前系统时间
        util_timer *tmp = head;
        // 从头结点开始依次遍历每个定时器，到期就进行处理，直到遇到一个尚未到期的定时器,就退出
        while(tmp)
        {
            // 定时器用的绝对时间作为超时时值，所以可以直接用系统当前时间进行比较定时器是否到期
            // 判断到期依据：定时器的expire小于当前系统时间
            // 头结点超时时间大于当前系统时间，说明没到期，后面结点也没到期，直接退出
            if(cur < tmp->expire)
            {
                break;
            }
            tmp->cb_func(tmp->user_data);
            head = tmp->next;
            if(head) 
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }
private:
    /*
    插入链表中间：
    需要记录当前节点和前一个节点
    */
    void add_timer(util_timer* timer, util_timer* lst_head)
    {
        util_timer* prev = lst_head;
        util_timer* cur = prev->next;
        while(cur)
        {
            if(timer->expire < cur->expire)
            {
                prev->next = timer;
                timer->prev = prev;
                timer->next = cur;
                cur->prev = timer;
                break;
            }
            prev = cur;
            cur = cur->next;
        }
        // 尾部插入
        if(!cur)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }
private:
    util_timer *head;
    util_timer *tail;
};


#endif