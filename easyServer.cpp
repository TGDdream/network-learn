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

/* ��ʼ������socket */
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
	//����socket����
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _UNIX
	maxSock = _sock;
#endif	
}

/* ��ip�Ͷ˿� */
int easyServer::Bind(char* ip, unsigned short port)
{
	_port = port;
	//bind�����ڽ��տͻ������ӵ�����˿�
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

/* �ر� */
void easyServer::Close()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		//�����˳�ǰ�ر�����socket
		for (auto a : _clients)
		{
			closesocket(a->getSock());
			//delete a;
		}
		//closesocket �ر�socket
		closesocket(_sock);
		WSACleanup();
#else//�����˳�ǰ�ر�����socket
		for (auto a : _clients)
		{
			close(a->getSock());
			//delete a;
		}
		//closesocket �ر�socket
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}
}

/* ��������˿� */
int easyServer::Listen(int n)
{
	//listen ��������˿�
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

/* ���տͻ������� */
SOCKET easyServer::Accept()
{
	sockaddr_in _clientAddr = {};
	SOCKET _cSock = INVALID_SOCKET;
	int nAddrLen = sizeof(sockaddr_in);
	//accept �ȴ��ͻ�������
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
		//���ͻ���IP��ַת��Ϊʮ���Ƶ�ָ�ʽ
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
		//�����пͻ��˹㲥�¿ͻ��˼������Ϣ
		/*newClientAdd nca;
		nca.newClientSocket = _cSock;
		sendToAll(&nca);*/
	}
	return _cSock;
}

/* ����������Ϣ */
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

		//���¿ͻ����˳����
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
		//����ʱ��
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

/* ��Ϣ���� */
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
		//һ���������ݿ���������������
		memcpy(client->_sdMsgBuf + client->_lastPos, _ftMsgBuf, nlen);

		client->_lastPos += nlen;
		//������������
		if (client->_lastPos >= MSG_BUF_SIZE * 10 || nlen >= MSG_BUF_SIZE * 10)
		{
			cout << "Server Overflow MSG_BUF_SIZE" << endl;
			return -1;
		}
		dataHeader* datahead = (dataHeader*)client->_sdMsgBuf;
		//��������ݳ��ȴ���head�ĳ���ʱ���жϽ�����������
		while (client->_lastPos >= sizeof(dataHeader))
		{
			//�жϽ��ճ��ȴ�����Ϣ���ܳ��ȣ�������Ϣ
			if (client->_lastPos >= datahead->dataLen)
			{
				int nSize = client->_lastPos - datahead->dataLen;
				onNetMsg(client, datahead);
				//��δ���������ǰ��
				memcpy(client->_sdMsgBuf, client->_sdMsgBuf + datahead->dataLen, nSize);
				//��Ϣ�������¶�����������lastPosλ��
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
		//�����ж�
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
		//�����ж�
		/////
		logOutResult logout_r;
		send(client->getSock(), (char *)&logout_r, logout_r.dataLen, 0);
		break;
		*/
	}
	default: {
		cout << "Sock" << client->getSock() << "  no define CMD" << endl;
		//errer cerr;
		//cerr._err = 0;//����ʧ��
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
		//�������߳�
		//thread custmoerThread(&easyServer::customer,this,pCell);
		pCustmoerThread = make_shared<thread>(mem_fun(&easyServer::customer), this, pCell);
		//thread custmoerThread(mem_fun(&easyServer::customer), this, pCell);//ʹ��mem_fun����void easyServer::customer(shared_ptr<cellServer> cell)�ĺ���ָ��ת��Ϊvoid customer(easyServer* pServer,shared_ptr<cellServer> cell)
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

/* ����������Ϣ */
bool cellServer::onRun()
{
	if (isRun())
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);

		//��ѯ��Ϣ��������Ƿ����¼���Ŀͻ�������
		while (false == _msgBuffQueue.empty())
		{
			lock_guard<mutex> lock(g_mutex);
			_clients.push_back(_msgBuffQueue.front());
			_msgBuffQueue.pop();
		}

		if (_clients.empty())
		{
			//û�пͻ���ʱ�����߳�1ms
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
		//��ѯ�ͻ����Ƿ�����Ӧ
		for (int i = _clients.size()-1; i>=0; i--)
		{
			if (FD_ISSET(_clients[i]->getSock(), &fdRead))
			{
				if (-1 == recvData(_clients[i]))//����ͻ�������
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

/* ��Ϣ���� */
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
		//һ���������ݿ���������������
		memcpy(client->_sdMsgBuf + client->_lastPos, _ftMsgBuf, nlen);

		client->_lastPos += nlen;
		//������������
		if (client->_lastPos >= MSG_BUF_SIZE * 10 || nlen >= MSG_BUF_SIZE * 10)
		{
			cout << "Server Overflow MSG_BUF_SIZE" << endl;
			return -1;
		}
		dataHeader* datahead = (dataHeader*)client->_sdMsgBuf;
		//��������ݳ��ȴ���head�ĳ���ʱ���жϽ�����������
		while (client->_lastPos >= sizeof(dataHeader))
		{
			//�жϽ��ճ��ȴ�����Ϣ���ܳ��ȣ�������Ϣ
			if (client->_lastPos >= datahead->dataLen)
			{
				int nSize = client->_lastPos - datahead->dataLen;
				onNetMsg(client, datahead);
				//��δ���������ǰ��
				memcpy(client->_sdMsgBuf, client->_sdMsgBuf + datahead->dataLen, nSize);
				//��Ϣ�������¶�����������lastPosλ��
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
		//�����ж�
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
		//�����ж�
		/////
		logOutResult logout_r;
		send(client->getSock(), (char *)&logout_r, logout_r.dataLen, 0);
		break;
		*/
	}
	default: {
		cout << "Sock" << client->getSock() << "  no define CMD" << endl;
		//errer cerr;
		//cerr._err = 0;//����ʧ��
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

/* �ر� */
void cellServer::Close()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		//�����˳�ǰ�ر�����socket
		for (auto a : _clients)
		{
			closesocket(a->getSock());
			//delete a;
		}
		//closesocket �ر�socket
		closesocket(_sock);
		WSACleanup();
#else//�����˳�ǰ�ر�����socket
		for (auto a : _clients)
		{
			close(a->getSock());
			//delete a;
		}
		//closesocket �ر�socket
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}
}