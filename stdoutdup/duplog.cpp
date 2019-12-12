/************************************************************************/
/*
开线程默认监听9999端口，等待连接。
当有新连接建立时，将原先的连接关闭，创建管道，将标准输出重定向到管道写端，
另外开线程从管道读端读数据发送给当前连接者。
*/
/************************************************************************/
#include "duplog.h"
#include <fcntl.h>
#include "string.h"
using namespace std;


#ifdef _WIN32

#define DUP_SOCKET_CLOSE closesocket
#define DUP_NOSIGNAL 0

#else

#define DUP_SOCKET_CLOSE close

#ifdef __linux__
#define DUP_NOSIGNAL MSG_NOSIGNAL
#else
#define DUP_NOSIGNAL SO_NOSIGPIPE
#endif

#endif

CClientManager::CClientManager()
{
	pthread_mutex_init(&m_lockClients, 0);
}

CClientManager::~CClientManager()
{
	pthread_mutex_destroy(&m_lockClients);
}

bool CClientManager::AddClient(int fd)
{
	Scope_Mutex_C lock(m_lockClients);

	if (m_vctfdClients.size() >= MAX_CLIENTS)
	{
		return false;
	}

	for (int i = m_vctfdClients.size() - 1; i >= 0; i--)
	{
		if (m_vctfdClients[i] == fd)
		{
			return true;
		}
	}

	m_vctfdClients.push_back(fd);
	return true;
}

void CClientManager::RemoveClient(int fd)
{
	Scope_Mutex_C lock(m_lockClients);

	for (int i = m_vctfdClients.size() - 1; i >= 0; i--)
	{
		if (m_vctfdClients[i] == fd)
		{
			m_vctfdClients.erase(m_vctfdClients.begin() + i);
		}
	}
}

void CClientManager::GetClients(vector<int> &fds)
{
	Scope_Mutex_C lock(m_lockClients);
	fds = m_vctfdClients;
}

bool CClientManager::IsEmpty()
{
	// Scope_Mutex_C lock(m_lockClients);
	return m_vctfdClients.empty();
}

int CClientManager::Size()
{
	Scope_Mutex_C lock(m_lockClients);
	return m_vctfdClients.size();
};


CDupLog::CDupLog(int iPort/* = DEF_DUPLOG_PORT*/)
: m_bExit(false)
, m_fdServ(-1)
, m_iPort(iPort)
, m_fdOrignStdOut(-1)
{
#ifdef WIN32
	m_hPipe[0] = NULL;
	m_hPipe[1] = NULL;
	g_pFPipeW = NULL;
#else
	m_fdsPipe[0] = -1;
	m_fdsPipe[1] = -1;
#endif

#ifdef _WIN32
	WSADATA wsadata;
	WORD wVersionRequested;

#ifdef IS_OLD_MSVCPP
	wVersionRequested = MAKEWORD(1, 1);
#else
	wVersionRequested = MAKEWORD(2, 2);
#endif

	if (WSAStartup(wVersionRequested, &wsadata) != 0)
	{
		WLOG_ERR(MD_DUPLOG, "WSAStartup failed\n");
	}
#endif
}

CDupLog::~CDupLog()
{
	if (m_fdServ > -1)
	{
		m_bExit = true;
		DUP_SOCKET_CLOSE(m_fdServ);
		m_fdServ = -1;
		pthread_join(m_thRunID, NULL);
		pthread_join(m_thSendID, NULL);
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

// static
CDupLog* CDupLog::CreateDupLog()
{
	CDupLog *tmp = new CDupLog(DEF_DUPLOG_PORT);

	if (tmp != NULL && pthread_create(&tmp->m_thRunID, NULL, CDupLog::threadRun, (void *)tmp) == 0)
	{
	}
	else if (tmp != NULL)
	{
		delete tmp;
		tmp = NULL;
	}

	return tmp;
}

// static
void* CDupLog::threadRun(void *pPram)
{
	CDupLog *tmp = static_cast<CDupLog *>(pPram);

	if (tmp != NULL)
	{
		tmp->run();
	}

	WLOG_INFO(MD_DUPLOG, "run-thread exit\n");
	return 0;
}

int CDupLog::run()
{
	m_iPort = (m_iPort > 0)? m_iPort : DEF_DUPLOG_PORT;
	sockaddr_in tagServAddr;
	memset(&tagServAddr, 0, sizeof(tagServAddr));
	tagServAddr.sin_family = AF_INET;
	tagServAddr.sin_port = htons(m_iPort);
	tagServAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((m_fdServ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		WLOG_WARNING(MD_DUPLOG, "socket create failed\n");
		return -1;
	}

	int option = 1;

#ifdef _WIN32
	setsockopt(m_fdServ, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option));
	// setsockopt(m_fdServ, SOL_SOCKET, SO_NOSIGPIPE, (const char *)&option, sizeof(option));
#else
	setsockopt(m_fdServ, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	setsockopt(m_fdServ, SOL_SOCKET, DUP_NOSIGNAL, &option, sizeof(option));
#endif

	if (bind(m_fdServ, (sockaddr *)&tagServAddr, sizeof(tagServAddr)) == -1)
	{
		WLOG_WARNING(MD_DUPLOG, "bind failed\n");
		return -1;
	}

	if (listen(m_fdServ, 256) == -1)
	{
		WLOG_WARNING(MD_DUPLOG, "listen on %d failed\n", m_iPort);
		return -1;
	}

	pthread_create(&m_thSendID, NULL, CDupLog::threadSend, (void *)this);

	sockaddr_in addrClient;

#ifdef _WIN32	
	int len = sizeof(struct sockaddr);
#else
	socklen_t len = (socklen_t)sizeof(struct sockaddr);
#endif

	while (!m_bExit)
	{
		int fdNewClient = -1;

		if ((fdNewClient = accept(m_fdServ, (sockaddr *)&addrClient, &len)) == -1)
		{
			continue;
		}

		if (!m_mrgClients.AddClient(fdNewClient)) {
			WLOG_WARNING(MD_DUPLOG, "AddClient failed\n");
			DUP_SOCKET_CLOSE(fdNewClient);
		} else {
			WLOG_INFO(MD_DUPLOG, "AddClient succeed, welcome fd-%d!\n", fdNewClient);
		}

		util::Sleep(10);
		WLOG_INFO(MD_DUPLOG, "current %d client(s)\n", m_mrgClients.Size());
	}

	if (!m_mrgClients.IsEmpty())
	{
		vector<int> vcttmp;
		m_mrgClients.GetClients(vcttmp);

		for (unsigned int i = 0; i < vcttmp.size(); i++)
		{
			DUP_SOCKET_CLOSE(vcttmp[i]);
			m_mrgClients.RemoveClient(vcttmp[i]);
		}
	}

	return 0;
}

// static
void* CDupLog::threadSend(void *pPram)
{
	CDupLog *tmp = static_cast<CDupLog *>(pPram);
	
	if (tmp != NULL)
	{
		tmp->Send();
	}

	WLOG_INFO(MD_DUPLOG, "send-thread exit\n");
	return NULL;
}

void CDupLog::Send()
{
	vector<int> vcttmp;

	char szBuf[51] = {0};
	long ReadNum = -1;
	int ilastSize = -1;
	int icurSize = -1;

	if (!InitPipe())
	{
		WLOG_WARNING(MD_DUPLOG, "Pipe open failed\n");
		return;
	}

	while (!m_bExit)
	{
		m_mrgClients.GetClients(vcttmp);
		icurSize = vcttmp.size();

		if (icurSize <= 0)
		{
			if (ilastSize > 0)
			{
				RecoverStdOut();
			}

			ilastSize = icurSize;
			util::Sleep(500);
			continue;
		}

		if (ilastSize <= 0)
		{
			DupStdOut();
		}

		ilastSize = icurSize;

		memset(szBuf, 0, 51);
		ReadNum = -1;
		
#ifdef WIN32
		ReadFile(m_hPipe[0], szBuf, 50, (unsigned long *)&ReadNum, NULL);
#else
		ReadNum = read(m_fdsPipe[0], szBuf, 50);
#endif

		if (ReadNum <= 0)
		{
			util::Sleep(500);
		}

		for (unsigned int i = 0; i < vcttmp.size(); i++)
		{
			if (send(vcttmp[i], szBuf, ReadNum, DUP_NOSIGNAL) < 0)
			{
				m_mrgClients.RemoveClient(vcttmp[i]);
				DUP_SOCKET_CLOSE(vcttmp[i]);
			}
		}

		write(m_fdOrignStdOut, szBuf, ReadNum);
		util::Sleep(1);
	}

	if (m_fdOrignStdOut != -1)
	{
		RecoverStdOut();
	}

	ReleasePipe();
}

#ifdef _WIN32

bool CDupLog::InitPipe()
{
	if (!CreatePipe(&(m_hPipe[0]), &(m_hPipe[1]), NULL, 0))
	{
		return false;
	}

	int fdWEnd = _open_osfhandle((long)(m_hPipe[1]), _O_TEXT);			// 将管道写端句柄关联到文件描述符上
	g_pFPipeW = _fdopen(fdWEnd, "w");									// 打开该文件描述符，得到关联到管道写端的文件句柄

	return true;
}

void CDupLog::ReleasePipe()
{
	fclose(g_pFPipeW);
	CloseHandle(m_hPipe[0]);
	CloseHandle(m_hPipe[1]);
}

int CDupLog::DupStdOut()
{
	if (g_pFPipeW == NULL || m_hPipe[0] == NULL || m_hPipe[1] == NULL)
	{
		WLOG_WARNING(MD_DUPLOG, "Pipe not open\n");
		return -1;
	}

	m_fdOrignStdOut = _fileno(stdout);									// 将标准输出的文件描述符保存起来
	m_FStdOut = *stdout;												// 将标准输出的文件流保存起来
	*stdout = *g_pFPipeW;												// 替换stdout的文件流，即将标准输出重定向到该文件
	setvbuf(stdout, NULL, _IONBF, 0);									// 设置标准输出不使用缓存区
	return 0;
}

int CDupLog::RecoverStdOut()
{
	*stdout = m_FStdOut;

	m_fdOrignStdOut = -1;
	return 0;
}

#else

bool CDupLog::InitPipe()
{
	return (pipe(m_fdsPipe) != -1);
}

void CDupLog::ReleasePipe()
{

}

int CDupLog::DupStdOut()
{
	if (m_fdsPipe[0] < 0 || m_fdsPipe[0] < 0 )
	{
		WLOG_WARNING(MD_DUPLOG, "Pipe not open\n");
		return -1;
	}

	m_fdOrignStdOut = dup(STDOUT_FILENO);								// 复制标准输出的文件描述符
	return dup2(m_fdsPipe[1], STDOUT_FILENO);
}

int CDupLog::RecoverStdOut()
{
	dup2(m_fdOrignStdOut, STDOUT_FILENO);

	m_fdOrignStdOut = -1;
	return 0;
}

#endif







