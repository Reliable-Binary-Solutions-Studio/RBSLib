#pragma once
#include <queue>
#include <list>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <exception>
#include <utility>
#include <semaphore>


namespace RbsLib::Thread
{
	class SpinLock {
	public:
		SpinLock() = default;
		SpinLock(const SpinLock&) = delete;
		SpinLock& operator=(const SpinLock&) = delete;

		void lock();

		void unlock();

	private:
		std::atomic_flag flag = ATOMIC_FLAG_INIT;
	};

	class TaskPool
	{
	private:
		std::atomic_uint64_t  keep_task_num;
		std::atomic_uint64_t running_task_num = 0, watting_task_num = 0;
		std::queue<std::function<void()>> task_list;
		std::condition_variable getter;
		std::mutex task_lock;
		std::list<std::thread*> threads_handle;
		std::atomic_bool is_cancel = false;
	public:
		TaskPool(int num = 0)noexcept;/*default is no keep thread*/
		TaskPool(const TaskPool&) = delete;
		void Run(std::function<void()> func);
		~TaskPool(void);
	};

	class ThreadPoolException : public std::exception
	{
	private:
		std::string message;
	public:
		ThreadPoolException(const std::string& msg);
		const char* what()const noexcept override;
	};

	class ThreadPool
	{
	private:
		std::atomic_uint64_t  keep_task_num;
		std::atomic_uint64_t running_task_num = 0, watting_task_num = 0;
		std::queue<std::function<void()>> task_list;
		std::condition_variable getter;
		std::mutex task_lock;
		std::list<std::thread*> threads_handle;
		std::atomic_bool is_cancel = false;
	public:
		template<typename T>
		class TaskResult
		{
		private:
            std::shared_ptr<std::tuple<std::binary_semaphore, std::optional<std::shared_ptr<void>>,std::exception_ptr>> handle;
			bool is_result_moved = false;
		public:
			/*要保证安全需要保证构造时句柄中的锁已经locked*/
			TaskResult(const std::shared_ptr<std::tuple<std::binary_semaphore, std::optional<std::shared_ptr<void>>, std::exception_ptr>>& result_handle) : handle(result_handle) {}

			T GetResult()
			{
				if (is_result_moved)
				{
					throw ThreadPoolException("Result has been moved.");
				}
				//检查句柄有效性
				if (!handle)
				{
					throw ThreadPoolException("Result not available yet.");
				}
				//获取锁以检查任务是否完成
				std::get<0>(*handle).acquire();
				//检查任务是否有未处理的异常
				if (std::get<2>(*handle))
				{
					std::rethrow_exception(std::get<2>(*handle));
				}
				if (!std::get<1>(*handle))
				{
					throw ThreadPoolException("result not available.");//不应该出现的错误
				}
				if constexpr (std::is_void_v<T>)
				{
					is_result_moved = true;
					return;
				}
				else
				{
					auto result = std::move(*std::reinterpret_pointer_cast<T>(*std::get<1>(*handle)));
					is_result_moved = true;
					return result;
				}
			}
		};
		/*
		* summary: set the number of threads to keep alive
		* -1: keep all threads alive
		* 0 : keep no thread alive
		*/
		ThreadPool(int num = 0)noexcept;/*default is no keep thread*/
		ThreadPool(const ThreadPool&) = delete;

		template<typename Func, typename ... Arg>
		auto Run(Func&& fn, Arg&& ... arg) -> TaskResult<std::decay_t<decltype(fn(std::forward<Arg>(arg)...))>>
		{
			std::shared_ptr<std::tuple<std::binary_semaphore, std::optional<std::shared_ptr<void>>, std::exception_ptr>> handle = std::make_shared<std::tuple<std::binary_semaphore, std::optional<std::shared_ptr<void>>, std::exception_ptr>>(0, std::nullopt, nullptr);
			std::function<void()> func = [handle, func = std::bind(std::forward<Func>(fn), std::forward<Arg>(arg)...)]() mutable {
				if constexpr (std::is_void_v<decltype(func())>)
				{
					try
					{
						func();
						std::get<1>(*handle).emplace();
					}
					catch (...)
					{
						std::get<2>(*handle) = std::current_exception();
					}
				}
				else
				{
					try
					{
						std::get<1>(*handle).emplace(std::make_shared<std::decay_t<decltype(func())>>(func()));
					}
					catch (...)
					{
						std::get<2>(*handle) = std::current_exception();
					}
				}
				std::get<0>(*handle).release();
				};
			std::unique_lock<std::mutex> lock(this->task_lock);
			this->task_list.push(std::move(func));
			lock.unlock();
			if (this->watting_task_num > 0)
			{
				this->getter.notify_one();
			}
			else
			{
				/*Need to start new thread*/
				auto& mutex = this->task_lock;
				auto& getter = this->getter;
				auto& running = this->running_task_num;
				auto& watting = this->watting_task_num;
				auto& iscancel = this->is_cancel;
				auto& max = this->keep_task_num;
				auto& task_list = this->task_list;
				auto& threads_list = this->threads_handle;

				lock.lock();
				std::thread* t = new std::thread([&max, &mutex, &getter, &running, &watting, &iscancel, &task_list, &threads_list]() {
					std::unique_lock<std::mutex> lock(mutex);
					watting += 1;
					while (iscancel == false)
					{
						getter.wait(lock, [&task_list, &iscancel]() {return (!task_list.empty()) || iscancel; });
						if (iscancel)
						{
							watting -= 1;
							break;
						}
						auto func = std::move(task_list.front());
						task_list.pop();
						watting -= 1;
						running += 1;
						lock.unlock();
						func();
						lock.lock();
						running -= 1;
						watting += 1;
						if (running + watting > max)
						{
							watting -= 1;
							break;
						}
					}
					if (!iscancel)
					{
						std::thread* ptr;
						for (auto it : threads_list)
						{
							if (it->get_id() == std::this_thread::get_id())
							{
								ptr = it;
								break;
							}
						}
						if (ptr->joinable() && !iscancel) ptr->detach();
						threads_list.remove(ptr);
						delete ptr;
					}
					});

				this->threads_handle.push_back(t);
				lock.unlock();
			}
			return TaskResult<std::decay_t<decltype(fn(std::forward<Arg>(arg)...))>>(handle);
		}

		~ThreadPool(void);

	};
}