#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

class ThreadPool
{
public:
	ThreadPool(int num):is_stop_(false) 
	{
		for (int i = 0; i < num; ++i)
		{
			threads_.emplace_back([this]() {
				while (1)
				{
					std::unique_lock<std::mutex> lock(mtx_);
					condition_var_.wait(lock, [this]() {
						return !tasks_.empty() || is_stop_;
					});
					if (is_stop_)
					{
						return;
					}
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
	template<class F, class... Args>
	void add_task(F &&f, Args&&... args) //在参数里面右值引用是万能引用
	{
		std::function<void()> task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx_);
			tasks_.emplace(std::move(task));
		}
		condition_var_.notify_one();
	}

private:
	std::vector<std::thread> threads_;
	std::queue<std::function<void()>> tasks_;
	std::mutex mtx_;
	std::condition_variable condition_var_;
	bool is_stop_;
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
			
			std::cout << " task :" << i  << " running "<< std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << " task :" << i << " end " << std::endl;
		});
	}
	
	return 0;  
}