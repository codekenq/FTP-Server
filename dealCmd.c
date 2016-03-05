#include "dealCmd.h"

MUTEX mutex;
char oldPath[PATH_SIZE];
char newPath[PATH_SIZE];

static parseIpPort(char *ipPort, char *ip, unsigned short *port)
{
	char data[7];		//这里7是因为，PORT命令传来的参数为h1 h2 h3 h4 p1 p2,刚好6个字符，但是
						//等下解析是以字符串形式作为结果，因此总长为7
	
	memset(data, 0, sizeof(data));
	removeChar(ipPort, data, ',');
	data[6] = 0;
	printf("%s\n", data);

	memset(ip, 0, IPV4_LEN + 1);					//	memset(ip, 0, sizeof(ip));
	sprintf(ip, "%d.%d.%d.%d", 
			(unsigned char)data[0], 
			(unsigned char)data[1], 
			(unsigned char)data[2], 
			(unsigned char)data[3]);
	
	memset(port, 0, sizeof(unsigned short));
	*port = (unsigned char)data[4];
	*port = (*port) << 8;
	*port += (unsigned char) data[5];

	#ifdef __DEBUG__
	write_log("PORT IP : %s : Port : %d\n", ip, *port);
	#endif /* __DEBUG__ */
}

static void dealPath(struct FtpClient *client, char *path, char *args)
{
	if(args[0] == '/' || args[0] == '~')
		strcpy(path, args);
	else
	{
		strcpy(path, client->_cur);
		strcat(path, args);
	}
}

static void getPathInfo(char *path, char *info) //相当于在这里实现一个简单的ls命令
{
	char *ls = "ls -l ";
	char cmd[CMD_BUF_SIZE];
	char temp[1024];						//这里的1024是命令的一条返回值的长度
	int infoLen = DATA_BUF_SIZE - 1;
	int n = 0;
	int count = 0;
	
	strcpy(cmd, ls);
	strcat(cmd, path);

	FILE *f;

	if((f = popen(cmd, "r")) == NULL)
	{
		write_log("popen invoke failed\n");
		info[0] = 0;						//info = NULL;
		return ;
	}
	write_log("popen invoke success\n");

	info[0] = 0;
	while(fgets(temp, sizeof(temp), f) != NULL)
	{
		strcat(temp, "\r\n");
		n = strlen(temp);
		count += n;
		if(count > infoLen)			//防止缓冲区溢出
			return ;
		strcat(info, temp);
	}
	pclose(f);
}

//确保data线程不会写struct FtpClient结构，那么就不必加锁

void handle_USER(struct FtpClient *client, char *args)
{
	//这里只是简单的将username传递给控制头
	memset(client->_userName, 0, USER_NAME_SIZE);
	strcpy(client->_userName, args);
}

void handle_PASS(struct FtpClient *client, char *args)
{
	//这里只是简单的将userpass传递给控制头
	memset(client->_userPass, 0, USER_PASS_SIZE);
	strcpy(client->_userPass, args);

	strcpy(client->_home, "/home/hanken/temp/");
	strcpy(client->_cur, "/home/hanken/temp/");
}

void handle_SYST(struct FtpClient *client)
{
	int socket = client->_controlSocket;

	char *msg = "215 UNIX Type: L8\r\n";
	sendMsg(client, msg, strlen(msg));
}

void handle_TYPE(struct FtpClient *client, char *args)
{
	char *msg;
	int socket = client->_controlSocket;

	//这里只是简单的执行将控制结构体的相应字段改变
	if(!strcmp(args, "I"))
	{
		msg = "200 Type set to I.\r\n";
		client->_dataType = I;
	}
	else if(!strcmp(args, "A"))
	{
		msg = "200 Type set to A.\r\n";
		client->_dataType = A;
	}
	else
	{
		msg = "error cmd\r\n";
	}
	sendMsg(client, msg, strlen(msg));
}

void handle_PORT(struct FtpClient *client, char *args)
{
	client->_transType = PORT;
	parseIpPort(args, client->_dataIp, &(client->_dataPort));
}

void handle_RETR(struct FtpClient *client, char *args)
{
	int r;
	FILE *f;
	char *err = "451 trouble to retr file\r\n";
	char path[PATH_SIZE];

	dealPath(client, path, args);
	f = fopen(path, "rb");
	if(f == NULL)
		sendMsg(client, err, strlen(err));

	setUpConnection(client);
	while(!feof(f))
	{
		r = fread(client->_dataBuf, sizeof(char), sizeof(client->_dataBuf) - 1, 0);
		(client->_dataBuf)[r] = 0;
		sendData(client, client->_dataBuf, strlen(client->_dataBuf));
	}
	close(client->_dataSocket);
	client->_dataSocket = 0;
	memset(client->_dataIp, 0, sizeof(client->_dataIp)); //这里不能用_dataIp = NULL,因为_dataIp的类型为
														 //char [16]
	client->_dataPort = 0;
}

void handle_PASV(struct FtpClient *client)
{
	unsigned char *p, *q;	//该指针用来取出32bit IP中的值和port中的值
	char *msg = "425 Can not open data connection\r\n";
	char ipPortMsg[STATUS_MSG_SIZE];
	int len, r;
	unsigned short port;
	struct sockaddr_in addr;
	char ip[IPV4_LEN + 1];

	addr.sin_family = AF_INET;
	inet_aton(client->_controlIp, &addr.sin_addr);
	addr.sin_port = htons(0);

	r = client->_acceptSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(r < 0)
	{
		write_log("accept socket get failed\n");
		sendMsg(client, msg, strlen(msg));
		return ;
	}
	write_log("data socket get success\n");

	r = bind(client->_acceptSocket, (struct sockaddr *)&addr, sizeof(addr));
	if(r < 0)
	{
		printf("%m\n");
		write_log("bind failed\n");
		sendMsg(client, msg, strlen(msg));
		return;
	}
	write_log("bind success");

	r = listen(client->_acceptSocket, LISTEN_QUEUE_SIZE);
	if(r < 0)
	{
		write_log("listen failed\n");
		sendMsg(client, msg, strlen(msg));
		return ;
	}
	write_log("listen success\n");

	len = sizeof(addr);
	getsockname(client->_acceptSocket, (struct sockaddr *)&addr, &len);

	p = (unsigned char *)&(addr.sin_addr);
	q = (unsigned char *)&(addr.sin_port);
	memset(ipPortMsg, 0, sizeof(ipPortMsg));
	sprintf(ipPortMsg, "227 Entering Passive Mode ("
			"%d,%d,%d,%d,%d,%d"
			")\r\n",
			p[0],
			p[1],
			p[2],
			p[3],
			q[0],
			q[1]
			);
	sendMsg(client, ipPortMsg, strlen(ipPortMsg));
	inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
	write_log("PASV listen IP : %s Port : %d\n", ip, ntohs(addr.sin_port));
}

void handle_QUIT(struct FtpClient *client)
{
	//根据RFC959，如果data连接没有在工作，那么应该由server端关闭control连接。
	//如果data连接正在传输数据，那么control连接应该保持直到data传输完毕，发送response后，由server进行关
	//闭
	
	char *res = "220 Bye~\r\n";
	sendMsg(client, res, strlen(res));
	clearClient(client);
	pthread_exit(NULL);
}

void handle_LIST(struct FtpClient *client, char *args)
{
	char buf[DATA_BUF_SIZE];
	char path[PATH_SIZE];
	
	memset(path, 0, sizeof(path));
	dealPath(client, path, args);
	getPathInfo(args, buf);
	setUpConnection(client);
	sendData(client, buf, strlen(buf));
}

void handle_PWD(struct FtpClient *client)
{
	char msg[PATH_SIZE];
	int socket = client->_controlSocket;

	memset(msg, 0, sizeof(msg));
	strcpy(msg, "257 \"");
	strcat(msg, client->_cur);
	strcat(msg, "\"\r\n");
	sendMsg(client, msg, strlen(msg));
}

void handle_STOR(struct FtpClient *client, char *args)
{
	int r;
	FILE *f = NULL;
	char *err = "451 trouble to STOR file\r\n";
	char path[PATH_SIZE];
	dealPath(client, path, args);

	f = fopen(path, "wb");
	if(f == NULL)
		sendMsg(client, err, strlen(err));

	setUpConnection(client);

	while(True)
	{
		r = recv(client->_dataSocket, client->_dataBuf, sizeof(client->_dataBuf), 0);
		if(r < 0)
		{
			write_log("stor recv file error\n");
			close(client->_dataSocket);
			client->_dataSocket = -1;
			memset(client->_dataIp, 0, sizeof(client->_dataIp));  //不能用client->_dataIp = NULL;
			client->_dataPort = 0;
			break;
		}
		else if(r == 0)
		{
			write_log("recv file finished\r\n");
			close(client->_dataSocket);
			client->_dataSocket = -1;
			memset(client->_dataIp, 0, sizeof(client->_dataIp));  //不能用client->_dataIp = NULL;
			client->_dataPort = 0;
			break;
		}
		else
		{
			fwrite(client->_dataBuf, 1, r, f);
		}
	}
	fclose(f);
}

void handle_CWD(struct FtpClient *client, char *args)
{
	char *err = "550 No such directory\r\n";
	char *success = "250 Okay\r\n";

	char path[PATH_SIZE];
	char cmd[CMD_BUF_SIZE];
	dealPath(client, path, args);

	#ifdef __DEBUG__
	printf("CWD PATH : %s\n", path);
	#endif /*__DEBUG__ */

	if(access(path, F_OK))
	{
		sendMsg(client, err, strlen(err));
		return ;
	}

	char *cmd1 = "cd ";
	char *cmd2 = " && pwd";
	strcpy(cmd, cmd1);
	strcat(cmd, path);
	strcat(cmd, cmd2);

	#ifdef __DEBUG__
	printf("CWD Order : %s\n", cmd);
	#endif 

	FILE * f = popen(cmd, "r");
	fgets(client->_cur, sizeof(client->_cur), f);
	pclose(f);
	removeCharSelf(client->_cur, '\n');
	strcat(client->_cur, "/");

	#ifdef __DEBUG__
	printf("Cur path : %s\n", client->_cur);
	#endif

	sendMsg(client, success, strlen(success));
}

void handle_MKD(struct FtpClient *client, char *args)
{
	//在这里要注意权限判断，目前暂时没有加入
	
	char *err = "550 Create directory failed\r\n";
	char *success = "250 Create directory success\r\n";
	int r;
	char path[PATH_SIZE];

	dealPath(client, path, args);
	r = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP);
	if(r != 0)
		sendMsg(client, err, strlen(err));
	else
		sendMsg(client, success, strlen(success));
}

void handle_RMD(struct FtpClient *client, char *args)
{
	char path[PATH_SIZE];
	int r;
	char *err = "550 Delete directory failed\r\n";
	char *success = "250 Delete directory success\r\n";

	memset(path, 0, sizeof(path));
	dealPath(client, path, args);
	r = rmdir(path);
	if(r != 0)
		sendMsg(client, err, strlen(err));
	else
		sendMsg(client, success, strlen(success));
}

void handle_CDUP(struct FtpClient *client)
{
	handle_CWD(client, "../");
}

void handle_RNFR(struct FtpClient *client, char *args)
{
	//因为在这里要进行对全局的origin path 和 new
	//path这两个变量的调用，所以，必须加锁，和使用条件变量
	//这里有个很有趣的问题，那就是在main函数之前就对mutex 和 condition进行初始化。
	//我记得《程序员的自我修养》里面介绍过相应的方法，等下实现一下
	
	lock(&mutex);
	memset(oldPath, 0, sizeof(oldPath));
	dealPath(client, oldPath, args);
	unlock(&mutex);
}

void handle_RNTO(struct FtpClient *client, char *args)
{
	int r;
	char *err = "550 change name failed\r\n";
	char *success = "250 change name success\r\n";
	lock(&mutex);
	memset(newPath, 0, sizeof(newPath));
	dealPath(client, newPath, args);
	r = rename(oldPath, newPath);
	unlock(&mutex);
	if(r != 0)
		sendMsg(client, err, strlen(err));
	else
		sendMsg(client, success, strlen(success));
}

