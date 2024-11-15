#include "command.h"

CCommand::CCommand()
{
	this->m_threadPool = new CThreadPool(4);
	pthread_mutex_init(&this->m_mutex,nullptr);
	struct arr {
		int cmd;
		FUNC func;
	} Arr[]
	{
		{0,&CCommand::getHeadPicture},
	    {-1,nullptr}
	};

	for (int i = 0 ; i < sizeof(Arr) / sizeof(Arr[0]);i++)
	{
		this->m_funcMap.insert(std::make_pair(Arr[i].cmd,Arr[i].func));
	}
}

CCommand::~CCommand()
{
	if (this->m_threadPool != nullptr)
	{
		delete this->m_threadPool;
	}
}

/*
总结对于大的IO操作，都需要走三步
1、定义好总量
2、循环读
3、循环写
*/
void CCommand::getHeadPicture(char* filePath, int sockClient, int epfd)
{
	//将任务函数添加到线程池中,等执行任务完毕关闭套接字，并且将文件描述符从fd中移除,并且需要释放传入的数据部分
	FILE* pFile = fopen(filePath,"rb+");
	if (pFile == nullptr)
	{
		delete filePath;
		fclose(pFile);
		close(sockClient);
		pthread_mutex_lock(&this->m_mutex);
		epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
		pthread_mutex_unlock(&this->m_mutex);
		return;
	}
	fseek(pFile,0,SEEK_END);
	off_t fileSize = ftello64(pFile);
	fseek(pFile,0,SEEK_SET);
	char* data = new char[1024 * 1024 * 2]; 
	memset(data,'\0',sizeof(char)* 1024 * 1024 * 2);

	size_t ret =  fread(data,1, fileSize,pFile);
	printf("read file size:%d\r\n",ret);
	//封包
	char* packet = new char[2 + 4 + 2 + (1024 * 1024 * 2) + 1];
	char* p = packet;
	short head = 0xFEFF;
	unsigned int length = fileSize;
	short cmd = 0;
	printf("data length:%d\r\n",length);
	memcpy(p,&head,sizeof(head));
	p += sizeof(short);
	memcpy(p,&length,sizeof(length));
	p += sizeof(unsigned int);
	memcpy(p,&cmd,sizeof(cmd));
	p += sizeof(short);
	memcpy(p,data,length);
	p += length;

	//发送数据包
	long long  sendCount = 2 + 4 + 2 + (1024 * 1024 * 2) + 1;
	long long alReadySend = 0;
	while (true)
	{
		ssize_t size = write(sockClient, packet + alReadySend,sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}
	printf("send size:%d\r\n",alReadySend);
	if (alReadySend < 0)
	{
		printf("send Error!\r\n");
	}
	delete[] filePath;
	close(sockClient);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL,sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
}

int CCommand::Excute(int cmd,char* data, int sockClient, int epfd)
{
	auto ret =  this->m_funcMap.find(cmd);
	if (ret != this->m_funcMap.end())
	{
		//将任务放到线程池中
		this->m_threadPool->addTask([=]() {  (this->*ret->second)(data,sockClient,epfd); }); //添加任务是线程安全的
		return 0;
	}
	return -1;
}