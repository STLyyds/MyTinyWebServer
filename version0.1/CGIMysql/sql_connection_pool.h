#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"

using namespace std;

class connection_pool
{
public:
    MYSQL* GetConnection(); // 获取一个连接
    bool ReleaseConnection(MYSQL* conn);    // 释放一个连接
    int GetFreeConn();      // 获取当前空闲的连接数
    void DestroyPool();     // 销毁连接池

    // 单例模式，采用静态内部类实现
    static connection_pool* GetInstance();

    void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn);

    connection_pool();
    ~connection_pool();
private:
    unsigned int MaxConn;   // 最大连接数
    unsigned int CurConn;   // 当前已使用的连接数
    unsigned int FreeConn;  // 空闲的连接数

private:
    locker lock;            // 互斥锁保证唯一线程对连接池进行操作
    list<MYSQL*> connList;  // 连接池
    sem reserve;            // 使用信号量来通知工作现场连接池有空闲连接

private:
    // 数据库属性信息
    string url;             // 主机地址
    string Port;            // 端口号
    string User;            // 用户名
    string PassWord;        // 密码
    string DatabaseName;    // 数据库名称

};

class connectionRAII
{
public:
    connectionRAII(MYSQL** con, connection_pool* connPool);
    ~connectionRAII();
private:
    MYSQL* connRAII;
    connection_pool* poolRAII;
};

#endif