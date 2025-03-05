#pragma once
#include "BaseType.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <future>
#include <memory>
#include <algorithm>
#include "ArrayView.h"
#include "TaskPool.h"

namespace RbsLib
{
	namespace Math
	{
		template <typename Tp> class Matrix;
		template <typename T>
		Matrix<T> operator+(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (!Matrix<T>::CheckSameSize(a, b))
			{
				throw std::invalid_argument("Matrix size mismatch");
			}
			Matrix<T> result(a.Rows(), a.Cols());
			for (size_t i = 0; i < a.Rows(); i++)
			{
				for (size_t j = 0; j < a.Cols(); j++)
				{
					result[i][j] = a[i][j] + b[i][j];
				}
			}
			return result;
		}
		template<typename T>
		Matrix<T> AddMultiThread(const Matrix<T>& a, const Matrix<T>& b, RbsLib::Thread::ThreadPool& pool)
		{
			if (!Matrix<T>::CheckSameSize(a, b))
			{
				throw std::invalid_argument("Matrix size mismatch");
			}
			Matrix<T> result(a.Rows(), a.Cols());
			int thread_num = std::thread::hardware_concurrency() - 1;
			size_t block_size = a.Cols() * a.Rows() / thread_num;
			std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
			for (int i = 0; i < thread_num; i++)
			{
				results.emplace_back(pool.Run([](const T* A, const T* B, T* C, int thread_index, size_t block_size) {
					size_t start = thread_index * block_size;
					size_t end = (thread_index + 1) * block_size;
					for (size_t i = start; i < end; i++)
					{
						C[i] = A[i] + B[i];
					}
					}, a.Data(), b.Data(), result.Data(), i, block_size));
			}
			size_t start = thread_num * block_size;
			size_t end = a.Cols() * a.Rows();
			for (size_t i = start; i < end; i++)
			{
				result.Data()[i] = a.Data()[i] + b.Data()[i];
			}
			for (auto& result : results)
			{
				result.GetResult();
			}
			return result;
		}
		template <typename T>
		Matrix<T> operator-(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (!Matrix<T>::CheckSameSize(a, b))
			{
				throw std::invalid_argument("Matrix size mismatch");
			}
			Matrix<T> result(a.Rows(), a.Cols());
			for (size_t i = 0; i < a.Rows(); i++)
			{
				for (size_t j = 0; j < a.Cols(); j++)
				{
					result[i][j] = a[i][j] - b[i][j];
				}
			}
			return result;
		}
		template<typename T>
		Matrix<T> SubMultiThread(const Matrix<T>& a, const Matrix<T>& b, RbsLib::Thread::ThreadPool& pool)
		{
			if (!Matrix<T>::CheckSameSize(a, b))
			{
				throw std::invalid_argument("Matrix size mismatch");
			}
			Matrix<T> result(a.Rows(), a.Cols());
			int thread_num = std::thread::hardware_concurrency() - 1;
			size_t block_size = a.Cols() * a.Rows() / thread_num;
			std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
			for (int i = 0; i < thread_num; i++)
			{
				results.emplace_back(pool.Run([](const T* A, const T* B, T* C, int thread_index, size_t block_size) {
					size_t start = thread_index * block_size;
					size_t end = (thread_index + 1) * block_size;
					for (size_t i = start; i < end; i++)
					{
						C[i] = A[i] - B[i];
					}
					}, a.Data(), b.Data(), result.Data(), i, block_size));
			}
			size_t start = thread_num * block_size;
			size_t end = a.Cols() * a.Rows();
			for (size_t i = start; i < end; i++)
			{
				result.Data()[i] = a.Data()[i] - b.Data()[i];
			}
			for (auto& result : results)
			{
				result.GetResult();
			}
			return result;
		}
		template <typename T>
		Matrix<T> operator*(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (a.Cols() != b.Rows())
			{
				throw std::invalid_argument("Matrix size mismatch");
			}

			Matrix<T> result(a.Rows(), b.Cols());

			// 设置块的大小
			size_t block_size = 64;
			for (size_t i = 0; i < a.Rows(); i += block_size)
			{
				for (size_t j = 0; j < b.Cols(); j += block_size)
				{
					for (size_t k = 0; k < a.Cols(); k += block_size)
					{
						// 处理每个小块的乘法
                        for (size_t ii = i; ii < (std::min)(i + block_size, a.Rows()); ++ii)
						{
							for (size_t jj = j; jj < (std::min)(j + block_size, b.Cols()); ++jj)
							{
								for (size_t kk = k; kk < (std::min)(k + block_size, a.Cols()); ++kk)
								{
									result[ii][jj] += a[ii][kk] * b[kk][jj];
								}
							}
						}
					}
				}
			}

			return result;
		}
		template<typename T>
		Matrix<T> MultiplyMultiThread(const Matrix<T>& a, const Matrix<T>& b, RbsLib::Thread::ThreadPool& pool)
		{
			if (a.Cols() != b.Rows())
			{
				throw std::invalid_argument("Matrix size mismatch");
			}
			Matrix<T> result(a.Rows(), b.Cols());
			// 设置块的大小
			int thread_num = std::thread::hardware_concurrency() - 1;

			size_t block_size = (std::max<size_t>)(1, (std::min<size_t>)(64, a.Rows() / thread_num));
			std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
			for (int t = 0; t < thread_num; t++)
			{
				results.emplace_back(pool.Run([&a, &b, &result, block_size, t, thread_num]() {
					for (size_t i = t * block_size; i < a.Rows(); i += block_size * thread_num)
					{
						for (size_t j = 0; j < b.Cols(); j += block_size)
						{
							for (size_t k = 0; k < a.Cols(); k += block_size)
							{
								// 处理每个小块的乘法
								for (size_t ii = i; ii < (std::min)(i + block_size, a.Rows()); ++ii)
								{
									for (size_t jj = j; jj < (std::min)(j + block_size, b.Cols()); ++jj)
									{
										for (size_t kk = k; kk < (std::min)(k + block_size, a.Cols()); ++kk)
										{
											result(ii, jj) += a(ii, kk) * b(kk, jj);
											//result[ii][jj] += a[ii][kk] * b[kk][jj];
										}
									}
								}
							}
						}
					}
					}));
			}
			for (auto& result : results)
			{
				result.GetResult();
			}
			return result;
		}
		template <typename T>
		Matrix<T> operator*(const Matrix<T>& a, T b)
		{
			Matrix<T> result(a.Rows(), a.Cols());
			for (size_t i = 0; i < a.Rows(); i++)
			{
				for (size_t j = 0; j < a.Cols(); j++)
				{
					result[i][j] = a[i][j] * b;
				}
			}
			return result;
		}
		template <typename T>
		Matrix<T> operator*(T a, const Matrix<T>& b)
		{
			return b * a;
		}
		template <typename T>
		Matrix<T> MultiplyMultiThread(T a, const Matrix<T>& b, RbsLib::Thread::ThreadPool& pool)
		{
			Matrix<T> result(b.Rows(), b.Cols());
			int thread_num = std::thread::hardware_concurrency() - 1;
			size_t block_size = b.Cols() * b.Rows() / thread_num;
			std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
			for (int i = 0; i < thread_num; i++)
			{
				results.emplace_back(pool.Run([](const T* B, T* C, T a, int thread_index, size_t block_size) {
					size_t start = thread_index * block_size;
					size_t end = (thread_index + 1) * block_size;
					for (size_t i = start; i < end; i++)
					{
						C[i] = a * B[i];
					}
					}, b.Data(), result.Data(), a, i, block_size));
			}
			size_t start = thread_num * block_size;
			size_t end = b.Cols() * b.Rows();
			for (size_t i = start; i < end; i++)
			{
				result.Data()[i] = a * b.Data()[i];
			}
			for (auto& result : results)
			{
				result.GetResult();
			}
			return result;
		}
		template <typename Tp>
		Matrix<Tp> T(const Matrix<Tp>& a)
		{
			Matrix<Tp> result(a.Cols(), a.Rows());
			for (size_t i = 0; i < a.Rows(); i++)
			{
				for (size_t j = 0; j < a.Cols(); j++)
				{
					result[j][i] = a[i][j];
				}
			}
			return result;
		}

		template <typename Tp>
		class Matrix
		{
		private:
			std::unique_ptr<Tp[]> data;
			size_t rows;
			size_t cols;
		public:
			Matrix()
				: rows(0), cols(0)
			{
			}

			Matrix(size_t rows, size_t cols, Tp value = 0)
				: data(std::make_unique<Tp[]>(rows* cols)), rows(rows), cols(cols)
			{
			}
			Matrix(const Matrix<Tp>& other)
				: data(std::make_unique<Tp[]>(other.rows* other.cols)), rows(other.rows), cols(other.cols)
			{
				std::memcpy(data.get(), other.data.get(), sizeof(Tp) * rows * cols);
			}
			Matrix(const std::vector<std::vector<Tp>>& data)
				: data(std::make_unique<Tp[]>(data.size()* data[0].size())), rows(data.size()), cols(data[0].size())
			{
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						this->data[i * cols + j] = data[i][j];
					}
				}
			}
			Matrix(Matrix<Tp>&& other) noexcept
				: data(std::move(other.data)), rows(other.rows), cols(other.cols)
			{
				other.rows = 0;
				other.cols = 0;
			}
			Matrix<Tp>& operator=(const Matrix<Tp>& other)
			{
				if (this == &other)
				{
					return *this;
				}
				//考虑同形矩阵优化取消重新分配内存
				if (rows == other.rows || cols == other.cols)
				{
					//直接调用memcpy
					std::memcpy(data.get(), other.data.get(), sizeof(Tp) * rows * cols);
				}
				data = std::make_unique<Tp[]>(other.rows * other.cols);
				rows = other.rows;
				cols = other.cols;
				//调用memcpy
				std::memcpy(data.get(), other.data.get(), sizeof(Tp) * rows * cols);
				return *this;
			}
			Matrix<Tp>& operator=(Matrix<Tp>&& other) noexcept
			{
				if (this == &other)
				{
					return *this;
				}
				data = std::move(other.data);
				rows = other.rows;
				cols = other.cols;
				other.rows = 0;
				other.cols = 0;
				return *this;
			}
			Tp* Data()
			{
				return data.get();
			}
			Tp& operator()(size_t i, size_t j)
			{
				return data[i * cols + j];
			}
			const Tp& operator()(size_t i, size_t j) const
			{
				return data[i * cols + j];
			}
			const Tp* Data() const
			{
				return data.get();
			}
			static Matrix<Tp> LinearSpace(Tp start, Tp end, size_t num)
			{
				Matrix<Tp> result(num, 1);
				Tp step = (end - start) / (num - 1);
				for (size_t i = 0; i < num; i++)
				{
					result[i][0] = start + i * step;
				}
				return result;
			}
			void Column(int index, const std::vector<Tp>& column)
			{
				if (index >= cols)
				{
					throw std::out_of_range("Index out of range");
				}
				if (column.size() != rows)
				{
					throw std::invalid_argument("Column size mismatch");
				}
				for (size_t i = 0; i < rows; i++)
				{
					data[i * this->cols + index] = column[i];
				}
			}
			std::vector<Tp> Column(int index)
			{
				if (index >= cols)
				{
					throw std::out_of_range("Index out of range");
				}
				std::vector<Tp> column(rows);
				for (size_t i = 0; i < rows; i++)
				{
					column[i] = data[i * this->cols + index];
				}
				return column;
			}


			RbsLib::ArrayView<Tp> Row(int index)
			{
				if (index >= rows)
				{
					throw std::out_of_range("Index out of range");
				}
				return RbsLib::ArrayView<Tp>(data.get() + index * this->cols, cols);
			}
			RbsLib::ConstArrayView<Tp> Row(int index) const
			{
				if (index >= rows)
				{
					throw std::out_of_range("Index out of range");
				}
				return RbsLib::ConstArrayView<Tp>(data.get() + index * this->cols, cols);
			}

			void Row(int index, const std::vector<Tp>& row)
			{
				if (index >= this->rows)
				{
					throw std::out_of_range("Index out of range");
				}
				if (row.size() != cols)
				{
					throw std::invalid_argument("Row size mismatch");
				}
				for (size_t i = 0; i < cols; i++)
				{
					data[index * cols + i] = row[i];
				}
			}
			RbsLib::ArrayView<Tp> operator[](size_t index)
			{
				return Row(index);
			}
			RbsLib::ConstArrayView<Tp> operator[](size_t index) const
			{
				return Row(index);
			}
			size_t Rows() const
			{
				return rows;
			}
			size_t Cols() const
			{
				return cols;
			}
			static bool CheckSameSize(const Matrix<Tp>& a, const Matrix<Tp>& b)
			{
				return a.Rows() == b.Rows() && a.Cols() == b.Cols();
			}
			std::string ToString() const
			{
				std::string result;
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						result += std::to_string((*this)[i][j]);
						if (j < cols - 1)
						{
							result += " ";
						}
					}
					if (i < rows - 1)
					{
						result += "\n";
					}
				}
				return result;
			}
			Matrix<Tp> T() const
			{
				Matrix<Tp> result(cols, rows);
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						result[j][i] = (*this)[i][j];
					}
				}
				return result;
			}

			/*
			* @brief 原地为矩阵每个元素应用函数
			* @param func 函数
			*/
			void Apply(std::function<Tp(Tp)> func)
			{
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						(*this)[i][j] = func((*this)[i][j]);
					}
				}
			}
			/*
			* @brief 使用多线程为矩阵每个元素应用函数
			* 需要注意func的线程安全性
			* @param pool 线程池对象
			*/
			void ApplyMultiThread(std::function<Tp(Tp)> func, RbsLib::Thread::ThreadPool& pool)
			{
				int thread_num = std::thread::hardware_concurrency() - 1;
				size_t block_size = this->cols * this->rows / thread_num;
				std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
				for (int i = 0; i < thread_num; i++)
				{
					results.emplace_back(pool.Run([&func](decltype(this) mat, int thread_index, size_t block_size) {
						size_t start = thread_index * block_size;
						size_t end = (thread_index + 1) * block_size;
						for (size_t i = start; i < end; i++)
						{
							mat->data[i] = func(mat->data[i]);
						}
						}, this, i, block_size));
				}
				size_t start = thread_num * block_size;
				size_t end = this->cols * this->rows;
				for (size_t i = start; i < end; i++)
				{
					this->data[i] = func(this->data[i]);
				}
				for (auto& result : results)
				{
					result.GetResult();
				}
			}

			/*
			* @brief 为矩阵每个元素应用函数得到新矩阵
			* @param func 函数
			* @param copy 复制矩阵，不论填什么都会复制矩阵，若不希望复制矩阵，使用Apply的其他重载
			*/
			Matrix<Tp> Apply(std::function<Tp(Tp)> func, bool copy)
			{
				Matrix<Tp> result(rows, cols);
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						result[i][j] = func((*this)[i][j]);
					}
				}
				return result;
			}
			/*
			* @brief 为矩阵每个元素应用函数得到新矩阵多线程版本
			* @param func 函数
			* @param copy 复制矩阵，不论填什么都会复制矩阵，若不希望复制矩阵，使用Apply的其他重载
			*/
			Matrix<Tp> ApplyMultiThread(std::function<Tp(Tp)> func, RbsLib::Thread::ThreadPool& pool, bool copy)
			{
				Matrix<Tp> result(rows, cols);
				int thread_num = std::thread::hardware_concurrency() - 1;
				size_t block_size = this->cols * this->rows / thread_num;
				std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
				for (int i = 0; i < thread_num; i++)
				{
					results.emplace_back(pool.Run([&func](decltype(this) mat, decltype(this) res, int thread_index, size_t block_size) {
						size_t start = thread_index * block_size;
						size_t end = (thread_index + 1) * block_size;
						for (size_t i = start; i < end; i++)
						{
							res->data[i] = func(mat->data[i]);
						}
						}, this, &result, i, block_size));
				}
				size_t start = thread_num * block_size;
				size_t end = this->cols * this->rows;
				for (size_t i = start; i < end; i++)
				{
					result.data[i] = func(this->data[i]);
				}
				for (auto& result : results)
				{
					result.GetResult();
				}
				return result;
			}
			/*
			* @brief 矩阵对应元素相乘
			*/
			Matrix<Tp> HadamardProduct(const Matrix<Tp>& other) const
			{
				if (!CheckSameSize(*this, other))
				{
					throw std::invalid_argument("Matrix size mismatch");
				}
				Matrix<Tp> result(rows, cols);
				for (int i = 0; i < rows * cols; i++)
				{
					result.data[i] = data[i] * other.data[i];
				}
				return result;
			}

			/*
			* @brief 矩阵对应元素相乘多线程版本
			*/
			Matrix<Tp> HadamardProductMultiThread(const Matrix<Tp>& other, RbsLib::Thread::ThreadPool& pool) const
			{
				Matrix<Tp> result(rows, cols);
				int thread_num = std::thread::hardware_concurrency() - 1;
				size_t block_size = this->cols * this->rows / thread_num;
				std::list<RbsLib::Thread::ThreadPool::TaskResult<void>> results;
				for (int i = 0; i < thread_num; i++)
				{
					results.emplace_back(pool.Run([](const Tp* A, const Tp* B, Tp* C, int thread_index, size_t block_size) {
						size_t start = thread_index * block_size;
						size_t end = (thread_index + 1) * block_size;
						for (size_t i = start; i < end; i++)
						{
							C[i] = A[i] * B[i];
						}
						}, this->data.get(), other.data.get(), result.data.get(), i, block_size));
				}
				size_t start = thread_num * block_size;
				size_t end = this->cols * this->rows;
				for (size_t i = start; i < end; i++)
				{
					result.data[i] = this->data[i] * other.data[i];
				}
				for (auto& result : results)
				{
					result.GetResult();
				}
				return result;
			}

			/*
			* @brief 矩阵相等判断多线程版本 注意本函数返回不保证线程池中的任务已经完成
			*/
			bool EqualMultiThread(const Matrix<Tp>& other, RbsLib::Thread::ThreadPool& pool) const
			{
				if (!CheckSameSize(*this, other))
				{
					throw std::invalid_argument("Matrix size mismatch");
				}
				int thread_num = std::thread::hardware_concurrency() - 1;
				size_t block_size = this->cols * this->rows / thread_num;
				std::list<RbsLib::Thread::ThreadPool::TaskResult<bool>> results;
				for (int i = 0; i < thread_num; i++)
				{
					results.emplace_back(pool.Run([](const Tp* A, const Tp* B, int thread_index, size_t block_size) {
						size_t start = thread_index * block_size;
						size_t end = (thread_index + 1) * block_size;
						for (size_t i = start; i < end; i++)
						{
							if (A[i] != B[i])
							{
								return false;
							}
						}
						return true;
						}, this->data.get(), other.data.get(), i, block_size));
				}
				size_t start = thread_num * block_size;
				size_t end = this->cols * this->rows;
				for (size_t i = start; i < end; i++)
				{
					if (this->data[i] != other.data[i])
					{
						return false;
					}
				}
				for (auto& result : results)
				{
					if (!result.GetResult())
					{
						return false;
					}
				}
				return true;
			}

			Matrix<Tp> AddMultiThread(const Matrix<Tp>& other, RbsLib::Thread::ThreadPool& pool) const
			{
				return RbsLib::Math::AddMultiThread(*this, other, pool);
			}
			Matrix<Tp> SubMultiThread(const Matrix<Tp>& other, RbsLib::Thread::ThreadPool& pool) const
			{
				return RbsLib::Math::SubMultiThread(*this, other, pool);
			}
			Matrix<Tp> MultiplyMultiThread(const Matrix<Tp>& other, RbsLib::Thread::ThreadPool& pool) const
			{
				return RbsLib::Math::MultiplyMultiThread(*this, other, pool);
			}
			Matrix<Tp> MultiplyMultiThread(Tp other, RbsLib::Thread::ThreadPool& pool) const
			{
				return RbsLib::Math::MultiplyMultiThread(other, *this, pool);
			}
		};
		
		float sigmoid(float x);
		float sigmoid_derivative(float x);
		float LeakyReLU(float x,float alpha = 0.01);
		float LeakyReLU_derivative(float x, float alpha = 0.01);
		float Tanh(float x);
		float Tanh_derivative(float x);
	}
}
