#include "easyServer.h"
#include<thread>

using namespace std;

//ԭ�Ӳ���
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
	//��ʼ
	server.start();
	while (g_Run)
	{
		server.onRun();

		///////////////
		//
		/* �������������� */
		//cout << "����������ҵ��" << endl;
		//
		//////////////
	}
	server.Close();
	getchar();
	return 0;
}

