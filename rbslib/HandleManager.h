#pragma once
#include <type_traits>
#include <functional>
namespace RbsLib
{
	//��������������ڹ���ϵͳ�����������Դ����������
	//T�����Ǳ�׼�ڴ沼�ֵ�����
	template <typename T,typename=std::enable_if_t<std::is_standard_layout_v<T>>>
	class UniqueHandle
	{
		bool m_isValid = false; //�Ƿ���Ч
		T m_handle;
		std::function<void(T)> m_releaseHandle; //�ͷž���ĺ���
	public:
		UniqueHandle(const T& handle, std::function<void(T)> release_handle) : m_handle(handle),m_isValid(true),m_releaseHandle(release_handle) {}
		~UniqueHandle()
		{
			if (m_isValid)
			{
				//�ͷž��
				this->Release();
			}
		}
		//��ֹ��������͸�ֵ	
		UniqueHandle(const UniqueHandle&) = delete;
		UniqueHandle& operator=(const UniqueHandle&) = delete;
		//�����ƶ�����͸�ֵ
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