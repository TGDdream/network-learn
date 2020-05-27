#include "easyServer.h"
#include<thread>

using namespace std;

//原子操作
//atomic_int g_atoCount = 0;
mutex g_mutex;

bool g_Run = true;
void cmdThread()
{
	while (g_Run)
	{
		string cmdstr;
		cin >> cmdstr;
		if (cmdstr == "exit")
		{
			g_Run = false;
			cout << "Exit cmdThread" << endl;
			break;
		}

	}
}

void customer(shared_ptr<cellServer> cell)
{
	while (g_Run)
	{
		cell->onRun();
	}
}

int main()
{
	easyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4657);
	server.Listen(5);
	thread t1(cmdThread);
	t1.detach();
	//开始
	server.start();
	while (g_Run)
	{
		server.onRun();

		///////////////
		//
		/* 服务器其他任务 */
		//cout << "服务器其他业务" << endl;
		//
		//////////////
	}
	server.Close();
	getchar();
	return 0;
}

