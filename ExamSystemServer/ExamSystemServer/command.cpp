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
	    {1,&CCommand::upLoadHeadPicture},
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

void CCommand::upLoadHeadPicture(char* pData, int sockClient, int epfd,int dataLength)
{
	if (pData == nullptr)
	{
		return;
	}

	//�Դ�����������ݽ��н���
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

	//�����ļ�д����
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
}

/*
�ܽ���ڴ��IO����������Ҫ������
1�����������
2��ѭ����
3��ѭ��д
*/
void CCommand::getHeadPicture(char* filePath, int sockClient, int epfd,int dataLength)
{
	//����������ӵ��̳߳���,��ִ��������Ϲر��׽��֣����ҽ��ļ���������fd���Ƴ�,������Ҫ�ͷŴ�������ݲ���
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
	//���
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

	//�������ݰ�
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

int CCommand::Excute(int cmd,char* data, int sockClient, int epfd,int dataLength)
{
	auto ret =  this->m_funcMap.find(cmd);
	if (ret != this->m_funcMap.end())
	{
		//������ŵ��̳߳���
		this->m_threadPool->addTask([=]() {  (this->*ret->second)(data,sockClient,epfd,dataLength); }); //����������̰߳�ȫ��
		return 0;
	}
	return -1;
}