#ifndef _easyClient_H
#define _easyClient_H

#define _WINDOWS

#ifdef _WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include<Windows.h>
	#include<WinSock2.h>
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#define SOCKET unsigned int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif
	#include <string>
	#include <string.h>
	#include<iostream>
	#include<queue>
	#include<memory>
	#include "dataHead.h"	#include "myTimer.h"
#define MSG_BUF_SIZE 10240
using namespace std;

class easyClient
{
private:
	SOCKET _sock;
	char* ServerIp;
	int _lastPos;
	char _ftMsgBuf[MSG_BUF_SIZE];
	char _sdMsgBuf[MSG_BUF_SIZE * 10];
public:
	easyClient();
	virtual ~easyClient();	void InitSocket();
	int Connect(char* ip, unsigned short port);
	void Close();
	bool onRun();
	SOCKET getSocket();
	int recvData();
	int onNetMsg(dataHeader* datahead);
	int sendData(dataHeader* data);
	int sendDataN(dataHeader* data,int n);
	bool isRun();
};
#endif
