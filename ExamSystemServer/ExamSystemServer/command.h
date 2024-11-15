#pragma once

#include<map>
#include "threadPool.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
class CCommand
{
public:
	CCommand();
	~CCommand();
	void getHeadPicture(char* filePath,int sockClient,int epfd);//根据路径读取本地头像图片信息，并且将内容发送
	int Excute(int cmd,char* data,int sockClient,int epfd); 
public:
	CThreadPool* m_threadPool;
private:
	pthread_mutex_t m_mutex;
	typedef void (CCommand::* FUNC)(char*,int,int);
	std::map<int, FUNC> m_funcMap;
};

