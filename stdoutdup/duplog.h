#ifndef _DUP_LOG_H
#define _DUP_LOG_H

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include "pthread.h"

#ifdef _WIN32

#ifdef IS_OLD_MSVCPP
#include <winsock.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <process.h>
#include <Windows.h>

#else

#include <unistd.h>
#include <netinet/tcp.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#endif

using namespace std;

#define DEF_DUPLOG_PORT 9999
#define MAX_CLIENTS 1024

#define MD_DUPLOG "duplog"
#define WLOG_INFO(_submodule_, format, ...)	printf("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_WARNING(_submodule_, format, ...)	printf("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_ERR(_submodule_, format, ...)	printf("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)

class util
{
public:
	static void Sleep(int ims)
	{
#ifdef _WIN32
		::Sleep(ims)
#else
		usleep(ims * 1000);
#endif
	}
};

class Scope_Mutex_C
{
private:
	pthread_mutex_t& mMutex;
	Scope_Mutex_C(const Scope_Mutex_C &);
	Scope_Mutex_C& operator = (const Scope_Mutex_C&);

public:
	explicit Scope_Mutex_C(pthread_mutex_t& m)
	: mMutex(m)
	{
		pthread_mutex_lock(&mMutex);
	}

	~Scope_Mutex_C()
	{
		pthread_mutex_unlock(&mMutex);
	}
};

class CClientManager
{
public:
	CClientManager();
	~CClientManager();

	bool AddClient(int fd);
	void RemoveClient(int fd);
	void GetClients(vector<int> &fds);
	bool IsEmpty();
	int Size();

private:
	vector<int> m_vctfdClients;
	pthread_mutex_t m_lockClients;
};

class CDupLog
{
public:
	CDupLog(int iPort = DEF_DUPLOG_PORT);
	virtual ~CDupLog();

public:
	static CDupLog* CreateDupLog();

	static void* threadRun(void *pPram);		// 等连接线程
	int run();
	
	static void* threadSend(void *pPram);		// 发数据线程
	void Send();
	
	bool InitPipe();
	void ReleasePipe();
	int DupStdOut();
	int RecoverStdOut();
	bool IsDupStdOut() { return (m_fdOrignStdOut >= 0); };

public:
	bool m_bExit;
	pthread_t m_thRunID;
	pthread_t m_thSendID;

private:
	CClientManager m_mrgClients;
	int m_fdServ;			// 监听套接字
	int m_iPort;

	int m_fdOrignStdOut;	// 标准输出的原文件描述符,不出意外应该是1,重定向后仍可通过该文件描述符做标准输出打印信息

#ifdef _WIN32
	FILE m_FStdOut;

	HANDLE m_hPipe[2];		// m_hPipe[0]管道读端句柄, m_hPipe[1]管道写端句柄
	FILE *g_pFPipeW;
#else
	int m_fdsPipe[2];		// m_fdsPipe[0]是读端 fdsPipe[1]是写端
#endif

};

#endif
