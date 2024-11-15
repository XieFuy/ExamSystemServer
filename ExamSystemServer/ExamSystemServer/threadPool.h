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
	std::queue<std::function<void()>> m_taskQueue; //�������
	pthread_mutex_t m_queueMutex; //�̳߳ػ������������е���
	std::vector<pthread_t> m_workers;//���幤���̼߳���
	sem_t m_sem;//����֪ͨ���е��߳�ȥȡ����
	int m_threadSize;
	static void* threadWorkFunc(void* arg);
	void destoryThreadPool();
};