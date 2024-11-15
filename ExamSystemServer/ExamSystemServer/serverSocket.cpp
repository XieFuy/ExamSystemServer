#include "serverSocket.h"

CServerSocket* CServerSocket::m_instance = nullptr;
CServerSocket::CHelper  CServerSocket::m_helper;

CServerSocket::CServerSocket()
{
	this->m_packetSize = 2 + 4 + 2 + (1024 * 1024 * 2) + 1;
	this->m_packet = new char[this->m_packetSize]; //数据部分的大小是2MB的大小
	memset(this->m_packet, '\0', sizeof(char)*this->m_packetSize);
}

CServerSocket::~CServerSocket()
{
	if (this->m_packet != nullptr)
	{
		delete[] this->m_packet;
		this->m_packet = nullptr;
	}
}

CServerSocket* CServerSocket::getInstance()
{
	if (CServerSocket::m_instance == nullptr)
	{
		CServerSocket::m_instance = new CServerSocket();
	}
	return CServerSocket::m_instance;
}

void CServerSocket::releaseInstance()
{
	if (CServerSocket::m_instance != nullptr)
	{
		delete CServerSocket::m_instance;
		CServerSocket::m_instance = nullptr;
	}
}

CServerSocket::CHelper::CHelper()
{
	CServerSocket::getInstance();
}

CServerSocket::CHelper::~CHelper()
{
	CServerSocket::releaseInstance();
}

int CServerSocket::Bind()
{
	if (bind(this->m_serverSocket,(sockaddr*)&this->m_sockAddrServer,sizeof(sockaddr)) < 0)
	{
		return -1;
	}
	return 0;
}

int CServerSocket::getSocketSerever()
{
	return this->m_serverSocket;
}

int CServerSocket::Listen()
{
	if (listen(this->m_serverSocket,10) < 0)
	{
		return -1;
	}
	return 0;
}

void CServerSocket::initSocket()
{
	this->m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->m_serverSocket <= 0)
	{
		return;
	}
	this->m_sockAddrServer.sin_port = htons(9527); //客户端连接9527端口
	this->m_sockAddrServer.sin_family = AF_INET;
	this->m_sockAddrServer.sin_addr.s_addr = htonl(INADDR_ANY);
}

bool CServerSocket::acceptClient()
{
	socklen_t len = sizeof(sockaddr);
	if ((this->m_sockClient =  accept(this->m_serverSocket, (sockaddr*)&this->sockAddrClient,&len)) < 0)
	{
		return false;
	}
	int size = 1024 * 1024 * 2;
	setsockopt(this->m_sockClient, SOL_SOCKET, SO_RCVBUF, (const void *)&size, sizeof(size));
	return true;
}

int CServerSocket::Send(char* pData)
{
	if (pData == nullptr)
	{
		return -1;
	}
	return  send(this->m_sockClient, pData, this->m_packetSize, 0);
}

int CServerSocket::Recv(char* buffer)
{
	if (buffer == nullptr)
	{
		return -1;
	}
	return read(this->m_sockClient, buffer, this->m_packetSize);
}

void CServerSocket::closeSocket()
{
	if (this->m_sockClient > 0)
	{
		close(this->m_sockClient);
		this->m_sockClient = -1;
	}
}

char* CServerSocket::getPacket()
{
	return this->m_packet;
}

long long CServerSocket::getPacketSize()
{
	return this->m_packetSize;
}

void CServerSocket::makePacket(char* pData, size_t length, short cmd) //注意单个数据包的数据位长度不能超过2MB
{
	if (pData == nullptr || length <= 0)
	{
		return;
	}
	short head = 0xFEFF;
	unsigned int  dataLength = length; //确保是四字节
	memset(this->m_packet, '\0', this->m_packetSize * sizeof(char));
	memcpy(this->m_packet, &head, sizeof(short));
	memcpy(this->m_packet + 2, &dataLength, sizeof(unsigned int));
	memcpy(this->m_packet + 2 + 4, &cmd, sizeof(unsigned int));
	memcpy(this->m_packet + 2 + 4 + 2, pData, length);
}
