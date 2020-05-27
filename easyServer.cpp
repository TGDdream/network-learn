#include "easyServer.h"



easyServer::easyServer()
{
	_sock = INVALID_SOCKET;
	_cnt = 0;
	//_ftMsgBuf = new char[MSG_BUF_SIZE];
#ifdef _UNIX
	maxSock = INVALID_SOCKET;
#endif
}


easyServer::~easyServer()
{
	//delete[] _ftMsgBuf;
	Close();
}

/* 初始化创建socket */
void easyServer::InitSocket()
{
#ifdef _WINDOWS
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	if (_sock != INVALID_SOCKET)
	{
		cout << "Socket have creat.Close Old Socket" << endl;
		Close();
	}
	//创建socket对象
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _UNIX
	maxSock = _sock;
#endif	
}

/* 绑定ip和端口 */
int easyServer::Bind(char* ip, unsigned short port)
{
	_port = port;
	//bind绑定用于接收客户端连接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);

	if (ip)
	{
#ifdef _WINDOWS
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
	}
	else
	{
#ifdef _WINDOWS
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
		_sin.sin_addr.s_addr = INADDR_ANY;
#endif
	}

	int retVal = ::bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
	if (SOCKET_ERROR == retVal)
	{
		cout << "int easyServer::Bind(char* ip, unsigned short port)-->" << "bind error...";
		cout << "Socket<" << _sock << "> Port<" << port << ">" << endl;
	}
	else
	{
		cout << "bind success..." << "Socket<" << _sock << ">  Port<" << port << ">" << endl;
	}
	return retVal;
}

/* 关闭 */
void easyServer::Close()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		//程序退出前关闭所有socket
		for (auto a : _clients)
		{
			closesocket(a->getSock());
			//delete a;
		}
		//closesocket 关闭socket
		closesocket(_sock);
		WSACleanup();
#else//程序退出前关闭所有socket
		for (auto a : _clients)
		{
			close(a->getSock());
			//delete a;
		}
		//closesocket 关闭socket
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}
}

/* 监听网络端口 */
int easyServer::Listen(int n)
{
	//listen 监听网络端口
	int retVal = listen(_sock, n);
	if (SOCKET_ERROR == retVal)
	{
		cout << "listen error...Socket<" << _sock << ">" << endl;
	}
	else
	{
		cout << "listen success...Socket<" << _sock << ">" << endl;
		return retVal;
	}
}

/* 接收客户端连接 */
SOCKET easyServer::Accept()
{
	sockaddr_in _clientAddr = {};
	SOCKET _cSock = INVALID_SOCKET;
	int nAddrLen = sizeof(sockaddr_in);
	//accept 等待客户端连接
#ifdef _WINDOWS
	_cSock = accept(_sock, (sockaddr*)&_clientAddr, &nAddrLen);
#else
	_cSock = accept(_sock, (sockaddr*)&_clientAddr, (unsigned int*)&nAddrLen);
#endif
	if (INVALID_SOCKET == _cSock)
	{
		cout << "accept error...Socket<" << _sock << ">" << endl;
	}
	else
	{
		//将客户端IP地址转换为十进制点分格式
		char str[16];
		const char* ptr = inet_ntop(AF_INET, &_clientAddr.sin_addr, str, sizeof(str));
		string s(ptr);

		auto pClient = make_shared<clientSock>(_cSock, _port, s);
		_clients.push_back(pClient);

		shared_ptr<cellServer> pMinCell = _cell[0];
		for (auto a:_cell)
		{
			if (a->getClientNum() < pMinCell->getClientNum())
			{
				pMinCell = a;
			}
		}
		pMinCell->addClient(pClient);

		//_clients.push_back(new clientSock(_cSock, _port, s));
		//向所有客户端广播新客户端加入的消息
		/*newClientAdd nca;
		nca.newClientSocket = _cSock;
		sendToAll(&nca);*/
	}
	return _cSock;
}

/* 处理网络信息 */
bool easyServer::onRun()
{
	if (isRun())
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);
		timeval select_time = { 0,10 };
#ifdef _WINDOWS
		auto ret = select(_sock + 1, &fdRead, nullptr, nullptr, &select_time);
#else	
		auto ret = select(maxSock + 1, &fdRead, nullptr, nullptr, &select_time);
#endif
		if (ret < 0)
		{
			cout << "Select end...." << endl;
			Close();
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);

			Accept();
		}

		//更新客户端退出情况
		g_mutex.lock();
		if (!clientErase.empty())
		{
			for (auto a : clientErase)
			{
				_clients.erase(find(_clients.begin(), _clients.end(), a));
			}
			clientErase.clear();
		}
		g_mutex.unlock();
		//更新时间
		auto time = _mtime.getSecond();
		if (time > 1.0)
		{
			//cout << "cell 1 Num : " << _cell[0]->getClientNum() << " cell 2 Num : " << _cell[1]->getClientNum() << "  cell 3 Num : " << _cell[2]->getClientNum() << endl;
			int clientNum = _clients.size();
			printf("time<%lf> Socket<%d> Connect_Num<%d> recvCount/s<%d>\n", time, _sock,  clientNum, (int)((double)cellServer::_atoCount/time));
			cellServer::_atoCount = 0;
			_mtime.update();
		}

		//for (size_t i = 0; i<_clients.size(); i++)
		//{
		//	if (FD_ISSET(_clients[i]->getSock(), &fdRead))
		//	{
		//		if (-1 == recvData(_clients[i]))
		//		{
		//			//delete _clients[i];
		//			_clients.erase(_clients.begin() + i);
		//			i--;
		//		}
		//	}
		//}
		return true;
	}
	else return false;

}

bool easyServer::isRun()
{
	return _sock != INVALID_SOCKET;
}

/* 消息处理 */
int easyServer::recvData(shared_ptr<clientSock> client)
{
	int nlen = recv(client->getSock(), _ftMsgBuf, MSG_BUF_SIZE, 0);
	if (nlen <= 0)
	{
		cout << "<" << client->getSock() << "> Client connect false...IP: " << client->getIp() << " port: " << client->getPort() << endl;
		return -1;
	}
	else
	{
		//cout << "nlen = " <<nlen<<endl;
		//一级缓冲数据拷贝到二级缓冲区
		memcpy(client->_sdMsgBuf + client->_lastPos, _ftMsgBuf, nlen);

		client->_lastPos += nlen;
		//二级缓冲区满
		if (client->_lastPos >= MSG_BUF_SIZE * 10 || nlen >= MSG_BUF_SIZE * 10)
		{
			cout << "Server Overflow MSG_BUF_SIZE" << endl;
			return -1;
		}
		dataHeader* datahead = (dataHeader*)client->_sdMsgBuf;
		//缓冲的数据长度大于head的长度时，判断接收数据类型
		while (client->_lastPos >= sizeof(dataHeader))
		{
			//判断接收长度大于消息的总长度，处理消息
			if (client->_lastPos >= datahead->dataLen)
			{
				int nSize = client->_lastPos - datahead->dataLen;
				onNetMsg(client, datahead);
				//将未处理的数据前移
				memcpy(client->_sdMsgBuf, client->_sdMsgBuf + datahead->dataLen, nSize);
				//消息处理后更新二级缓冲区中lastPos位置
				client->_lastPos = nSize;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

int easyServer::onNetMsg(shared_ptr<clientSock>client, dataHeader* datahead)
{
	auto t = _mtime.getSecond();
	_cnt++;
	if (t >= 1.0)
	{
		printf("time<%lf> Socket<%d> Connect_Num<%d> recvCount<%d>\n",t,_sock,_clients.size(),_cnt);
		_cnt = 0;
		_mtime.update();
	}
	switch (datahead->dataCmd)
	{
	case(CMD_LOGIN): {

		cout << "Start CMD_LOGIN..." << endl;

		logIner* CMD_LOGIN = (logIner*)datahead;

		//cout << "CMD_LOGIN User Name : " << CMD_LOGIN->name << endl;
		//cout << "CMD_LOGIN User Password : " << CMD_LOGIN->password << endl;
		//////
		//数据判断
		/////
		logInResult login_r;
		//send(client->getSock(), (char *)&login_r, login_r.dataLen, 0);

		break;
	}
	case(CMD_DATATX): {
		//cout << "Start Recive data..." << endl;
		//dataer* dataBuf = (dataer*)datahead;
		//cout << "recive data : " << dataBuf->dataStr << endl;
		/*if (0 != strcmp(dataBuf->dataStr, "ssssssssssssssssssssssssssq"))
		{
		cout << "error data" << endl;
		}*/
		//Send(dataBuf, client->getSock());
		//send(client->getSock(), (char *)dataBuf, dataBuf->dataLen, 0);
		break;
	}
	case(CMD_LOGOUT): {
		/*
		logOuter* CMD_LOGOUT = (logOuter*)datahead;
		cout << "CMD_LOGOUT..." << "client IP : " << client->getIp() << endl;
		//////
		//数据判断
		/////
		logOutResult logout_r;
		send(client->getSock(), (char *)&logout_r, logout_r.dataLen, 0);
		break;
		*/
	}
	default: {
		cout << "Sock" << client->getSock() << "  no define CMD" << endl;
		//errer cerr;
		//cerr._err = 0;//接收失败
		//send(client->getSock(), (char *)&cerr, cerr.dataLen, 0);
	}
	}
	return 0;
}

int easyServer::Send(dataHeader* data, SOCKET _cSock)
{
	if (isRun() && data)
		return send(_cSock, (char *)data, data->dataLen, 0);
	else return SOCKET_ERROR;
}

void easyServer::sendToAll(dataHeader* data)
{
	for (auto a : _clients)
	{
		Send(data, a->getSock());
	}
}

void easyServer::start()
{
	for (int i = 0; i < CELL_THREAD_COUNT; i++)
	{
		auto pCell = make_shared<cellServer>(_sock);
		_cell.push_back(pCell);
		pCell->setEventObj(this);
		//启动子线程
		//thread custmoerThread(&easyServer::customer,this,pCell);
		pCustmoerThread = make_shared<thread>(mem_fun(&easyServer::customer), this, pCell);
		//thread custmoerThread(mem_fun(&easyServer::customer), this, pCell);//使用mem_fun来将void easyServer::customer(shared_ptr<cellServer> cell)的函数指针转换为void customer(easyServer* pServer,shared_ptr<cellServer> cell)
		pCustmoerThread->detach();
	}
}

void easyServer::OnLeave(shared_ptr<clientSock> pClient)
{
	lock_guard<mutex> lock(g_mutex);
	for (auto a : _clients)
	{
		if (pClient->getSock() == a->getSock())
		{
			clientErase.push_back(pClient);
			break;
		}
	}
}

void easyServer::customer(shared_ptr<cellServer> cell)
{
	while (isRun())
	{
		cell->onRun();
	}
}

//////////////////////////////////////////////////////////////////////////////////////
/* cellServer function*/
/*static variate statement*/
int cellServer::s_cellServerId = 0;
atomic_int cellServer::_atoCount = 0;
//test
//bool cellServer::onRun()
//{
//	cout << "Cell ID: " << _id << endl;
//	return true;
//}

/* 处理网络信息 */
bool cellServer::onRun()
{
	if (isRun())
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);

		//查询消息缓冲队列是否有新加入的客户端连接
		while (false == _msgBuffQueue.empty())
		{
			lock_guard<mutex> lock(g_mutex);
			_clients.push_back(_msgBuffQueue.front());
			_msgBuffQueue.pop();
		}

		if (_clients.empty())
		{
			//没有客户端时休眠线程1ms
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			return false;
		}

		for (auto a : _clients)
		{
			FD_SET(a->getSock(), &fdRead);
#ifdef _UNIX
			if (maxSock < a->getSock())
			{
				maxSock = a->getSock();
			}
#endif
		}

		//timeval select_time = { 1,0 };
#ifdef _WINDOWS
		auto ret = select(_sock + 1, &fdRead, nullptr, nullptr, nullptr);
#else	
		auto ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &select_time);
#endif
		if (ret < 0)
		{
			cout << "Select end...." << endl;
			Close();
			return false;
		}
		//查询客户端是否有响应
		for (int i = _clients.size()-1; i>=0; i--)
		{
			if (FD_ISSET(_clients[i]->getSock(), &fdRead))
			{
				if (-1 == recvData(_clients[i]))//处理客户端数据
				{
					if (_pINetEvent)
					{
						_pINetEvent->OnLeave(_clients[i]);
					}			
					_clients.erase(_clients.begin() + i);
				}
			}
		}
		return true;
	}
	else return false;

}

bool cellServer::isRun()
{
	return _sock != INVALID_SOCKET;
}

/* 消息处理 */
int cellServer::recvData(shared_ptr<clientSock> client)
{
	int nlen = recv(client->getSock(), _ftMsgBuf, MSG_BUF_SIZE, 0);
	if (nlen <= 0)
	{
		cout << "<" << client->getSock() << "> Client connect false...IP: " << client->getIp() << " port: " << client->getPort() << endl;
		return -1;
	}
	else
	{
		//cout << "nlen = " <<nlen<<endl;
		//一级缓冲数据拷贝到二级缓冲区
		memcpy(client->_sdMsgBuf + client->_lastPos, _ftMsgBuf, nlen);

		client->_lastPos += nlen;
		//二级缓冲区满
		if (client->_lastPos >= MSG_BUF_SIZE * 10 || nlen >= MSG_BUF_SIZE * 10)
		{
			cout << "Server Overflow MSG_BUF_SIZE" << endl;
			return -1;
		}
		dataHeader* datahead = (dataHeader*)client->_sdMsgBuf;
		//缓冲的数据长度大于head的长度时，判断接收数据类型
		while (client->_lastPos >= sizeof(dataHeader))
		{
			//判断接收长度大于消息的总长度，处理消息
			if (client->_lastPos >= datahead->dataLen)
			{
				int nSize = client->_lastPos - datahead->dataLen;
				onNetMsg(client, datahead);
				//将未处理的数据前移
				memcpy(client->_sdMsgBuf, client->_sdMsgBuf + datahead->dataLen, nSize);
				//消息处理后更新二级缓冲区中lastPos位置
				client->_lastPos = nSize;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

int cellServer::onNetMsg(shared_ptr<clientSock>client, dataHeader* datahead)
{
	//auto t = _mtime.getSecond();
	//_cnt++;
	/*if (t >= 1.0)
	{
		printf("time<%lf> Socket<%d> Connect_Num<%d> recvCount<%d>\n", t, _sock, _clients.size(), _cnt);
		_cnt = 0;
		_mtime.update();
	}*/
	_atoCount++;
	switch (datahead->dataCmd)
	{
	case(CMD_LOGIN): {

		//cout << "Start CMD_LOGIN..." << endl;

		logIner* CMD_LOGIN = (logIner*)datahead;

		//cout << "CMD_LOGIN User Name : " << CMD_LOGIN->name << endl;
		//cout << "CMD_LOGIN User Password : " << CMD_LOGIN->password << endl;
		//////
		//数据判断
		/////
		logInResult login_r;
		//send(client->getSock(), (char *)&login_r, login_r.dataLen, 0);

		break;
	}
	case(CMD_DATATX): {
		//cout << "Start Recive data..." << endl;
		//dataer* dataBuf = (dataer*)datahead;
		//cout << "recive data : " << dataBuf->dataStr << endl;
		/*if (0 != strcmp(dataBuf->dataStr, "ssssssssssssssssssssssssssq"))
		{
		cout << "error data" << endl;
		}*/
		//Send(dataBuf, client->getSock());
		//send(client->getSock(), (char *)dataBuf, dataBuf->dataLen, 0);
		break;
	}
	case(CMD_LOGOUT): {
		/*
		logOuter* CMD_LOGOUT = (logOuter*)datahead;
		cout << "CMD_LOGOUT..." << "client IP : " << client->getIp() << endl;
		//////
		//数据判断
		/////
		logOutResult logout_r;
		send(client->getSock(), (char *)&logout_r, logout_r.dataLen, 0);
		break;
		*/
	}
	default: {
		cout << "Sock" << client->getSock() << "  no define CMD" << endl;
		//errer cerr;
		//cerr._err = 0;//接收失败
		//send(client->getSock(), (char *)&cerr, cerr.dataLen, 0);
	}
	}
	return 0;
}

int cellServer::Send(dataHeader* data, SOCKET _cSock)
{
	if (isRun() && data)
		return send(_cSock, (char *)data, data->dataLen, 0);
	else return SOCKET_ERROR;
}

/* 关闭 */
void cellServer::Close()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		//程序退出前关闭所有socket
		for (auto a : _clients)
		{
			closesocket(a->getSock());
			//delete a;
		}
		//closesocket 关闭socket
		closesocket(_sock);
		WSACleanup();
#else//程序退出前关闭所有socket
		for (auto a : _clients)
		{
			close(a->getSock());
			//delete a;
		}
		//closesocket 关闭socket
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}
}