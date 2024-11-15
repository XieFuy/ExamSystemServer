#include <unistd.h>
#include "serverSocket.h"
#include"command.h"
#include <sys/epoll.h>


//ȷ��Ҫ��װ����
/*
1������ģ����
2���̳߳���
3�������� 

����IOģ��ʹ��epoll,�ܹ�����߲����ĳ�������
*/
struct Arg
{
	int epfd;
	int sockClient;
	char* packet;
};

CCommand command;

void* threadWork(void* arg)
{
	Arg* myArg = (Arg*)arg;
	//��������ⲿ���н������������ݰ�����н���
	char* pData = myArg->packet;
	short head = *reinterpret_cast<short*>(pData);
	pData += 2;
	unsigned int length = *reinterpret_cast<unsigned int*>(pData);
	pData += 4;
	short cmd = *reinterpret_cast<short*>(pData);
	pData += 2;
	char* data = new char[length+1]; //������ȫ����,���Ұ���һ��\0
	memset(data,'\0',length * sizeof(char) + 1);
	memcpy(data,pData,length);
	//delete data;
	delete[] myArg->packet;
	//���������ҳ���Ӧ������
	int sockClient = myArg->sockClient;
	int epfd = myArg->epfd;
	command.Excute(cmd,data,sockClient,epfd);
	delete myArg;
	pthread_exit(nullptr);
}

int main() //���߿���ϵͳ����� //����IOģ��ʹ��epoll ,��������ʹ���̳߳�
{
	//�eplloģ��
	CServerSocket* server = CServerSocket::getInstance();
	server->initSocket();
	server->Bind();
	server->Listen();
	//server->acceptClient();

#if 0
	//char* packet = server->getPacket();
	char* recvBuffer = new char[2 + 4 + 2 + (1024 * 1024 * 2) + 1];
	int count = 0;
	while (true) //�ó�������ڹ����У��ͻ���send�ܴ�����ݣ��ڷ������Ҫִ�кܶ��recv���ܽ������һ�����ݰ�
	{
		int size = server->Recv(recvBuffer);
		if (size > 0)
		{
			count += size;
		}
		else
		{
			break;
		}
	}
	
	delete[] recvBuffer;
	printf("recv Size:%d\r\n",count);
	while (1);
#endif 
	//����һ��epoll����
	int epfd = epoll_create(1);
	int epl_cnt; //�з�Ӧ���ļ�������������
	epoll_event* allEvents = new epoll_event[1000]; //�������У��ܴ洢��fd����
	epoll_event epoEvent;
	epoEvent.events = EPOLLIN;
	epoEvent.data.fd = server->getSocketSerever();
	//��sockServer��ӵ�������
	int ret =  epoll_ctl(epfd, EPOLL_CTL_ADD, server->getSocketSerever(), &epoEvent);
	int sockClient;
	socklen_t sockLen = sizeof(sockaddr);

	//���в��ϵļ�������
	while (true)
	{
		epl_cnt = epoll_wait(epfd, allEvents, 1000, 1000);
		if (epl_cnt < 0)
		{
			printf("epllo_wait error , errno:%d.\r\n", errno);
			return -1;
		}
		else if (epl_cnt == 0)
		{
			printf("no Event!\r\n");
			continue;
		}
		else
		{
			for (int i = 0; i < epl_cnt; i++)
			{
				if (allEvents[i].data.fd == server->getSocketSerever()) //����з�Ӧ���Ƿ���˵��׽���
				{
					server->m_sockClient = accept(allEvents[i].data.fd, (sockaddr*)&server->sockAddrClient, &sockLen);
					//printf("sucess accept client!\r\n");
					//���ͻ��˵��׽��ֽ�����ӵ�������
					epoEvent.data.fd = server->m_sockClient;
					epoEvent.events = EPOLLIN; //����Ϊ��Ե����
					epoll_ctl(epfd, EPOLL_CTL_ADD, server->m_sockClient, &epoEvent);
					printf("accept client!\r\n");
				}
				else
				{
					printf("recv client!\r\n");
					/*���µĿ���ԭ���ǿͻ��˷��͵İ��Ĵ�С�� 2097161
					����˲���һ���Խ�����ϣ�
					���Ե�ȷ�����յ�����һ�����ݰ�����ܿ������߳̽��й���*/
					//���н���������Ϣ
					char* packet = new char[2 + 4 + 2 + (1024 * 1024 * 2) + 1]; //����һ�����ݰ��Ĵ�С
					memset(packet, '\0', sizeof(char) * (2 + 4 + 2 + (1024 * 1024 * 2) + 1));
					long long packetSize = 2 + 4 + 2 + (1024 * 1024 * 2) + 1;
					long long readSize = 0;
					while (readSize < packetSize)
					{
						ssize_t ret  = read(allEvents[i].data.fd,packet + readSize,packetSize - readSize);
						if (ret <= 0)
						{
							break;
						}
						readSize += ret;
					}
					//����ǿͻ��˵��׽�������Ӧ,���տͻ��˷��͵���Ϣ�������������Ž��̳߳�
					//����ʹ�õ��Ƕ����ӣ����ͻ��˽������һ��ͨ�Ž������ӵ�ʱ����Ҫ�Ӽ������Ƴ��ͻ��˵�fd
					Arg* arg = new Arg();
					arg->sockClient = allEvents[i].data.fd;
					arg->epfd = epfd;
					arg->packet = packet;
					pthread_t  thread;
					pthread_create(&thread,nullptr,&threadWork,arg);
				}
			}
		}
	}
	delete[]  allEvents;
	return 0;
}