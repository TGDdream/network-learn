#ifndef _easyServer_H
#define _easyServer_H

#define _WINDOWS
//#define _UNIX
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#define CELL_THREAD_COUNT 3
#include<Windows.h>
#define FD_SETSIZE      2506//修改FD_SET的最大值
#include<WinSock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define SOCKET unsigned int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif

#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <queue>
#include "dataHead.h"
#include <memory>
#include<mutex>
#include<atomic>
#include <functional>
#include "myTimer.h"
//#include<thread>
using namespace std;
#define MSG_BUF_SIZE 10240

//全局互斥锁
extern mutex g_mutex;

class clientSock {
private:
	SOCKET _cSockfd;
	string _cIp;
	unsigned short _port;

public:
	int _lastPos;
	//二级消息缓冲区
	//char* _sdMsgBuf;
	char _sdMsgBuf[MSG_BUF_SIZE * 10];

	clientSock()
	{
		_lastPos = 0;
	}
	~clientSock()
	{
		//delete[] _sdMsgBuf;
	}

	clientSock(SOCKET sock, unsigned short port, string cIp = ""):clientSock()
	{
		_cSockfd = sock;
		_cIp = cIp;
		_port = port;
		//_lastPos = 0;//已经使用委托构造
		//_sdMsgBuf = new char[MSG_BUF_SIZE * 10];
	}
	SOCKET getSock()
	{
		return _cSockfd;
	}
	string getIp()
	{
		return _cIp;
	}
	unsigned short getPort()
	{
		return _port;
	}
	void setSock(SOCKET sock)
	{
		_cSockfd = sock;
	}
	void setIp(string s)
	{
		_cIp = s;
	}
	void setPort(unsigned short port)
	{
		_port = port;
	}
};

class INetEvent
{
public:
	//客户端离开事件
	virtual void OnLeave(shared_ptr<clientSock> pClient) = 0;

private:

};

class cellServer {
public:
	static atomic_int _atoCount;
	static int s_cellServerId;
	
	cellServer() {}
	cellServer(SOCKET sock)
	{
		_sock = sock;
		_id = s_cellServerId;
		s_cellServerId++;
	}
	bool onRun();
	bool isRun();
	int recvData(shared_ptr<clientSock> client);
	virtual int onNetMsg(shared_ptr<clientSock> client, dataHeader* datahead);
	int Send(dataHeader* data, SOCKET _cSock);
	void Close();
	inline int getMaxId()
	{
		return s_cellServerId;
	}
	inline size_t getClientNum()
	{
		return _clients.size()+ _msgBuffQueue.size();
	}
	SOCKET getServerSock()
	{
		return _sock;
	}
	void addClient(shared_ptr<clientSock> pClient)
	{
		g_mutex.lock();//加锁
		_msgBuffQueue.push(pClient);
		g_mutex.unlock();
	}
	void setEventObj(INetEvent* event)
	{
		_pINetEvent = event;
	}

private:
	//连接的客户端socket存储
	vector<shared_ptr<clientSock>> _clients;
	queue<shared_ptr<clientSock>> _msgBuffQueue;
	//mutex g_mutex;
	char _ftMsgBuf[MSG_BUF_SIZE];
	SOCKET _sock;
	int _id;
	INetEvent* _pINetEvent;
};

class easyServer:public INetEvent
{
private:
	SOCKET _sock;
	//一级缓冲区
	char _ftMsgBuf[MSG_BUF_SIZE];
	unsigned short _port;
	myTimer _mtime;
	int _cnt;
	int maxSock;
	vector<shared_ptr<clientSock>> _clients;
	vector<shared_ptr<clientSock>> clientErase;
	shared_ptr<thread> pCustmoerThread;
	
public:
	vector<shared_ptr<cellServer>> _cell;
	easyServer();
	virtual ~easyServer();

	void InitSocket();
	int Bind(char* ip, unsigned short port);
	void Close();
	int Listen(int n);
	SOCKET Accept();
	bool onRun();
	bool isRun();
	int recvData(shared_ptr<clientSock> client);
	virtual int onNetMsg(shared_ptr<clientSock> client, dataHeader* datahead);
	int Send(dataHeader* data, SOCKET _cSock);
	void sendToAll(dataHeader* data);
	void start();
	inline SOCKET getSock()
	{
		return _sock;
	}
	virtual void OnLeave(shared_ptr<clientSock> pClient);
	void customer(shared_ptr<cellServer> cell);


};

#endif // !_easyServer_H