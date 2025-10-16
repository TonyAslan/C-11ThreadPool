#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

//c++ 11 线程池demo
class ThreadPool
{
public:
	ThreadPool(int num):is_stop_(false) 
	{
		for (int i = 0; i < num; ++i)
		{
			//就地构造容器元素，避免额外拷贝/移动
			threads_.emplace_back([this]() {
				while (1)
				{
					//比 lock_guard 更灵活的锁封装，支持延迟锁定/手动解锁/条件变量协作。 
					//用于需要临时释放锁或与 condition_variable 一起使用的场景；unique_lock 可移动但不可拷贝。
					std::unique_lock<std::mutex> lock(mtx_);
					//条件变量，线程等待某个条件（通常配合 unique_lock 与一个谓词）。
					//条件为真时往下走， 为假时等待
					condition_var_.wait(lock, [this]() {
						return !tasks_.empty() || is_stop_;
					});
					//线程停止或者队列空 直接返回
					if (is_stop_ && tasks_.empty())
					{
						return;
					}
					//std::function<void()> 类型擦除的通用可调用对象容器，可以持任意签名匹配的可调用对象。 
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
	//模板参数中的 T&& 在模板上下文下可同时绑定左值和右值，用于实现完美转发。std::forward 保留原始值类别。
	template<class F, class... Args>
	void add_task(F &&f, Args&&... args) //在参数里面右值引用是万能引用
	{
		//std::bind 与 std::placeholders 将函数与部分参数绑定，生成新的可调用对象
		std::function<void()> task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx_);
			// std::move(task) 把左值显式转换为右值引用以触发移动构造 / 移动赋值（本身不做移动）。
			tasks_.emplace(std::move(task));
		}
		condition_var_.notify_one();
	}

private:
	//std::thread C++11 原生线程类，用于在程序中启动并管理线程。
	std::vector<std::thread> threads_;  //线程数组
	std::queue<std::function<void()>> tasks_;  //任务队列
	std::mutex mtx_;  //互斥锁，用于保护共享数据以避免并发访问冲突。
	std::condition_variable condition_var_;  //条件变量
	bool is_stop_;  //停止标志
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