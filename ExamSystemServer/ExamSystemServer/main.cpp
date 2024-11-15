#include <unistd.h>
#include "serverSocket.h"
#include"command.h"
#include <sys/epoll.h>


//确保要封装的类
/*
1、网络模块类
2、线程池类
3、任务类 

网络IO模型使用epoll,能够处理高并发的场景需求
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
	//解包，在外部进行接收完整的数据包后进行解析
	char* pData = myArg->packet;
	short head = *reinterpret_cast<short*>(pData);
	pData += 2;
	unsigned int length = *reinterpret_cast<unsigned int*>(pData);
	pData += 4;
	short cmd = *reinterpret_cast<short*>(pData);
	pData += 2;
	char* data = new char[length+1]; //这里是全数据,并且包含一个\0
	memset(data,'\0',length * sizeof(char) + 1);
	memcpy(data,pData,length);
	//delete data;
	delete[] myArg->packet;
	//根据命令找出对应的任务
	int sockClient = myArg->sockClient;
	int epfd = myArg->epfd;
	command.Excute(cmd,data,sockClient,epfd);
	delete myArg;
	pthread_exit(nullptr);
}

int main() //在线考试系统服务端 //网络IO模型使用epoll ,工作任务使用线程池
{
	//搭建epllo模型
	CServerSocket* server = CServerSocket::getInstance();
	server->initSocket();
	server->Bind();
	server->Listen();
	//server->acceptClient();

#if 0
	//char* packet = server->getPacket();
	char* recvBuffer = new char[2 + 4 + 2 + (1024 * 1024 * 2) + 1];
	int count = 0;
	while (true) //得出结果，在公网中，客户端send很大的数据，在服务端需要执行很多次recv才能接收完毕一个数据包
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
	//创建一个epoll对象
	int epfd = epoll_create(1);
	int epl_cnt; //有反应的文件描述符总数量
	epoll_event* allEvents = new epoll_event[1000]; //容器队列，能存储的fd队列
	epoll_event epoEvent;
	epoEvent.events = EPOLLIN;
	epoEvent.data.fd = server->getSocketSerever();
	//将sockServer添加到集合中
	int ret =  epoll_ctl(epfd, EPOLL_CTL_ADD, server->getSocketSerever(), &epoEvent);
	int sockClient;
	socklen_t sockLen = sizeof(sockaddr);

	//进行不断的监听集合
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
				if (allEvents[i].data.fd == server->getSocketSerever()) //如果有反应的是服务端的套接字
				{
					server->m_sockClient = accept(allEvents[i].data.fd, (sockaddr*)&server->sockAddrClient, &sockLen);
					//printf("sucess accept client!\r\n");
					//将客户端的套接字进行添加到集合中
					epoEvent.data.fd = server->m_sockClient;
					epoEvent.events = EPOLLIN; //设置为边缘触发
					epoll_ctl(epfd, EPOLL_CTL_ADD, server->m_sockClient, &epoEvent);
					printf("accept client!\r\n");
				}
				else
				{
					printf("recv client!\r\n");
					/*导致的可能原因是客户端发送的包的大小是 2097161
					服务端不能一次性接收完毕，
					所以得确保接收到的是一个数据包后才能开启子线程进行工作*/
					//进行接收网络消息
					char* packet = new char[2 + 4 + 2 + (1024 * 1024 * 2) + 1]; //接收一个数据包的大小
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
					//如果是客户端的套接字有响应,接收客户端发送的消息，解包，将任务放进线程池
					//这里使用的是短连接，当客户端进行完成一次通信结束连接的时候需要从集合中移除客户端的fd
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