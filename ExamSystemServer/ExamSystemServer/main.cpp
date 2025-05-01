#include <unistd.h>
#include "serverSocket.h"
#include"command.h"
#include <sys/epoll.h>
#include <sys/errno.h>
#include <memory>

//确保要封装的类
/*
1、网络模块类
2、线程池类
3、任务类 

网络IO模型使用epoll,能够处理高并发的场景需求
*/

/*
服务端出现的问题：1、内存泄漏，应该存在，在程序不正常的时候进程占用内存229MB
                  2、进程在持续一段时间后会出现阻塞现象，应该是网络的I/O阻塞或者是epoll_wait阻塞
*/
struct Arg
{
	int epfd;
	int sockClient;
	const char* packet;
};

static CCommand command;

void* threadWork(void* arg)
{
	std::shared_ptr<Arg>* p = static_cast<std::shared_ptr<Arg>*>(arg);
	std::shared_ptr<Arg> myArg = *p;
	//Arg* myArg = (Arg*)arg;

	//如果是客户端的套接字有响应,接收客户端发送的消息，解包，将任务放进线程池
	//这里使用的是短连接，当客户端进行完成一次通信结束连接的时候需要从集合中移除客户端的fd
	//如果套接字错误
	if (myArg->sockClient <= 0)
	{
		return nullptr;
	}

	//解包，在外部进行接收完整的数据包后进行解析
	char* pData = const_cast<char*>(myArg->packet);
	short head = *(reinterpret_cast<short*>(pData));
	pData += 2;
	unsigned int length = *reinterpret_cast<unsigned int*>(pData);
	pData += 4;
	short cmd = *reinterpret_cast<short*>(pData);
	printf("packet: head:%d length:%d cmd:%d", head, length, cmd);
	pData += 2;
	char* data = new char[length + 1]; //这里是全数据,并且包含一个\0
	memset(data, '\0', length * sizeof(char) + 1);
	memcpy(data, pData, length);
	delete[] myArg->packet;
	//根据命令找出对应的任务
	int sockClient = myArg->sockClient;
	int epfd = myArg->epfd;
	command.Excute(cmd,data,sockClient,epfd,length);
	delete p;
}

int main() //在线考试系统服务端 //网络IO模型使用epoll ,工作任务使用线程池
{
	//搭建epllo模型
	CServerSocket* server = CServerSocket::getInstance();
	server->initSocket();
	server->Bind();
	server->Listen();

	//创建一个epoll对象
	int epfd = epoll_create(1);
	int epl_cnt; //有反应的文件描述符总数量
	std::unique_ptr<epoll_event[]> allEvents(new epoll_event[1000]);
	//epoll_event* allEvents = new epoll_event[1000]; //容器队列，能存储的fd队列
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
		epl_cnt = epoll_wait(epfd, allEvents.get(), 1000, -1);
		if (epl_cnt < 0)
		{
			printf("epllo_wait error , errno:%d.\r\n", errno);
			return -1;
		}
		else if (epl_cnt == 0)
		{
			//printf("no Event!\r\n");
			continue;
		}
		else
		{
			for (int i = 0; i < epl_cnt; i++)
			{
				if (allEvents[i].data.fd == server->getSocketSerever()) //如果有反应的是服务端的套接字
				{
					server->m_sockClient = accept(allEvents[i].data.fd, (sockaddr*)&server->sockAddrClient, &sockLen);
					if (server->m_sockClient <= 0)
					{
						printf("server accept Error:%d\r\n",errno);
						continue;
					}
					//printf("sucess accept client!\r\n");
					//将客户端的套接字进行添加到集合中
					epoEvent.data.fd = server->m_sockClient;
					epoEvent.events = EPOLLIN | EPOLLET; //设置为边缘触发，并且关注epllo的读事件
					epoll_ctl(epfd, EPOLL_CTL_ADD, server->m_sockClient, &epoEvent);
					printf("\naccept client!\r\n");
				}
				else
				{	
					printf("cliect socket active:%d\r\n", allEvents[i].data.fd);
					if (allEvents[i].events == EPOLLIN)
					{
					//虽然边缘触发有数据来的时候，只会触发epoll_wait一次，而水平触发只要有数据来，就会一直触发epoll_wait
					/*导致的可能原因是客户端发送的包的大小是 2097161服务端不能一次性接收完毕，所以得确保接收到的是一个数据包后才能开启子线程进行工作*/
	               //进行接收网络消息
					char* packet = new char[2 + 4 + 2 + (1024 * 500) + 1]; //接收一个数据包的大小
					memset(packet, '\0', sizeof(char) * (2 + 4 + 2 + (1024 * 500) + 1));
					long long packetSize = 2 + 4 + 2 + (1024 * 500) + 1;
					long long readSize = 0;
					while (readSize < packetSize)
					{
						//printf("start recv!\n");
						ssize_t ret = read(allEvents[i].data.fd, packet + readSize, packetSize - readSize);
						if (ret <= 0)//接收出现问题
						{
							printf("server recive error!\r\n");
							// 关闭 socket 并从 epoll 移除
							close(allEvents[i].data.fd);
							epoll_ctl(epfd, EPOLL_CTL_DEL, allEvents[i].data.fd, nullptr);
							// 释放已分配的内存
							delete[] packet;
							packet = nullptr;
							break;
						}
						readSize += ret;
					}
					//如果出现异常，那么packet指针就是nullptr
					if (packet == nullptr)
					{
						continue;
					}
					printf("recv end!\r\n");
					//在这里检查如果发送的数据包的包头不是0xFEFF,就不允许创建子线程
					short head = 0xFEFF;
					short recvHead;
					memcpy(&recvHead, packet, sizeof(short));
					if (head != recvHead)
					{
						continue;
					}
					std::shared_ptr<Arg> arg = std::make_shared<Arg>();
					//Arg* arg = new Arg();
					arg->sockClient = allEvents[i].data.fd;
					arg->epfd = epfd;
					arg->packet = packet;
					std::shared_ptr<Arg>* pArg = new std::shared_ptr<Arg>(arg);
					pthread_t  thread;
					pthread_create(&thread,nullptr,&threadWork,pArg);
					pthread_detach(thread); //避免僵尸线程的出现
					}
				}
			}
		}
	}
	return 0;
}