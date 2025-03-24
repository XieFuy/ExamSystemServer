#include "command.h"

class Test {
public:
	Test() {
		printf("Test is init!\r\n");
	}

	~Test(){
		printf("Test is delete!\r\n");
	}
};

CCommand::CCommand()
{
	this->m_threadPool = new CThreadPool(4);
	pthread_mutex_init(&this->m_mutex,nullptr);
	pthread_mutex_init(&this->m_mutex2,nullptr);
	pthread_mutex_init(&this->m_mutex_3,nullptr);
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

//每个函数都要检查传进来的数组是否delete[] ,并且打开的文件指针是否fclose，还有epoll移出文件描述符，其余全换成智能指针
void CCommand::getClassTbaleIcon(char* filePath, int sockClient, int epfd, int dataLenght)
{
	std::shared_ptr<Test> test = std::make_shared<Test>();
	printf("path:%s\r\n", filePath);
	//将任务函数添加到线程池中,等执行任务完毕关闭套接字，并且将文件描述符从fd中移除,并且需要释放传入的数据部分
	FILE* pFile = fopen(filePath, "rb+");
	if (pFile == nullptr)
	{
		delete[] filePath;
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
	std::shared_ptr<char[]> strFileSize (new char[8]);
	//char* strFileSize = new char[8];
	memset(strFileSize.get(),'\0',sizeof(char) * 8);
	memcpy(strFileSize.get(),&temp,sizeof(temp));

	//完全发送出这八个字节
	long long  sendCount = 8;
	long long alReadySend = 0;
	while (alReadySend < sendCount)
	{
		ssize_t size = write(sockClient, strFileSize.get() + alReadySend, sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}
	//delete[] strFileSize;

	//进行读文件，将文件的所有
	std::shared_ptr<char[]> fileData (new char[fileSize]);
	//char* fileData = new char[fileSize];
	memset(fileData.get(),'\0',sizeof(char) * fileSize);
	size_t ret = fread(fileData.get(), 1, fileSize, pFile);
	//fclose(pFile);

	sendCount = fileSize;
	alReadySend =  0;
	while (alReadySend < sendCount)
	{
		ssize_t size = write(sockClient, fileData.get() + alReadySend, sendCount - alReadySend);
		if (size <= 0)
		{
			break;
		}
		alReadySend += size;
	}

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
	delete[] filePath;
	fclose(pFile);
	//pthread_mutex_unlock(&this->m_mutex2);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
}


void CCommand::upLoadHeadPicture(char* pData, int sockClient, int epfd,int dataLength)
{
	std::shared_ptr<Test> test = std::make_shared<Test>();
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
	std::shared_ptr<char[]> pictureData(new char[fileDataLength]);
	//char* pictureData = new char[fileDataLength];
	memcpy(pictureData.get(),p,fileDataLength);
	p += fileDataLength;

	//char* path = new char[pathLength + 1];
	std::shared_ptr<char[]> path(new char[pathLength + 1]);
	memset(path.get(),'\0',sizeof(char)*(pathLength + 1));
	memcpy(path.get(),p,pathLength);

	//进行文件写操作
	FILE* pFile = fopen(path.get(),"wb+");

	if (pFile == nullptr)
	{
		fclose(pFile);
		delete[] pData;
		close(sockClient);
		pthread_mutex_lock(&this->m_mutex);
		epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
		pthread_mutex_unlock(&this->m_mutex);
		printf("file open failed!\r\n");
		return;
	}

	long long alReadyWrite = 0;
	while (true)
	{
		size_t ret =  fwrite(pictureData.get() + alReadyWrite,1, fileDataLength - alReadyWrite,pFile);
		if (ret <= 0)
		{
			break;
		}
		alReadyWrite += ret;
	}
	fclose(pFile);
	delete[] pData;
	close(sockClient);
	pthread_mutex_lock(&this->m_mutex);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sockClient, NULL);
	pthread_mutex_unlock(&this->m_mutex);
}

/*
总结对于大的IO操作，都需要走三步
1、定义好总量
2、循环读
3、循环写
*/
void CCommand::getHeadPicture(char* filePath, int sockClient, int epfd,int dataLength)
{
	std::shared_ptr<Test> test = std::make_shared<Test>();
	printf("path:%s\r\n",filePath);
	//将任务函数添加到线程池中,等执行任务完毕关闭套接字，并且将文件描述符从fd中移除,并且需要释放传入的数据部分
	FILE* pFile = fopen(filePath,"rb+");
	if (pFile == nullptr)
	{
		delete[] filePath;
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

	std::shared_ptr<char[]> data(new char[1024 * 500]);
	//char* data = new char[1024 * 500]; 
	memset(data.get(),'\0',sizeof(char)* 1024 * 500);
	size_t ret =  fread(data.get(),1, fileSize,pFile);
	//this->Dump(data,ret);
	printf("read file size:%d\r\n",ret);
	fclose(pFile);
	//封包

	std::shared_ptr<char[]> packet(new char[2 + 4 + 2 + (1024 * 500) + 1]);
	//char* packet = new char[2 + 4 + 2 + (1024 * 500) + 1];
	char* p = packet.get();
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
	memcpy(p,data.get(),length);
	p += length;

	
	//pthread_mutex_lock(&this->m_mutex2);
	//发送数据包
	long long  sendCount = 2 + 4 + 2 + (1024 * 500) + 1;
	long long alReadySend = 0;
	while (true)
	{
		ssize_t size = write(sockClient, packet.get() + alReadySend,sendCount - alReadySend);
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

typedef struct arg {
	typedef void (CCommand::* FUNC)(char*, int, int, int);
	int cmd;
	char* data;
	int sockClient;
	int epfd;
	int dataLength;
	FUNC function;
	CCommand* thiz;
	arg(int cmd,char* data, int sockClient, int epfd, int dataLength,FUNC function,CCommand* thiz)
	{
		this->cmd = cmd;
		this->data = data;
		this->sockClient = sockClient;
		this->epfd = epfd;
		this->dataLength = dataLength;
		this->function = function;
		this->thiz = thiz;
	}
}Arg;

void* CCommand::task(void* arg)
{
	std::shared_ptr<Arg>* p = (std::shared_ptr<Arg>*)arg;
	std::shared_ptr<Arg> aInfo = *p;
	(aInfo->thiz->*aInfo->function)(aInfo->data,aInfo->sockClient,aInfo->epfd,aInfo->dataLength);
	delete p;
	return nullptr;
}

int CCommand::Excute(int cmd,char* data, int sockClient, int epfd,int dataLength)
{
	auto ret =  this->m_funcMap.find(cmd);
	if (ret != this->m_funcMap.end())
	{
		pthread_mutex_lock(&this->m_mutex_3);
		pthread_t thread;
		std::shared_ptr<Arg> arg = std::make_shared<Arg>(cmd,data,sockClient,epfd,dataLength,ret->second,this);
		std::shared_ptr<Arg>* p = new std::shared_ptr<Arg>(arg);
		pthread_create(&thread,nullptr,&task,p); //在这里进行开启子线程执行任务
		pthread_mutex_unlock(&this->m_mutex_3);
		//将任务放到线程池中
		//this->m_threadPool->addTask([=]() {  (this->*ret->second)(data,sockClient,epfd,dataLength); }); //添加任务是线程安全的
		return 0;
	}
	return -1;
}