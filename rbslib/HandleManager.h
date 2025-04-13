#pragma once
#include <type_traits>
#include <functional>
namespace RbsLib
{
	//句柄管理器，用于管理系统句柄或类似资源的生命周期
	//T必须是标准内存布局的类型
	template <typename T,typename=std::enable_if_t<std::is_standard_layout_v<T>>>
	class UniqueHandle
	{
		bool m_isValid = false; //是否有效
		T m_handle;
		std::function<void(T)> m_releaseHandle; //释放句柄的函数
	public:
		UniqueHandle(const T& handle, std::function<void(T)> release_handle) : m_handle(handle),m_isValid(true),m_releaseHandle(release_handle) {}
		~UniqueHandle()
		{
			if (m_isValid)
			{
				//释放句柄
				this->Release();
			}
		}
		//禁止拷贝构造和赋值	
		UniqueHandle(const UniqueHandle&) = delete;
		UniqueHandle& operator=(const UniqueHandle&) = delete;
		//允许移动构造和赋值
		UniqueHandle(UniqueHandle&& other) noexcept : m_handle(other.m_handle), m_isValid(other.m_isValid), m_releaseHandle(std::move(other.m_releaseHandle))
		{
			other.m_isValid = false;
		}
		UniqueHandle& operator=(UniqueHandle&& other) noexcept
		{
			if (this != &other)
			{
				if (m_isValid)
				{
					this->Release();
				}
				m_handle = other.m_handle;
				m_isValid = other.m_isValid;
				m_releaseHandle = other.m_releaseHandle;
				other.m_isValid = false;
			}
			return *this;
		}
		const T& GetHandle() const
		{
			return m_handle;
		}
		bool IsValid() const
		{
			return m_isValid;
		}
		void Release()
		{
			if (m_isValid)
			{
				m_releaseHandle(m_handle);
				m_isValid = false;
			}
		}
		T Detach()
		{
			m_isValid = false;
			return m_handle;
		}
	};
}