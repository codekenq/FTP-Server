#ifndef _DEALCMD_H_
#define _DEALCMD_H_

#include "common.h"
#include "ftpServer.h"
#include "log.h"
#include "_string.h"

#include "_pthread.h"

extern MUTEX mutex;

struct FtpClient;		//不加这个会出现‘struct FtpClient’ declared inside parameter list
						//但是前面已经包含ftpServer.h了，看来还是有的地方忘了。

void handle_USER(struct FtpClient *, char *);
void handle_PASS(struct FtpClient *, char *);
void handle_SYST(struct FtpClient *);
void handle_TYPE(struct FtpClient * , char *);
void handle_PORT(struct FtpClient *, char *);
void handle_RETR(struct FtpClient *, char *);
void handle_PASV(struct FtpClient *);
void handle_QUIT(struct FtpClient *);
void handle_LIST(struct FtpClient *, char *);
void handle_PWD(struct FtpClient *);
void handle_STOR(struct FtpClient *, char *);
void handle_CWD(struct FtpClient *, char *);
void handle_MKD(struct FtpClient *, char *);
void handle_RMD(struct FtpClient *, char *);
void handle_CDUP(struct FtpClient *);
void handle_RNFR(struct FtpClient *, char *args);
void handle_RNTO(struct FtpClient *, char *args);

#endif /* _DEALCMD_H_ */
