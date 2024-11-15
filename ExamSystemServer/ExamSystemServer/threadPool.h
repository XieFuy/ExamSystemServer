#pragma once
#include<pthread.h>
#include<queue>
#include<vector>
#include<semaphore.h>
#include<functional>

class CThreadPool 
{
public:	
	CThreadPool(int threadSize);
	~CThreadPool();
	CThreadPool(const CThreadPool& threadPool);
	CThreadPool& operator=(const CThreadPool& threadPool);
	void addTask(std::function<void()> taskFunc);
private:
	pthread_mutex_t m_TaskMutex;
	bool m_isStop;
	CThreadPool();
	std::queue<std::function<void()>> m_taskQueue; //任务队列
	pthread_mutex_t m_queueMutex; //线程池互斥访问任务队列的锁
	std::vector<pthread_t> m_workers;//定义工作线程集合
	sem_t m_sem;//用于通知空闲的线程去取任务
	int m_threadSize;
	static void* threadWorkFunc(void* arg);
	void destoryThreadPool();
};