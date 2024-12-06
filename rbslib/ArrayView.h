#pragma once
#include <vector>
namespace RbsLib
{
	template<typename T>
	class ArrayView
	{
	private:
		T* m_data;
		size_t m_size;
	public:
		ArrayView(T* data, size_t size) : m_data(data), m_size(size) {}
		ArrayView(std::vector<T>& arr) : m_data(arr.data()), m_size(arr.size()) {}
		std::vector<T> ToVector() const
		{
			return std::vector<T>(m_data, m_data + m_size);
		}

		T& operator[](size_t index)
		{
			return m_data[index];
		}

		const T& operator[](size_t index) const
		{
			return m_data[index];
		}

		size_t size() const
		{
			return m_size;
		}

		T* data()
		{
			return m_data;
		}

		const T* data() const
		{
			return m_data;
		}
		//允许基于范围的for循环
		T* begin()
		{
			return m_data;
		}
		T* end()
		{
			return m_data + m_size;
		}
	};
	template<typename T>
	class ConstArrayView
	{
	private:

		const T* m_data;
		size_t m_size;
	public:
		ConstArrayView(const T* data, size_t size) : m_data(data), m_size(size) {}
		ConstArrayView(const std::vector<T>& arr) : m_data(arr.data()), m_size(arr.size()) {}
		std::vector<T> ToVector() const
		{
			return std::vector<T>(m_data, m_data + m_size);
		}

		const T& operator[](size_t index) const
		{
			return m_data[index];
		}

		size_t size() const
		{
			return m_size;
		}

		const T* data() const
		{
			return m_data;
		}
		//允许基于范围的for循环
		const T* begin() const
		{
			return m_data;
		}
		const T* end() const
		{
			return m_data + m_size;
		}
	};
}