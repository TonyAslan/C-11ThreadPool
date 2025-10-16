#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

//c++ 11 �̳߳�demo
class ThreadPool
{
public:
	ThreadPool(int num):is_stop_(false) 
	{
		for (int i = 0; i < num; ++i)
		{
			//�͵ع�������Ԫ�أ�������⿽��/�ƶ�
			threads_.emplace_back([this]() {
				while (1)
				{
					//�� lock_guard ����������װ��֧���ӳ�����/�ֶ�����/��������Э���� 
					//������Ҫ��ʱ�ͷ������� condition_variable һ��ʹ�õĳ�����unique_lock ���ƶ������ɿ�����
					std::unique_lock<std::mutex> lock(mtx_);
					//�����������̵߳ȴ�ĳ��������ͨ����� unique_lock ��һ��ν�ʣ���
					//����Ϊ��ʱ�����ߣ� Ϊ��ʱ�ȴ�
					condition_var_.wait(lock, [this]() {
						return !tasks_.empty() || is_stop_;
					});
					//�߳�ֹͣ���߶��п� ֱ�ӷ���
					if (is_stop_ && tasks_.empty())
					{
						return;
					}
					//std::function<void()> ���Ͳ�����ͨ�ÿɵ��ö������������Գ�����ǩ��ƥ��Ŀɵ��ö��� 
					std::function<void()> task(std::move(tasks_.front()));
					tasks_.pop();
					lock.unlock();
					task();
				}
			});

		}
	}
	~ThreadPool() 
	{	
		{
			std::unique_lock<std::mutex> lock(mtx_);
			is_stop_ = true;
		}
		condition_var_.notify_all();
		for (auto &t : threads_)
		{
			t.join();
		}
		
	}
	//ģ������е� T&& ��ģ���������¿�ͬʱ����ֵ����ֵ������ʵ������ת����std::forward ����ԭʼֵ���
	template<class F, class... Args>
	void add_task(F &&f, Args&&... args) //�ڲ���������ֵ��������������
	{
		//std::bind �� std::placeholders �������벿�ֲ����󶨣������µĿɵ��ö���
		std::function<void()> task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx_);
			// std::move(task) ����ֵ��ʽת��Ϊ��ֵ�����Դ����ƶ����� / �ƶ���ֵ���������ƶ�����
			tasks_.emplace(std::move(task));
		}
		condition_var_.notify_one();
	}

private:
	//std::thread C++11 ԭ���߳��࣬�����ڳ����������������̡߳�
	std::vector<std::thread> threads_;  //�߳�����
	std::queue<std::function<void()>> tasks_;  //�������
	std::mutex mtx_;  //�����������ڱ������������Ա��Ⲣ�����ʳ�ͻ��
	std::condition_variable condition_var_;  //��������
	bool is_stop_;  //ֹͣ��־
};



int main()
{
	ThreadPool pool(3);
	std::mutex mtx;
	for (int i = 0; i < 10; ++i)
	{
		//std::unique_lock<std::mutex> lock(mtx);
		pool.add_task([i]()
		{
			
			printf(" task : %d running \n", i); 
			std::this_thread::sleep_for(std::chrono::seconds(3));
			printf(" task : %d end \n", i);
		});
	}
	
	return 0;  
}