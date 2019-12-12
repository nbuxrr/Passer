#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

extern bool Log_init(const char *pLogPath, const char *pLogName);
extern void Log_Emergency(const char *s, ...);	// [����] ϵͳ������
extern void Log_Alert(const char *s, ...);		// [����] ���뱻���������
extern void Log_Critical(const char *s, ...);	// [�ؼ�] �ؼ���Ϣ
extern void Log_Error(const char *s, ...);		// [����] ������Ϣ
extern void Log_Warning(const char *s, ...);	// [�澯] �澯��Ϣ
extern void Log_Notice(const char *s, ...);		// [֪ͨ] ��������Ҫ����Ϣ
extern void Log_Info(const char *s, ...);		// [��ʾ] ��ʾ��Ϣ
extern void Log_Debug(const char *s, ...);		// [����] ������Ϣ

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
