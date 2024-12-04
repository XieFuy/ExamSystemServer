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
	void getClassTbaleIcon(char* filePath,int sockClient, int epfd, int dataLenght);
	void getHeadPicture(char* filePath,int sockClient,int epfd,int dataLenght);//����·����ȡ����ͷ��ͼƬ��Ϣ�����ҽ����ݷ���
	void upLoadHeadPicture(char* pData,int sockClient,int epfd,int dataLenght); //�ͻ����ϴ�ͷ����Ϣ�����������̽��д洢
	int Excute(int cmd,char* data,int sockClient,int epfd,int dataLength); 
public:
	CThreadPool* m_threadPool;
	void Dump(const char* Data, size_t nSize)  //��ӡ���������Ƶİ���������ʲô
	{
		std::string strOut;  //���ڴ洢�����������ݵĽ��
		//strOut.resize(nSize);
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0))
			{
				strOut += '\n';
			}
			snprintf(buf, sizeof(buf), "%02X", Data[i] & 0xFF);  //%02X����˼�� д���ʮ������ռ��λ������λ�����ǰ���� 
			strOut += buf;
		}
		strOut += "\n";
		printf("%s",strOut.c_str()); //����������Ե��ַ�����������QDebug
	}
private:
	pthread_mutex_t m_mutex;
	pthread_mutex_t m_mutex2;
	typedef void (CCommand::* FUNC)(char*,int,int,int);
	std::map<int, FUNC> m_funcMap;
};

