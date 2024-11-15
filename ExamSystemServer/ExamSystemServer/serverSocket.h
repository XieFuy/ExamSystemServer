#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include<string.h>
#include<unistd.h>
enum userOpeator //用户操作的命令类型
{
	
};

class CServerSocket //套接字类，网络通信的主要负责类，这里采用饿汉式单例设计模式
{
public:
	static CServerSocket* getInstance();
	sockaddr_in sockAddrClient;
	int m_sockClient;
public:
	int Bind();
	int Listen();
	void initSocket(); //初始化套接字
	bool acceptClient();//接收客户端
	int Send(char* pData); //向服务器发送数据 ,每次发送一个数据包大小的数据
	int Recv(char* buffer);//接收服务器发送的数据,每次接收一个数据包大小的数据
	void closeSocket();
	char* getPacket();
	long long getPacketSize(); //返回整个数据包的大小
	void makePacket(char* pData, size_t length, short cmd);//进行封包操作
	int getSocketSerever();
private:
private:
	CServerSocket();
	~CServerSocket();
	CServerSocket(const CServerSocket& clientsocket) = delete;
	CServerSocket& operator=(const CServerSocket& clientSocket) = delete;
	static void releaseInstance();
	class CHelper
	{
	public:
		CHelper();
		~CHelper();
	};
private:
	char* m_packet = nullptr;//数据包 ，包含两字节包头，四字节数据长度、两字节命令、数据部分 加上一个'\0'
	long long m_packetSize; //一个数据包的大小
	int m_serverSocket;
	sockaddr_in m_sockAddrServer;
	static CServerSocket* m_instance;
	static CHelper m_helper;
};
