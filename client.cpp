#define _WINDOWS

#include "easyClient.h"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

using namespace std;

bool g_bRun = true;
//总客户端连接数
const int g_count = 10000;
//客户端发送线程数
const int g_threadNum = 4;
vector<shared_ptr<easyClient>> g_clt(g_count);

mutex m;
void cmdThread()
{
	while (g_bRun)
	{
		int _command;
		string cmdstr;
		cin >> cmdstr;
		if (cmdstr == "exit")
		{
			g_bRun = false;
			cout << "Exit cmdThread" << endl;
			break;
		}
	}
}
void sendThread(int c_count)//c_count  0,1,2,3  四个线程
{
	const int cltNum = g_count / g_threadNum;
	int begin = cltNum*c_count;
	int end = cltNum*(c_count + 1);
	for (int i = begin; i < end; i++)
	{
		if (!g_bRun)
			return;
		g_clt[i]->Connect("127.0.0.1", 4657);
		m.lock();
		cout << "count = " << i+ cltNum*c_count << endl;
		m.unlock();
	}
	std::chrono::milliseconds t(3000);
	std::this_thread::sleep_for(t);

	dataer data[10];
	while (g_bRun)
	{
		for (int i = begin; i < end; i++)
		{
			g_clt[i]->sendDataN(data,10);
			
			//g_clt[i]->onRun();
		}
	}
	for (int i = begin; i < end; i++)
	{
		g_clt[i]->Close();
	}
}
int main()
{
	thread t1(cmdThread);
	t1.detach();	
	for (int i = 0; i < g_clt.size(); i++)
	{
		g_clt[i] = make_shared<easyClient>();
	}
	for (int i = 0; i < g_threadNum; i++)
	{
		thread t1(sendThread,i);
		t1.detach();
	}
	while(g_bRun)
		Sleep(1000);
	return 0;
}