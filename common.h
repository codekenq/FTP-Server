#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <sys/socket.h>
/*
 * bind()
 * */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>

#define	IPV4_LEN		15
#define DATA_BUF_SIZE	1024
#define CMD_BUF_SIZE	1024	//这个是用户输入的一串命令的buf的Size，包含cmd和args
#define CMD_SIZE		10		//这个只是cmd的Size
#define ARGS_SIZE		1014	//这个只是args的Size
#define STATUS_MSG_SIZE	1024
#define USER_NAME_SIZE	20
#define USER_PASS_SIZE	20
#define PATH_SIZE		100

#define True			1
#define False			0

#endif /* _COMMON_H_ */
