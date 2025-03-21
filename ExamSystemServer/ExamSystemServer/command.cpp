#include "command.h"

CCommand::CCommand()
{
	this->m_threadPool = new CThreadPool(4);
	pthread_mutex_init(&this->m_mutex,nullptr);
	pthread_mutex_init(&this->m_mutex2,nullptr);
	struct arr {
		int cmd;
		FUNC func;
	} Arr[]
	{
		{0,&CCommand::getHeadPicture},
	    {1,&CCommand::upLoadHeadPicture},
	    {2,&CCommand::getClassTbaleIcon},
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

void CCommand::getClassTbaleIcon(char* filePath, int sockClient, int epfd, int dataLenght)
{
	printf("path:%s\r\n", filePath);
	//将任务函数添加到线程池中,等执行任务完毕关闭套接字，并且将文件描述符从fd中移除,并且需要释放传入的数据部分
	FILE* pFile = fopen(filePath, "rb+");
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
	fseek(pFile, 0, SEEK_END);
	off_t fileSize = ftello64(pFile);
	fseek(pFile, 0, SEEK_SET);

	//先向客户端发送文件大小
	long long temp = fileSize;
	char* strFileSize = new char[8];
	memset(strFileSize,'\0',sizeof(char) * 8);
	memcpy(strFileSize,&temp,sizeof(temp));

	//完全发送出这八个字节
	long long  sendCount = 8;
	long long alReadySend = 0;
	while (alReadySend < sendCount)
	{
		ssize_t size = write(sockClient, strFileSize + alReadySend, sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}
	delete[] strFileSize;

	//进行读文件，将文件的所有
	char* fileData = new char[fileSize];
	memset(fileData,'\0',sizeof(char) * fileSize);
	size_t ret = fread(fileData, 1, fileSize, pFile);
	fclose(pFile);

	sendCount = fileSize;
	alReadySend =  0;
	while (alReadySend < sendCount)
	{
		ssize_t size = write(sockClient, fileData + alReadySend, sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}
	delete[] fileData;


	//char* data = new char[1024 * 1024 * 2]; 
	//memset(data,'\0',sizeof(char)* 1024 * 1024 * 2);

	/*char* data = new char[1024 * 500];
	memset(data, '\0', sizeof(char) * 1024 * 500);
	size_t ret = fread(data, 1, fileSize, pFile);
	//this->Dump(data,ret);
	printf("read file size:%d\r\n", ret);
	fclose(pFile);
	//封包
	char* packet = new char[2 + 4 + 2 + (1024 * 500) + 1];
	char* p = packet;
	short head = 0xFEFF;
	unsigned int length = fileSize;
	short cmd = 0;
	printf("data length:%d\r\n", length);
	memcpy(p, &head, sizeof(head));
	p += sizeof(short);
	memcpy(p, &length, sizeof(length));
	p += sizeof(unsigned int);
	memcpy(p, &cmd, sizeof(cmd));
	p += sizeof(short);
	memcpy(p, data, length);
	p += length;

	//pthread_mutex_lock(&this->m_mutex2);
	//发送数据包
	long long  sendCount = 2 + 4 + 2 + (1024 * 500) + 1;
	long long alReadySend = 0;
	while (true)
	{
		ssize_t size = write(sockClient, packet + alReadySend, sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}
	printf("file:%s  send size:%d\r\n", filePath, alReadySend);
	if (alReadySend < 0)
	{
		printf("send Error!\r\n");
	}
	delete[] filePath;*/
	close(sockClient);
	//pthread_mutex_unlock(&this->m_mutex2);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
}


void CCommand::upLoadHeadPicture(char* pData, int sockClient, int epfd,int dataLength)
{
	if (pData == nullptr)
	{
		return;
	}

	//对传输过来的数据进行解析
	char* p = pData;
	short pathLength;
	memcpy(&pathLength,p,sizeof(short));
	p += sizeof(short);
	unsigned int fileDataLength = dataLength - pathLength - 2;
	printf("allDataLength:%d  pathlength: %d  fileDataLengthL:%d\r\n",dataLength,pathLength,fileDataLength);
	char* pictureData = new char[fileDataLength];
	memcpy(pictureData,p,fileDataLength);
	p += fileDataLength;

	char* path = new char[pathLength + 1];
	memset(path,'\0',sizeof(char)*(pathLength + 1));
	memcpy(path,p,pathLength);

	//进行文件写操作
	FILE* pFile = fopen(path,"wb+");

	if (pFile == nullptr)
	{
		fclose(pFile);
		printf("file open failed!\r\n");
		return;
	}

	long long alReadyWrite = 0;
	while (true)
	{
		size_t ret =  fwrite(pictureData + alReadyWrite,1, fileDataLength - alReadyWrite,pFile);
		if (ret <= 0)
		{
			break;
		}
		alReadyWrite += ret;
	}
	fclose(pFile);
	delete[] path;
	delete[] pictureData;
	close(sockClient);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
	if (pData != nullptr)
	{
		delete[] pData;
	}
}

/*
总结对于大的IO操作，都需要走三步
1、定义好总量
2、循环读
3、循环写
*/
void CCommand::getHeadPicture(char* filePath, int sockClient, int epfd,int dataLength)
{
	
	printf("path:%s\r\n",filePath);
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
	//char* data = new char[1024 * 1024 * 2]; 
	//memset(data,'\0',sizeof(char)* 1024 * 1024 * 2);

	char* data = new char[1024 * 500]; 
	memset(data,'\0',sizeof(char)* 1024 * 500);
	size_t ret =  fread(data,1, fileSize,pFile);
	//this->Dump(data,ret);
	printf("read file size:%d\r\n",ret);
	fclose(pFile);
	//封包
	char* packet = new char[2 + 4 + 2 + (1024 * 500) + 1];
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

	//pthread_mutex_lock(&this->m_mutex2);
	//发送数据包
	long long  sendCount = 2 + 4 + 2 + (1024 * 500) + 1;
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
	printf("file:%s  send size:%d\r\n",filePath,alReadySend);
	if (alReadySend < 0)
	{
		printf("send Error!\r\n");
	}
	delete[] filePath;
	close(sockClient);
	//pthread_mutex_unlock(&this->m_mutex2);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL,sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
}

int CCommand::Excute(int cmd,char* data, int sockClient, int epfd,int dataLength)
{
	auto ret =  this->m_funcMap.find(cmd);
	if (ret != this->m_funcMap.end())
	{
		//将任务放到线程池中
		this->m_threadPool->addTask([=]() {  (this->*ret->second)(data,sockClient,epfd,dataLength); }); //添加任务是线程安全的
		return 0;
	}
	return -1;
}