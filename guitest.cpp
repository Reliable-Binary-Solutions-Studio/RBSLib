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
		std::cout << "����" << std::endl;
	}
	~Test()
	{
		std::cout << "����" << std::endl;
	}
	Test(const Test&)
	{
		std::cout << "��������" << std::endl;
	}
	Test(Test&&)
	{
		std::cout << "�ƶ�����" << std::endl;
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
	std::cout << "ʹ���̳߳غ�ʱ: " << duration.count() << " ��" << std::endl;
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
	std::cout << "��ʹ���̳߳غ�ʱ: " << duration.count() << " ��" << std::endl;
}

template <typename T>
void func(T&& x)
{

}
int main()
{
	RbsLib::Thread::ThreadPool pool(std::thread::hardware_concurrency());
	//���Զ��̰߳汾�����Ƿ���ȷ
	RbsLib::Math::Matrix<int> a(10, 10);
	RbsLib::Math::Matrix<int> b(10, 10);
	//�����ֵ
	auto c = a.Apply([](int)->int {return rand(); }, true);

	if (c.EqualMultiThread(c,pool))
	{
		std::cout << "EqualMultiThread��������ͨ��" << std::endl;
	}
	else
	{
		std::cout << "EqualMultiThread��������ʧ��" << std::endl;
	}
	if (c.EqualMultiThread(c.ApplyMultiThread([](int x)->int {return x; }, pool,true), pool))
	{
		std::cout << "ApplyMultiThread��������ͨ��" << std::endl;
	}
	else
	{
		std::cout << "ApplyMultiThread��������ʧ��" << std::endl;
	}
	b.ApplyMultiThread([](int x)->int {return rand(); }, pool);
	if (a.HadamardProductMultiThread(b,pool).EqualMultiThread(b.HadamardProduct(a),pool)&&a.EqualMultiThread(b,pool)==false)
	{
		std::cout << "HadamardProductMultiThread��������ͨ��" << std::endl;
	}
	else
	{
		std::cout << "HadamardProductMultiThread��������ʧ��" << std::endl;
	}
	//���Ծ���ӷ�
	if (RbsLib::Math::AddMultiThread(a, b, pool).EqualMultiThread(a + b, pool))
	{
		std::cout << "AddMultiThread��������ͨ��" << std::endl;
	}
	else
	{
		std::cout << "AddMultiThread��������ʧ��" << std::endl;
	}
	a = RbsLib::Math::Matrix<int>::LinearSpace(0, 100, 100);
	b = RbsLib::Math::Matrix<int>::LinearSpace(0, 100, 10000).T();
	//���Ծ���˷�
	if (RbsLib::Math::MultiplyMultiThread(a, b, pool).EqualMultiThread(a * b, pool))
	{
		std::cout << "MultiplyMultiThread��������ͨ��" << std::endl;
	}
	else
	{
		std::cout << "MultiplyMultiThread��������ʧ��" << std::endl;
	}
	//���Ծ���˷�������
	
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	RbsLib::Math::MultiplyMultiThread(a, b, pool);
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;
	std::cout << "����˷����̰߳汾��ʱ: " << duration.count() << " ��" << std::endl;
	start = std::chrono::high_resolution_clock::now();
	a* b;
	end = std::chrono::high_resolution_clock::now();
	duration = end - start;
	std::cout << "����˷���ͨ�汾��ʱ: " << duration.count() << " ��" << std::endl;
}


