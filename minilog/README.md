# minilog

这是在log.c的开源代码基础上修改的日志模块

## 功能特性

- 超轻量易集成易使用
- 支持跨平台（win平台可能需要稍微修改）
- 支持多线程安全
- 支持分层标记
- 支持分模块标记
- 支持按日期滚动存储（默认保存最近7天日志）

## 注意

- 本模块当前依赖pthread库，win平台可能需要加入pthread库，或者使用win的线程API或C++11的多线程线程特性稍微修改即可

## 调用示例

```cpp
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
```

## 效果

```bash
[2019-12-12 14:37:24] emergency : [main][main - 16] test 8973, this is a test
[2019-12-12 14:37:24] alert    : [main][main - 17] test 8973, this is a test
[2019-12-12 14:37:24] critical : [main][main - 18] test 8973, this is a test
[2019-12-12 14:37:24] error    : [main][main - 19] test 8973, this is a test
[2019-12-12 14:37:24] warning  : [main][main - 20] test 8973, this is a test
[2019-12-12 14:37:24] notice   : [main][main - 21] test 8973, this is a test
[2019-12-12 14:37:24] info     : [main][main - 22] test 8973, this is a test
[2019-12-12 14:37:24] debug    : [main][main - 23] test 8973, this is a test
```
