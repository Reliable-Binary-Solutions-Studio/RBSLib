#include "rbslib/Windows/BasicUI.h"
#include <functional>
#include <thread>
#include <iostream>
#include "rbslib/TaskPool.h"
#include "rbslib/Math.h"
#include <algorithm>
#include <chrono>
#include "rbslib/Math.h"

class Test
{
public:
	Test()
	{
		std::cout << "构造" << std::endl;
	}
	~Test()
	{
		std::cout << "析构" << std::endl;
	}
	Test(const Test&)
	{
		std::cout << "拷贝构造" << std::endl;
	}
	Test(Test&&)
	{
		std::cout << "移动构造" << std::endl;
	}
};

void TestFunction()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestThreadPoolPerformance(RbsLib::Thread::ThreadPool&pool)
{
	auto start = std::chrono::high_resolution_clock::now();
	std::vector<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
	for (int i = 0; i < 20; ++i)
	{
		results.emplace_back(pool.Run([]()
			{
				TestFunction();
			}));
	}
	for (auto& result : results)
	{
		result.GetResult();
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "使用线程池耗时: " << duration.count() << " 秒" << std::endl;
}

void TestNoThreadPoolPerformance()
{
	auto start = std::chrono::high_resolution_clock::now();
	std::vector<std::thread> threads;
	for (int i = 0; i < 20; ++i)
	{
		threads.emplace_back([]()
			{
				TestFunction();
			});
	}
	for (auto& thread : threads)
	{
		thread.join();
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "不使用线程池耗时: " << duration.count() << " 秒" << std::endl;
}

template <typename T>
void func(T&& x)
{

}
int main()
{
	RbsLib::Thread::ThreadPool pool(std::thread::hardware_concurrency());
	//测试多线程版本函数是否正确
	RbsLib::Math::Matrix<int> a(10, 10);
	RbsLib::Math::Matrix<int> b(10, 10);
	//给随机值
	auto c = a.Apply([](int)->int {return rand(); }, true);

	if (c.EqualMultiThread(c,pool))
	{
		std::cout << "EqualMultiThread函数测试通过" << std::endl;
	}
	else
	{
		std::cout << "EqualMultiThread函数测试失败" << std::endl;
	}
	if (c.EqualMultiThread(c.ApplyMultiThread([](int x)->int {return x; }, pool,true), pool))
	{
		std::cout << "ApplyMultiThread函数测试通过" << std::endl;
	}
	else
	{
		std::cout << "ApplyMultiThread函数测试失败" << std::endl;
	}
	b.ApplyMultiThread([](int x)->int {return rand(); }, pool);
	if (a.HadamardProductMultiThread(b,pool).EqualMultiThread(b.HadamardProduct(a),pool)&&a.EqualMultiThread(b,pool)==false)
	{
		std::cout << "HadamardProductMultiThread函数测试通过" << std::endl;
	}
	else
	{
		std::cout << "HadamardProductMultiThread函数测试失败" << std::endl;
	}
	//测试矩阵加法
	if (RbsLib::Math::AddMultiThread(a, b, pool).EqualMultiThread(a + b, pool))
	{
		std::cout << "AddMultiThread函数测试通过" << std::endl;
	}
	else
	{
		std::cout << "AddMultiThread函数测试失败" << std::endl;
	}
	a = RbsLib::Math::Matrix<int>::LinearSpace(0, 100, 100);
	b = RbsLib::Math::Matrix<int>::LinearSpace(0, 100, 10000).T();
	//测试矩阵乘法
	if (RbsLib::Math::MultiplyMultiThread(a, b, pool).EqualMultiThread(a * b, pool))
	{
		std::cout << "MultiplyMultiThread函数测试通过" << std::endl;
	}
	else
	{
		std::cout << "MultiplyMultiThread函数测试失败" << std::endl;
	}
	//测试矩阵乘法的性能
	
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	RbsLib::Math::MultiplyMultiThread(a, b, pool);
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "矩阵乘法多线程版本耗时: " << duration.count() << " 秒" << std::endl;
	start = std::chrono::high_resolution_clock::now();
	a* b;
	end = std::chrono::high_resolution_clock::now();
	duration = end - start;
	std::cout << "矩阵乘法普通版本耗时: " << duration.count() << " 秒" << std::endl;
}


