
#include <iostream>
#include <time.h>
#include "duplog.h"

using namespace std;

void* testFunc(void *pParam)
{
	printf("%s\n", __FUNCTION__);
	char now[64] = {0};
	time_t t;

	while (1)
	{
		t = time(0);
		strftime(now, sizeof(now)-1, "%Y-%m-%d %H:%M:%S", localtime(&t));
		cout << now << endl;
		util::Sleep(1000);
	}
}

int main(int argc, char **argv)
{
	pthread_t id;
	pthread_create(&id, NULL, testFunc, NULL);
	CDupLog* ptmp = CDupLog::CreateDupLog();

	util::Sleep(60000);
	delete ptmp;
	ptmp = NULL;

	return 0;
}

