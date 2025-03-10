#include"threadPool.h"

CThreadPool::CThreadPool()
{
	 pthread_mutex_init(&this->m_queueMutex,nullptr);
	 sem_init(&this->m_sem,0,0);
	 this->m_threadSize = 0;
}

CThreadPool::CThreadPool(int threadSize)
{
	this->m_isStop = false;
	this->m_threadSize = threadSize;
	pthread_mutex_init(&this->m_queueMutex, nullptr);
	pthread_mutex_init(&this->m_TaskMutex,nullptr);
	sem_init(&this->m_sem, 0, 0);
	this->m_workers.resize(this->m_threadSize);
	for (int i = 0 ; i < this->m_workers.size() ; i++)
	{
		pthread_create(&this->m_workers.at(i),nullptr,&CThreadPool::threadWorkFunc,this);
	}
}

CThreadPool::~CThreadPool()
{
	this->destoryThreadPool();
}

void* CThreadPool::threadWorkFunc(void* arg) //ÿһ���̶߳�Ҫ����������
{
	CThreadPool* thiz = (CThreadPool*)arg;
	while (true)
	{
		//����������ȡ����������еķ���Ȩ
		pthread_mutex_lock(&thiz->m_queueMutex);
		//�ȵõ����е�֪ͨ���ٽ�����ȡ����
		sem_wait(&thiz->m_sem);

		if (thiz->m_isStop && thiz->m_taskQueue.empty()) //����̳߳�Ҫ�رգ�������������ִ����ϣ�������߳�
		{
			pthread_mutex_unlock(&thiz->m_queueMutex);
			pthread_detach(pthread_self());
			return nullptr;
			//pthread_exit(NULL);
		}
		//�������������ȡ����
		std::function<void()> task = std::move(thiz->m_taskQueue.front()); //��Ϊ��ֵ�ĵĶ�����תΪ��ֵ����
		thiz->m_taskQueue.pop();
		pthread_mutex_unlock(&thiz->m_queueMutex);
		task(); //ִ���̺߳���
	}
}

CThreadPool::CThreadPool(const CThreadPool& threadPool) {}
CThreadPool& CThreadPool::operator=(const CThreadPool& threadPool) {}

void  CThreadPool::destoryThreadPool()
{
	this->m_isStop = true;
	//�ȴ��̳߳��еĹ����̶߳�ִ��������ϣ����ҹر����е��߳�
	// �����źŸ����еȴ����̣߳�ʹ���Ǽ���˳�����  
	for (int i = 0; i < this->m_workers.size(); ++i) {
		sem_post(&this->m_sem);
		//pthread_join(this->m_workers.at(i),NULL);
	}
	
	while (this->m_taskQueue.size() > 0)
	{
		this->m_taskQueue.pop();
	}
	pthread_mutex_destroy(&this->m_queueMutex);
	sem_destroy(&this->m_sem);
}

void CThreadPool::addTask(std::function<void()> taskFunc)
{
	if (taskFunc == nullptr)
	{
		return;
	}
	pthread_mutex_lock(&this->m_TaskMutex);
	this->m_taskQueue.push(taskFunc);
	//����֪ͨ���е��߳���������ȡ���񣬸����ź�����ֵ
	sem_post(&this->m_sem);
	pthread_mutex_unlock(&this->m_TaskMutex);
}



