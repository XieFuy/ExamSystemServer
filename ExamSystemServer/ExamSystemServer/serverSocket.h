#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include<string.h>
#include<unistd.h>
enum userOpeator //�û���������������
{
	
};

class CServerSocket //�׽����࣬����ͨ�ŵ���Ҫ�����࣬������ö���ʽ�������ģʽ
{
public:
	static CServerSocket* getInstance();
	sockaddr_in sockAddrClient;
	int m_sockClient;
public:
	int Bind();
	int Listen();
	void initSocket(); //��ʼ���׽���
	bool acceptClient();//���տͻ���
	int Send(char* pData); //��������������� ,ÿ�η���һ�����ݰ���С������
	int Recv(char* buffer);//���շ��������͵�����,ÿ�ν���һ�����ݰ���С������
	void closeSocket();
	char* getPacket();
	long long getPacketSize(); //�����������ݰ��Ĵ�С
	void makePacket(char* pData, size_t length, short cmd);//���з������
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
	char* m_packet = nullptr;//���ݰ� ���������ֽڰ�ͷ�����ֽ����ݳ��ȡ����ֽ�������ݲ��� ����һ��'\0'
	long long m_packetSize; //һ�����ݰ��Ĵ�С
	int m_serverSocket;
	sockaddr_in m_sockAddrServer;
	static CServerSocket* m_instance;
	static CHelper m_helper;
};
