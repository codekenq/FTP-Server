#include "ftpServer.h"
#include "_string.h"

void initFtpServer(struct FtpServer *server, char *ip, unsigned short port)
{
	int r;

	strcpy(server->_Ip, ip);
	server->_port = port;

	//获取socket 文件描述符
	server->_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server->_socket == -1)
		write_log("get server socket failed\n"), exit(-1);
	write_log("get server socket successed, sock num is %d\n", server->_socket);

	//绑定服务器端口和IP
	server->_addr.sin_family = AF_INET;
	server->_addr.sin_port = htons(port);
	inet_aton(server->_Ip, &server->_addr.sin_addr);
	r = bind(server->_socket, 
			(struct sockaddr*)(&server->_addr), sizeof(server->_addr));
	if(r == -1)
		write_log("server bind addr failed, ip is %s, port is %d\n", server->_Ip, server->_port), 
			exit(-1);
	write_log("server bind addr success\n");

	//监听端口
	r = listen(server->_socket, LISTEN_QUEUE_SIZE);
	if(r == -1)
		write_log("sever listen failed\n"), exit(-1);
	write_log("listen success\n");
}

void runFtpServer(struct FtpServer *server)
{
	int clientSocket;
	struct sockaddr_in clientAddr;
	socklen_t len;
	char clientIp[IPV4_LEN + 1];

	//主线程在指定的端口监听，当有用户接入，就开启一个新的线程，由这个线程去处理用户，主线程继续监听
	while(True)
	{
		len = sizeof(struct sockaddr_in);
		memset(&clientAddr, 0, sizeof(clientAddr));
		memset(clientIp, 0, sizeof(clientIp));
		clientSocket = accept(server->_socket, (struct sockaddr*)&clientAddr, &len);
	
		//建议用inet_ntop而不是inet_ntoa，因为inet_ntoa它是使用了一个全局变量，不可重入
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
		write_log("someone connect to server, socket : %d, IP : %s, port : %d\n",
				clientSocket, clientIp, ntohs(clientAddr.sin_port));

		//此处malloc  client的客户的控制连接，注意在适当的时候进行free
		struct FtpClient *client = (struct FtpClient *)malloc(sizeof(struct FtpClient));
		memset(client, 0, sizeof(struct FtpClient));
		
		initFtpClient(client, clientSocket, 
				clientIp, ntohs(clientAddr.sin_port));

		pthread_t thread;
		int r;
		r = pthread_create(&thread, NULL, dealClient, (void *)client);
		if(r != 0)
		{
			write_log("client deal failed, can`t create thread\n");
			close(clientSocket);
			if(client)
			{
				free(client);
				client = NULL;
			}
		}
	}
}

//注意，因为这个ip是一个由inet_ntoa()定义的全局变量，所以才能这么传参，否则的话，会引起问题的，而且
//错误极其的隐蔽。
void initFtpClient(struct FtpClient *client, int clientSocket, char *ip, unsigned short port)
{
	setClientZero(client);
	client->_controlSocket = clientSocket;//每一个socket封装了连接的两端。
	strcpy(client->_controlIp, ip);
	client->_controlPort = port;
}

void *dealClient(void *args)
{
	struct FtpClient *client = (struct FtpClient *)args;
	char statusMsg[STATUS_MSG_SIZE] = {0};

	strcpy(statusMsg, 
			"Welcome To Hanken`s Ftp Server, The Server Name is GonSon, Chinese Name is 狗剩,"
			"The Email of Author is codeken@163.com\n");
	sendMsg(client, statusMsg, strlen(statusMsg));
	dealUserCmd(client);

	return NULL;
}

void sendMsg(struct FtpClient *client, char *msg, int len)
{
	int r = 0;
	int socket = client->_controlSocket;

	r = send(socket, msg, len, 0);
	if(r < 0)
	{
		write_log("send msg to IP : %s Port : %d failed, Msg : %s", 
				client->_controlIp, client->_controlPort, msg);
		close(client->_controlSocket);
		if(client)
		{
			free(client);
			client = NULL;
		}
		pthread_exit(NULL);
	}
}

void sendData(struct FtpClient *client, char *data, int len)
{
	int r = 0;
	int socket = client->_dataSocket;
	r = send(socket, data, len, 0);
	if(r < 0)
	{
		write_log("send data failed\n");
		close(client->_dataSocket);
		client->_dataSocket = -1;
		memset(client->_dataIp, 0, sizeof(client->_dataIp));
		client->_dataPort = 0;
	}
}

void dealUserCmd(struct FtpClient *client)
{
	char cmd[CMD_SIZE];
	char args[ARGS_SIZE];

	while(True)
	{
		memset(cmd, 0, sizeof(cmd));							//不能用cmd = NULL
		memset(args, 0, sizeof(args));							//不能用args = NULL
		memset(client->_cmdBuf, 0, sizeof(client->_cmdBuf));	//不能用memset(buf, 0, sizeof(buf));

		recvMsg(client, client->_cmdBuf); //需要保证接受到的buf是一个字符串，以保证下面的parseMsg
		parseMsg(client->_cmdBuf, cmd, args);
		
#ifdef __DEBUG__
		printf("Buf : %s\n", client->_cmdBuf);
		printf("cmd : %s\n", cmd);
		printf("args : %s\n", args);
#endif /* __DEBUG__ */
		dealCmdArgs(client, cmd, args);
	}
}

void recvMsg(struct FtpClient *client, char *buf)
{
	int r;
	int socket = client->_controlSocket;

//	memset(buf, 0, sizeof(buf));这里sizeof(buf)只是buf这个变量的大小，不是buf指向的空间，呵呵
	r = recv(socket, buf, CMD_BUF_SIZE, 0);
	if(r == 0)
	{
		write_log("client leave the server, IP : %s Port : %d\n",
				client->_controlIp, client->_controlPort);
		clearClient(client);
		pthread_exit(NULL);
	}
	else if(r < 0)
	{
		write_log("recv msg from client error, IP : %s Port : %d Socket : %d\n",
				client->_controlIp, client->_controlPort, client->_controlSocket);
		clearClient(client);
		pthread_exit(NULL);
	}
	else 
	{
		buf[r] = 0;
	}
}

void dealCmdArgs(struct FtpClient *client, char *cmd, char *args)
{
	if (strcmp("USER", cmd) == 0) 
	{
		handle_USER(client, args);
	} 
	else if (strcmp("PASS", cmd) == 0) 
	{
		handle_PASS(client, args);
	} 
	else if (strcmp("SYST", cmd) == 0) 
	{
		handle_SYST(client);
	} 
	else if (strcmp("TYPE", cmd) == 0) 
	{
		handle_TYPE(client, args);
	} 
	else if (strcmp("PORT", cmd) == 0) 
	{
		handle_PORT(client, args);
	} 
	else if (strcmp("RETR", cmd) == 0) 
	{
		handle_RETR(client, args);
	} 
	else if (strcmp("PASV", cmd) == 0) 
	{
		handle_PASV(client);
	}
	else if (strcmp("QUIT", cmd) == 0) 
	{
		handle_QUIT(client);
	}
	else if (strcmp("LIST", cmd) == 0) 
	{
		handle_LIST(client, args);
	}
	else if (strcmp("PWD", cmd) == 0) 
	{
		handle_PWD(client);
	}
	else if (strcmp("STOR", cmd) == 0) 
	{
		handle_STOR(client, args);
	}
	else if (strcmp("CWD", cmd) == 0) 
	{
		handle_CWD(client, args);
	} 
	else if(strcmp("MKD", cmd) == 0)
	{
		handle_MKD(client, args);
	}
	else if(strcmp("RMD", cmd) == 0)
	{
		handle_RMD(client, args);
	}
	else if(strcmp("CDUP", cmd) == 0)
	{
		handle_CDUP(client);
	}
	else if(strcmp("RNFR", cmd) == 0)
	{
		handle_RNFR(client, args);
	}
	else if(strcmp("RNTO", cmd) == 0)
	{
		handle_RNTO(client, args);
	}
	else 
	{
		char err[STATUS_MSG_SIZE];
		strcpy(err, "550 ");
		strcat(err, cmd);
		strcat(err, " can not recognized by server\r\n");
		sendMsg(client, err, strlen(err));
	}
}

void parseMsg(char *buf, char *cmd, char *args) //必须保证buf为str,返回的cmd, args为str.
{
	int len = strlen(buf);
	int i, j;

	memset(cmd, 0, CMD_SIZE);
	memset(args, 0, ARGS_SIZE);
	i = 0;
	while(i < len && (isspace(buf[i]) || buf[i] == '\r')) i++;

	for(i, j = 0; i < len && (!(isspace(buf[i]) || buf[i] == '\r')); i++, j++)
			cmd[j] = buf[i];

	while(i < len && (isspace(buf[i]) || buf[i] == '\r')) i++;
	
	for(i, j = 0; i < len && (!(isspace(buf[i]) || buf[i] == '\r')); i++, j++)
		args[j] = buf[i];
}

void setClientZero(struct FtpClient *client)
{
	client->_controlSocket = -1;
	client->_dataSocket = -1;
	client->_acceptSocket = -1;

	memset(client->_controlIp, 0, sizeof(client->_controlIp));
	memset(client->_dataIp, 0, sizeof(client->_dataIp));

	client->_controlPort = 0;
	client->_dataPort = 0;

	memset(client->_cmdBuf, 0, sizeof(client->_cmdBuf));
	memset(client->_dataBuf, 0, sizeof(client->_dataBuf));

	memset(client->_userName, 0, sizeof(client->_userName));
	memset(client->_userPass, 0, sizeof(client->_userPass));

	memset(client->_home, 0, sizeof(client->_home));
	memset(client->_cur, 0, sizeof(client->_cur));

	client->_transType = NO_TRANS;
	client->_dataType = A;
}

int setUpConnection(struct FtpClient *client)
{
	if(client->_transType == NO_TRANS)
	{
		char *msg = "please specifiy PASV or PORT first\r\n";
		sendMsg(client, msg, strlen(msg));
		return ERR_NO_CONNECT_TYPE;
	}
	else if(client->_transType == PORT)
		return setUpPORTConnection(client);
	else if(client->_transType == PASV)
		return setUpPASVConnection(client);
}

int setUpPORTConnection(struct FtpClient *client)
{
	if(client->_dataSocket = socket(AF_INET, SOCK_STREAM, 0) < 0)
	{
		write_log("get socket failed\n");
		return ERR_PORT;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(client->_dataPort);
	inet_aton(client->_dataIp, &(addr.sin_addr));

	if(connect(client->_dataSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		write_log("connect to client error\n");
		return ERR_PORT;
	}
	write_log("connect to client success\n");
	return SUCCESS;
}

int setUpPASVConnection(struct FtpClient *client)
{
	struct sockaddr_in addr;
	int len = sizeof(addr);

	if(client->_dataSocket = accept(client->_acceptSocket, (struct sockaddr *)&addr, &len) < 0)
	{
		write_log("accept failed\n");
		return ERR_PASV;
	}
	write_log("accept success\n");

	memset(client->_dataIp, 0, sizeof(client->_dataIp));
	memset(&client->_dataPort, 0, sizeof(client->_dataPort));

	inet_ntop(AF_INET, &addr.sin_addr, client->_dataIp, sizeof(client->_dataIp));
	client->_dataPort = ntohs(addr.sin_port);
	return SUCCESS;
}

void clearClient(struct FtpClient *client)
{
	if(client->_controlSocket > 0)
	{
		close(client->_controlSocket);
		client->_controlSocket = -1;
	}
	if(client->_dataSocket > 0)
	{
		close(client->_dataSocket);
		client->_dataSocket = -1;
	}
	if(client->_acceptSocket > 0)
	{
		close(client->_acceptSocket);
		client->_acceptSocket = -1;
	}
	free(client);
}
