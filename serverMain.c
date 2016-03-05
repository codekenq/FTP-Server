/*
 * 此处添加头文件
 * */

#include "ftpServer.h"

void callBack()
{
	destroyLock(&mutex);
}

int main(char argc, char *argv[])
{
	char serverIp[IPV4_LEN + 1];	//IPV4_LEN长度为15,但是字符串必须以0结尾
	unsigned short serverPort;

	//处理命令行参数
	if(argc == 1)
	{
		strcpy(serverIp, "0.0.0.0");
		serverPort = 21;
	}
	else if(argc == 2)
	{
		if(strlen(argv[1]) > IPV4_LEN)
			printf("ip addr is illegal\n"), exit(-1);
		strcpy(serverIp, argv[1]);
		serverPort = 21;
	}
	else if(argc == 3)
	{
		if(strlen(argv[1]) > IPV4_LEN)
			printf("ip addr is illegal\n"), exit(-1);
		strcpy(serverIp, argv[1]);
		serverPort = atoi(argv[2]);
	}

	//初始化服务器
	struct FtpServer *server = (struct FtpServer *)malloc(sizeof(struct FtpServer));
	initFtpServer(server, serverIp, serverPort);

	//初始化全局锁
	initLock(&mutex);
	atexit(callBack);
	//运行服务器
	runFtpServer(server);
}
