#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

extern bool Log_init(const char *pLogPath, const char *pLogName);
extern void Log_Emergency(const char *s, ...);	// [紧急] 系统不可用
extern void Log_Alert(const char *s, ...);		// [警报] 必须被立即处理的
extern void Log_Critical(const char *s, ...);	// [关键] 关键信息
extern void Log_Error(const char *s, ...);		// [错误] 错误信息
extern void Log_Warning(const char *s, ...);	// [告警] 告警信息
extern void Log_Notice(const char *s, ...);		// [通知] 正常但重要的信息
extern void Log_Info(const char *s, ...);		// [提示] 提示信息
extern void Log_Debug(const char *s, ...);		// [调试] 调试信息

#ifdef __cplusplus
	}
#endif

#define WLOG_EMERG(_submodule_, format, ...)	Log_Emergency("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_ALERT(_submodule_, format, ...)		Log_Alert("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_CRIT(_submodule_, format, ...)		 Log_Critical("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_ERR(_submodule_, format, ...)			Log_Error("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_WARNING(_submodule_, format, ...)	  Log_Warning("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_NOTICE(_submodule_, format, ...)	   Log_Notice("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_INFO(_submodule_, format, ...)			 Log_Info("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define WLOG_DEBUG(_submodule_, format, ...)		Log_Debug("[%s][%s - %d] " format, _submodule_, __FUNCTION__, __LINE__, ##__VA_ARGS__)


#endif
