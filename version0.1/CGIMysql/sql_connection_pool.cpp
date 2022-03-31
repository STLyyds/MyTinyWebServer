#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool()
{
    this->CurConn = 0;
    this->FreeConn = 0;
}

connection_pool* connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

// 初始化，将数据库信息绑定，循环创建连接并放入连接池
void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn)
{
    this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DataBaseName;
    lock.lock();
    for(int i=0;i<MaxConn;i++) {
        MYSQL* conn = NULL;
        conn = mysql_init(conn);
        if(conn == NULL)
        {
            cout << "Error:" << mysql_error(conn);
            exit(1);
        }
        conn = mysql_real_connect(conn, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0);
        if(conn == NULL)
        {
            cout << "Error:" << mysql_error(conn);
        }
        connList.push_back(conn);
        ++FreeConn;
    }
    // 初始化信号量
    reserve = sem(FreeConn);

    this->MaxConn = FreeConn;

    lock.unlock();
}

// 获取一个连接
MYSQL* connection_pool::GetConnection()
{
    MYSQL* conn = NULL;
    if(connList.empty()) {
        return NULL;
    }
    reserve.wait();
    lock.lock();
    conn = connList.front();
    connList.pop_front();
    --FreeConn;
    ++CurConn;
    lock.unlock();
    return conn;
}

// 释放当前连接
bool connection_pool::ReleaseConnection(MYSQL* conn)
{
    if(conn == NULL)
        return false;
    lock.lock();

    connList.push_back(conn);
    ++FreeConn;
    --CurConn;

    lock.unlock();

    reserve.post();

    return true;
}

// 销毁连接池，并销毁其中的对象
void connection_pool::DestroyPool()
{
    lock.lock();
    if(!connList.empty())
    {
        list<MYSQL*>::iterator it;
        for(it = connList.begin();it!=connList.end();++it) 
        {
            MYSQL* con = *it;
            mysql_close(con);
        }
        CurConn = 0;
        FreeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

// 获得当前的空闲连接数
int connection_pool::GetFreeConn()
{
    return this->FreeConn;
}

connection_pool::~connection_pool()
{
    DestroyPool();
}

// 用RAII机制封装连接池和连接
connectionRAII::connectionRAII(MYSQL** conn, connection_pool* connPool)
{
    *conn = connPool->GetConnection();
    connRAII = *conn;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->DestroyPool();
}
