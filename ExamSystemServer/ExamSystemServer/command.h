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
	void getHeadPicture(char* filePath,int sockClient,int epfd,int dataLenght);//根据路径读取本地头像图片信息，并且将内容发送
	void upLoadHeadPicture(char* pData,int sockClient,int epfd,int dataLenght); //客户端上传头像信息到服务器磁盘进行存储
	int Excute(int cmd,char* data,int sockClient,int epfd,int dataLength); 
public:
	CThreadPool* m_threadPool;
	void Dump(const char* Data, size_t nSize)  //打印输出测试设计的包的数据是什么
	{
		std::string strOut;  //用于存储整个包的数据的结果
		//strOut.resize(nSize);
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0))
			{
				strOut += '\n';
			}
			snprintf(buf, sizeof(buf), "%02X", Data[i] & 0xFF);  //%02X的意思是 写入的十六进制占两位，不足位是填充前导零 
			strOut += buf;
		}
		strOut += "\n";
		printf("%s",strOut.c_str()); //用于输出调试的字符串，类似于QDebug
	}
private:
	pthread_mutex_t m_mutex;
	pthread_mutex_t m_mutex2;
	typedef void (CCommand::* FUNC)(char*,int,int,int);
	std::map<int, FUNC> m_funcMap;
};

