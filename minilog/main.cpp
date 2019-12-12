#include <stdio.h>
#include "log.h"

#define MD_MAIN "main"

int main(int argc, char **argv)
{
	if (!Log_init("./testlog/", "TestApp"))
	{
		printf("Log init failed\n");
		return 0;
	}

	for (int i = 0; i < 100000; i++)
	{
		WLOG_EMERG(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_ALERT(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_CRIT(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_ERR(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_WARNING(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_NOTICE(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_INFO(MD_MAIN, "test %d, this is a test\n", i);
		WLOG_DEBUG(MD_MAIN, "test %d, this is a test\n", i);
	}

	return 0;
}

