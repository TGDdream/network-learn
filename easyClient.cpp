#include "easyClient.h"

easyClient::easyClient()
{
	_sock = INVALID_SOCKET;
	_lastPos = 0;
}


easyClient::~easyClient()
{
	Close();
}

/* ��ʼ��Socket */
void easyClient::InitSocket()
{
	if (_sock != INVALID_SOCKET)
	{
		cout << "repeat Socket ..close _socket" << "<" << _sock << ">" << endl;
		Close();
	}

#ifdef _WINDOWS
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif
	_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		cout << "creat socket error..." << endl;
	}
}

/* ���ӵ�ָ��IP�Ͷ˿ڵķ����� */
int easyClient::Connect(char* ip, unsigned short port)
{
	ServerIp = ip;
	if (_sock == INVALID_SOCKET)
	{
		InitSocket();
	}
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);
#ifdef _WINDOWS
	_sin.sin_addr.S_un.S_addr = inet_addr(ServerIp);
#else 
	_sin.sin_addr.s_addr = inet_addr(ServerIp);
#endif
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		cout << "connect Socket <" << _sock << "> error..." << endl;
		Close();
	}
	else
	{
		//cout << "connect Socket <" << _sock << "> success..." << "--connect Server IP: " << ServerIp << endl;
	}
	return ret;
}

/*�رյ�ǰ����������*/
void easyClient::Close()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WINDOWS
		closesocket(_sock);
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
	}
}

SOCKET easyClient::getSocket()
{
	return _sock;
}

/* ����������� */

bool easyClient::onRun()
{
	if (isRun())
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);
		timeval select_time = { 0,0 };
#ifdef _WINDOWS
		auto ret = select(0, &fdRead, 0, 0, &select_time);
#else
		auto ret = select(_sock + 1, &fdRead, 0, 0, &select_time);
#endif
		if (ret < 0)
		{
			cout << "select end..." << endl;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == recvData())
			{
				cout << "select end..." << endl;
				Close();
				return false;
			}
		}

		return true;
	}
	else return false;
}
bool easyClient::isRun()
{
	return _sock != INVALID_SOCKET;
}

/* ��Ϣ���� */
/* ����ճ������� */
int easyClient::recvData()
{
	int nlen = recv(_sock, _ftMsgBuf, MSG_BUF_SIZE, 0);
	if (nlen <= 0)
	{
		cout << "Server connect false..." << endl;
		return -1;
	}
	else
	{
		//һ���������ݿ���������������
		memcpy(_sdMsgBuf + _lastPos, _ftMsgBuf, nlen);
		_lastPos += nlen;
		//������������
		if (_lastPos >= MSG_BUF_SIZE * 10 || nlen >= MSG_BUF_SIZE)
		{
			cout << "Client Overflow MSG_BUF_SIZE" << endl;
			return -1;
		}
		dataHeader* datahead = (dataHeader*)_sdMsgBuf;
		//��������ݳ��ȴ���head�ĳ���ʱ���жϽ�����������
		while (_lastPos >= sizeof(dataHeader))
		{
			//�жϽ��ճ��ȴ�����Ϣ���ܳ��ȣ�������Ϣ
			if (_lastPos >= datahead->dataLen)
			{
				int nSize = _lastPos - datahead->dataLen;
				onNetMsg(datahead);
				//��δ���������ǰ��
				memcpy(_sdMsgBuf, _sdMsgBuf + datahead->dataLen, nSize);
				//��Ϣ�������¶�����������lastPosλ��
				_lastPos = nSize;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

/* ��Ӧ������Ϣ */
int easyClient::onNetMsg(dataHeader* datahead)
{
	switch (datahead->dataCmd)
	{
	case(CMD_LOGIN_RESULT): {
		logInResult* login_r = (logInResult*)datahead;
		if (login_r->i_logInReslut != 0)
		{
			cout << "login success" << endl;
		}
		else
		{
			cout << "Check the password" << endl;
		}
		break;
	}
	case(CMD_NEW_CLIENT_ADD): {
		newClientAdd* newClient = (newClientAdd*)datahead;
		cout << "new Client Add..." << "SOCKET:" << newClient->newClientSocket << endl;
		break;
	}
	case(CMD_LOGOUT_RESULT): {
		logOutResult* logout_r = (logOutResult*)datahead;
		if (logout_r->i_logOutReslut != 0)
		{
			cout << "logout success" << endl;
		}
		else
		{
			cout << "can't Logout..." << endl;
		}
		break;
	}case(CMD_ERR): {
		errer* err = (errer*)datahead;
		cout << "CMD_ERR ; " << err->_err << endl;
		break;
	}case(CMD_DATATX): {
		//dataer* dat = (dataer*)datahead;
		//cout << "test::reciv CMD_DATATX" << "::" << dat->dataStr << endl;
		/*if (0 != strcmp(dat->dataStr, "ssssssssssssssssssssssssssq"))
		{
		cout << "error data" << endl;
		}*/
		break;
	}
	default: {
		cout << "no define Head Command!" << endl;
	}
	}
	return 0;
}

/* �������ݰ� */
int easyClient::sendData(dataHeader* data)
{
	if (isRun() && data)
		return send(_sock, (char *)data, data->dataLen, 0);
	else return SOCKET_ERROR;
}

/* һ�η��Ͷ�����ݰ� */
int easyClient::sendDataN(dataHeader* data,int n)
{
	if (isRun() && data)
		return send(_sock, (char *)data, (data->dataLen)*n, 0);
	else return SOCKET_ERROR;
}