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
	void getHeadPicture(char* filePath,int sockClient,int epfd,int dataLenght);//����·����ȡ����ͷ��ͼƬ��Ϣ�����ҽ����ݷ���
	void upLoadHeadPicture(char* pData,int sockClient,int epfd,int dataLenght); //�ͻ����ϴ�ͷ����Ϣ�����������̽��д洢
	int Excute(int cmd,char* data,int sockClient,int epfd,int dataLength); 
public:
	CThreadPool* m_threadPool;
private:
	pthread_mutex_t m_mutex;
	typedef void (CCommand::* FUNC)(char*,int,int,int);
	std::map<int, FUNC> m_funcMap;
};

