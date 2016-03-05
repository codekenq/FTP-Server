#ifndef _FTPSERVER_H_
#define _FTPSERVER_H_

#include "common.h"
#include "dealCmd.h"

#define LISTEN_QUEUE_SIZE	20

enum TRANS_TYPE
{
	PORT,
	PASV,
	NO_TRANS
};

enum ERROR
{
	SUCCESS = 0,
	ERR_NO_CONNECT_TYPE,			//未指定PORT或者PASV
	ERR_PASV,						//PASV连接建立失败
	ERR_PORT						//PORT连接建立失败
};

enum DATA_TYPE
{
	I,
	A
};

struct FtpServer
{
	char _Ip[IPV4_LEN + 1];
	short _port;
	int _socket;
	struct sockaddr_in _addr;
};

struct FtpClient
{
	int _controlSocket; //这个socket的服务器端的IP和port其实和listen的IP和端口一样
	int _dataSocket;
	int _acceptSocket;

	char _controlIp[IPV4_LEN + 1]; //注意，这个_controlIp其实是客户的控制连接IP
	char _dataIp[IPV4_LEN + 1];

	unsigned short _controlPort;			 //注意，这个_controlPort 其实是客户的控制连接Port
	unsigned short _dataPort;				 //端口号一定要用unsigned short 来表示，或者比它大的int
											 //否则会出现意外的情况。

	char _cmdBuf[CMD_BUF_SIZE];
	char _dataBuf[DATA_BUF_SIZE];

	char _userName[USER_NAME_SIZE];
	char _userPass[USER_PASS_SIZE];

	char _home[PATH_SIZE];
	char _cur[PATH_SIZE];

	char _transType;
	char _dataType;
};

void initFtpServer(struct FtpServer *, char *, unsigned short);
void runFtpServer(struct FtpServer *);
void initFtpClient(struct FtpClient *, int, char *, unsigned short);
void *dealClient(void *args);
void sendMsg(struct FtpClient *, char *, int);
void sendData(struct FtpClient *, char *, int);
void dealUserCmd(struct FtpClient *);
void recvMsg(struct FtpClient *, char *);
void dealCmdArgs(struct FtpClient *, char *, char *);
void parseMsg(char *, char *, char *);
void setClientZero(struct FtpClient *);
int setUpConnection(struct FtpClient *);
int setUpPORTConnection(struct FtpClient *);
int setupPASVConnection(struct FtpClient *);
void clearClient(struct FtpClient *);

#endif /* _FTPSERVER_H_ */
