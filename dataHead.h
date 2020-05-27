#ifndef _dataHead_H
#define _dataHead_H

enum CMD {
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_DATATX,
	CMD_ERR,
	CMD_NEW_CLIENT_ADD,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT_RESULT,
};

struct dataHeader
{
	int dataLen;
	int dataCmd;
};

struct logIner :public dataHeader
{
	logIner() {
		dataLen = sizeof(logIner);
		dataCmd = CMD_LOGIN;
		CMD_LOGINResult = false;
	}
	char name[20];
	char password[20];
	bool CMD_LOGINResult;
};

struct logOuter :public dataHeader
{
	logOuter() {
		dataLen = sizeof(logOuter);
		dataCmd = CMD_LOGOUT;
		CMD_LOGOUTResult = false;
	}
	bool CMD_LOGOUTResult;
};

struct dataer :public dataHeader
{
	dataer() {
		dataLen = sizeof(dataer);
		dataCmd = CMD_DATATX;
	}
	char dataStr[91];
};

struct errer :public dataHeader
{
	errer() {
		dataLen = sizeof(errer);
		dataCmd = CMD_ERR;
		_err = 0;
	}
	int _err;
};

struct newClientAdd :public dataHeader
{
	newClientAdd() {
		dataLen = sizeof(newClientAdd);
		dataCmd = CMD_NEW_CLIENT_ADD;
		newClientSocket = 0;
	}
	SOCKET newClientSocket;
};

struct logOutResult :public dataHeader
{
	logOutResult() {
		dataLen = sizeof(logOutResult);
		dataCmd = CMD_LOGOUT_RESULT;
		i_logOutReslut = 0;
	}
	int i_logOutReslut;
};

struct logInResult :public dataHeader
{
	logInResult() {
		dataLen = sizeof(logInResult);
		dataCmd = CMD_LOGIN_RESULT;
		i_logInReslut = 0;
	}
	int i_logInReslut;
};

#endif
