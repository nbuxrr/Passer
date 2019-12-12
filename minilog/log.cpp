#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#ifndef WIN32
#include <strings.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#else
#include <direct.h>
#include <io.h>
#endif

#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#ifdef WIN32
/*
* priorities/facilities are encoded into a single 32-bit quantity, where the
* bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
* (0-big number).  Both the priorities and the facilities map roughly
* one-to-one to strings in the syslogd(8) source code.  This mapping is
* included in this file.
*
* priorities (these are ordered)
*/
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */

#define NEWDIR(X) _mkdir(X)
#define EXISTDIR(X) _access(X, 0)
#define SNPRINTF _snprintf
#else

#define NEWDIR(X) mkdir(X, 0766)
#define EXISTDIR(X) access(X, 0)
#define SNPRINTF snprintf

#endif

#define DATEFORMAT         "%Y-%m-%d"
#define TIMEFORMAT         DATEFORMAT " %H:%M:%S"
#define STRERROR           strerror(errno)
#define STRLEN             256

#ifndef PATH_MAX
#define PATH_MAX 256
#endif // !PATH_MAX

/**
* Mutex object type
* @hideinitializer
*/
#define Mutex_T pthread_mutex_t
static Mutex_T log_mutex = PTHREAD_MUTEX_INITIALIZER;


static char LOG_PATH[STRLEN] = {0};
static char LOG_NAME[STRLEN] = {0};
static char LOG_DATE[STRLEN] = {0};
static FILE *g_pLOG = NULL;
#define LOG_BKNUM 7


static bool log_open();
void log_close();
void Log_Critical(const char *s, ...);


#define LOCK(mutex) \
	do {\
		Mutex_T *_yymutex = &(mutex);\
		int _yystatus = pthread_mutex_lock(_yymutex);\
		assert(_yystatus==0);

#define END_LOCK \
		_yystatus = pthread_mutex_unlock(_yymutex);\
		assert(_yystatus==0);\
	} while (0)


#define ASSERT(e) \
	do {\
		if (!(e)) {\
			Log_Critical("AssertException: " #e " at %s:%d\naborting..\n", __FILE__, __LINE__);\
			abort();\
		}\
	} while (0)


#define Log_Comm(LEVEL) {\
		ASSERT(s);\
		va_list ap;\
		va_list ap1;\
		va_start(ap, s);\
		va_start(ap1, s);\
		log_log(LEVEL, s, ap, ap1);\
		va_end(ap1);\
		va_end(ap);\
	}

	struct mylogpriority {
		int  priority;
		const char *description;
	};

	static const mylogpriority logPriority[] = {
		{LOG_EMERG,	  "emergency"},
		{LOG_ALERT,   "alert"},
		{LOG_CRIT,    "critical"},
		{LOG_ERR,     "error"},
		{LOG_WARNING, "warning"},
		{LOG_NOTICE,  "notice"},
		{LOG_INFO,    "info"},
		{LOG_DEBUG,   "debug"},
		{-1,          NULL}
	};

	// 防止linux中log层级宏定义之数值与此不一致，使用遍历比较
	static const char *log_PriorityDescription(int p) {
		const struct mylogpriority *lp = logPriority;
		
		while ((*lp).description) {
			if (p == (*lp).priority) {
				return (*lp).description;
			}

			lp++;
		}

		return "unknown";
	}

	static char *Time_fmt(char *result, int size, const char *format, time_t time) {
		struct tm tm;
		assert(result);
		assert(format);
		localtime_r((const time_t *)&time, &tm);
		
		if (strftime(result, size, format, &tm) == 0)
			*result = 0;
		
		return result;
	}

	static void createdirs(const char* pPath)
	{
		char tmpPath[PATH_MAX] = {0};
		const char *pCur = pPath;
		int pos = 0;

		if(-1 != EXISTDIR(pCur))
			return;

		memset(tmpPath, 0, sizeof(tmpPath));
		
		while (*pCur++ != '\0')
		{
			tmpPath[pos++] = *(pCur-1);

			if (*pCur == '/' || *pCur == '\0')
			{
				if (0 != EXISTDIR(tmpPath) && strlen(tmpPath) > 0)
				{
					NEWDIR(tmpPath);
				}
			}
		}
	}

	// 可继续优化清理只剩下最新的n个log文件，而不是只清理最新n个以外中最旧的1个log文件 2018-7-17 16:34:48 xurr
	int rollclearlogfile()
	{
		int iRet = 0;
		int icnt = 0;
		char aimfile[STRLEN] = {0};
		char aimrm[STRLEN] = {0};

#ifdef WIN32
		long Handle;
		struct _finddata_t FileInfo;
		char filter[STRLEN] = {0};
		SNPRINTF(filter, sizeof(filter), "%s*.log", LOG_PATH);

		if ((Handle = _findfirst(filter, &FileInfo)) != -1L)
		{
			SNPRINTF(aimfile, sizeof(aimfile), "%s", FileInfo.name);
			icnt++;

			while (_findnext(Handle, &FileInfo) == 0)
			{
				if (strcmp(aimfile, FileInfo.name) > 0)
				{
					SNPRINTF(aimfile, sizeof(aimfile), "%s", FileInfo.name);
				}

				icnt++;
			}

			_findclose(Handle);
		}
#else
		DIR *dir;
		struct dirent *ptr;

		if ((dir = opendir(LOG_PATH)) != NULL)
		{
			while ((ptr = readdir(dir)) != NULL)
			{
				if (ptr->d_name != "." && ptr->d_name != ".." && ptr->d_type == 8)    ///file
				{
					if (aimfile[0] == '\0' || strcmp(aimfile, ptr->d_name) > 0) {
						SNPRINTF(aimfile, sizeof(aimfile), "%s", ptr->d_name);
					}

					icnt++;
				}
			}
			
			closedir(dir);
		}
#endif // !WIN32

		if (aimfile[0] != '\0' && icnt > LOG_BKNUM)
		{
			SNPRINTF(aimrm, sizeof(aimrm), "%s%s", LOG_PATH, aimfile);
			remove(aimrm);
		}

		return iRet;
	}

	static void rollfile(const char *pcurtime) {
		assert(pcurtime);

		if (strncmp(LOG_DATE, pcurtime, strlen(LOG_DATE))) {
			log_close();
			log_open();
		}
	}
	
	static void log_log(int priority, const char *s, va_list ap, va_list ap1) {
		ASSERT(s);

		LOCK(log_mutex)
		{
			if (g_pLOG) {
				char datetime[STRLEN] = {0};
				Time_fmt(datetime, sizeof(datetime), TIMEFORMAT, time(NULL));
				
				rollfile(datetime);
				fprintf(g_pLOG, "[%s] %-8s : ", datetime, log_PriorityDescription(priority));
				vfprintf(g_pLOG, s, ap);

				printf("[%s] %-8s : ", datetime, log_PriorityDescription(priority));
				vprintf(s, ap1);
			}
		}
		END_LOCK;
	}

	static void log_backtrace() {
#ifdef HAVE_BACKTRACE
		int i, frames;
		void *callstack[128];
		char **strs;

		if (Run.debug >= 2) {
			frames = backtrace(callstack, 128);
			strs = backtrace_symbols(callstack, frames);
			Log_Debug("-------------------------------------------------------------------------------\n");
			for (i = 0; i < frames; ++i)
				Log_Debug("    %s\n", strs[i]);
			Log_Debug("-------------------------------------------------------------------------------\n");
			FREE(strs);
		}
#endif
	}
	
	void Log_Debug(const char *s, ...) { Log_Comm(LOG_DEBUG); }
	void Log_Info(const char *s, ...) { Log_Comm(LOG_INFO); }
	void Log_Notice(const char *s, ...) { Log_Comm(LOG_NOTICE); }
	void Log_Warning(const char *s, ...) { Log_Comm(LOG_WARNING); }
	void Log_Error(const char *s, ...) { Log_Comm(LOG_ERR); log_backtrace(); }
	void Log_Critical(const char *s, ...) { Log_Comm(LOG_CRIT); log_backtrace(); }
	void Log_Alert(const char *s, ...) { Log_Comm(LOG_ALERT); log_backtrace(); }
	void Log_Emergency(const char *s, ...) { Log_Comm(LOG_EMERG); log_backtrace(); }

	static bool log_open() {
		rollclearlogfile();

		char LogFile[STRLEN] = {0};
		createdirs(LOG_PATH);

#ifndef WIN32
		openlog(LOG_NAME, LOG_PID, LOG_DAEMON);
#endif

		Time_fmt(LOG_DATE, sizeof(LOG_DATE), DATEFORMAT, time(NULL));
		SNPRINTF(LogFile, sizeof(LogFile), "%s%s-%s.log", LOG_PATH, LOG_NAME, LOG_DATE);
		printf("%s\n", LogFile);
		
		g_pLOG = fopen(LogFile, "a+");

		if (!g_pLOG) {
			printf("Error opening the log file '%s' for writing -- %s\n", LogFile, STRERROR);
			return false;
		}

		setvbuf(g_pLOG, NULL, _IONBF, 0);	/* Set logger in unbuffered mode */
		return true;
	}

	void log_close() {
#ifndef WIN32
		closelog();
#endif
		if (g_pLOG && (0 != fclose(g_pLOG))) {
			Log_Error("Error closing the log file -- %s\n", STRERROR);
		}
		
		g_pLOG = NULL;
		printf("log closed\n");
	}

	bool Log_init(const char *pLogPath, const char *pLogName) {
		if (pLogPath != NULL
			&& pLogName != NULL
			&& strlen(pLogPath) > 0 
			&& strlen(pLogName) > 0) {

			SNPRINTF(LOG_PATH, STRLEN, "%s", pLogPath);
			SNPRINTF(LOG_NAME, STRLEN, "%s", pLogName);

			if (log_open()) {
				atexit(log_close);
				return true;
			}
		}

		return false;
	}

#ifdef __cplusplus
}
#endif

