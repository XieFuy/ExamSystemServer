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

void* CThreadPool::threadWorkFunc(void* arg) //每一个线程都要工作的内容
{
	CThreadPool* thiz = (CThreadPool*)arg;
	while (true)
	{
		//进行阻塞获取到对任务队列的访问权
		pthread_mutex_lock(&thiz->m_queueMutex);
		//等得到队列的通知后，再进行拿取任务
		sem_wait(&thiz->m_sem);

		if (thiz->m_isStop && thiz->m_taskQueue.empty()) //如果线程池要关闭，并且所有任务执行完毕，则结束线程
		{
			pthread_mutex_unlock(&thiz->m_queueMutex);
			pthread_detach(pthread_self());
			return nullptr;
			//pthread_exit(NULL);
		}
		//从任务队列中拿取任务
		std::function<void()> task = std::move(thiz->m_taskQueue.front()); //作为右值的的对象尽量转为右值引用
		thiz->m_taskQueue.pop();
		pthread_mutex_unlock(&thiz->m_queueMutex);
		task(); //执行线程函数
	}
}

CThreadPool::CThreadPool(const CThreadPool& threadPool) {}
CThreadPool& CThreadPool::operator=(const CThreadPool& threadPool) {}

void  CThreadPool::destoryThreadPool()
{
	this->m_isStop = true;
	//等待线程池中的工作线程都执行任务完毕，并且关闭所有的线程
	// 发送信号给所有等待的线程，使它们检查退出条件  
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
	//进行通知空闲的线程来进行拿取任务，更改信号量的值
	sem_post(&this->m_sem);
	pthread_mutex_unlock(&this->m_TaskMutex);
}



