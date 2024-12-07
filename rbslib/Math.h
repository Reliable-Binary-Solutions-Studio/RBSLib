#pragma once
#include "BaseType.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <future>
#include <memory>
#include "ArrayView.h"

namespace RbsLib
{
	namespace Math
	{
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
				data = std::make_unique<Tp[]>(other.rows * other.cols);
				rows = other.rows;
				cols = other.cols;
				for (size_t i = 0; i < rows; i++)
				{
					for (size_t j = 0; j < cols; j++)
					{
						data[i * cols + j] = other.data[i * cols + j];
					}
				}
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
					data[i*this->cols+index] = column[i];
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
				return RbsLib::ArrayView<Tp>(data.get()+index*this->cols, cols);
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

		};
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
		template <typename T>
		Matrix<T> operator*(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (a.Cols() != b.Rows())
			{
				throw std::invalid_argument("Matrix size mismatch");
			}

			Matrix<T> result(a.Rows(), b.Cols());

			// 设置块的大小
			size_t block_size = 64;  // 可以根据实际情况调整

			for (size_t i = 0; i < a.Rows(); i += block_size)
			{
				for (size_t j = 0; j < b.Cols(); j += block_size)
				{
					for (size_t k = 0; k < a.Cols(); k += block_size)
					{
						// 处理每个小块的乘法
						for (size_t ii = i; ii < std::min(i + block_size, a.Rows()); ++ii)
						{
							for (size_t jj = j; jj < std::min(j + block_size, b.Cols()); ++jj)
							{
								for (size_t kk = k; kk < std::min(k + block_size, a.Cols()); ++kk)
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
		template <typename T>
		Matrix<T> AddMatrixThreaded(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (!Matrix<T>::CheckSameSize(a, b))
			{
				throw std::invalid_argument("Matrix size mismatch");
			}

			Matrix<T> result(a.Rows(), a.Cols());

			// 定义线程数为硬件支持的并发线程数
			unsigned int num_threads = std::thread::hardware_concurrency();
			unsigned int rows_per_thread = a.Rows() / num_threads;
			unsigned int remaining_rows = a.Rows() % num_threads;  // 处理剩余行
			std::vector<std::future<void>> futures;

			// 创建线程并分配每个线程负责的行
			for (unsigned int t = 0; t < num_threads; ++t)
			{
				futures.push_back(std::async(std::launch::async, [&, t]() {
					unsigned int start_row = t * rows_per_thread;
					unsigned int end_row = (t == num_threads - 1) ? (t + 1) * rows_per_thread + remaining_rows : (t + 1) * rows_per_thread;

					// 每个线程处理指定范围内的行
					for (unsigned int i = start_row; i < end_row; ++i)
					{
						for (unsigned int j = 0; j < a.Cols(); ++j)
						{
							result[i][j] = a[i][j] + b[i][j];
						}
					}
					}));
			}

			// 等待所有线程完成
			for (auto& future : futures)
			{
				future.get();
			}

			return result;
		}

		template <typename T>
		Matrix<T> MulMatrixThreaded(const Matrix<T>& a, const Matrix<T>& b)
		{
			if (a.Cols() != b.Rows())
			{
				throw std::invalid_argument("Matrix size mismatch");
			}

			Matrix<T> result(a.Rows(), b.Cols());

			// 设置块的大小
			size_t block_size = 64;  // 可以根据实际情况调整

			// 获取并发线程数
			unsigned int num_threads = std::thread::hardware_concurrency();
			unsigned int rows_per_thread = a.Rows() / num_threads;
			unsigned int remaining_rows = a.Rows() % num_threads;

			std::vector<std::future<void>> futures;

			// 并行计算每个小块的乘法
			for (unsigned int t = 0; t < num_threads; ++t)
			{
				futures.push_back(std::async(std::launch::async, [&, t]() {
					unsigned int start_row = t * rows_per_thread;
					unsigned int end_row = (t == num_threads - 1) ? (t + 1) * rows_per_thread + remaining_rows : (t + 1) * rows_per_thread;

					for (size_t i = start_row; i < end_row; i += block_size)
					{
						for (size_t j = 0; j < b.Cols(); j += block_size)
						{
							for (size_t k = 0; k < a.Cols(); k += block_size)
							{
								for (size_t ii = i; ii < std::min(i + block_size, a.Rows()); ++ii)
								{
									for (size_t jj = j; jj < std::min(j + block_size, b.Cols()); ++jj)
									{
										for (size_t kk = k; kk < std::min(k + block_size, a.Cols()); ++kk)
										{
											result[ii][jj] += a[ii][kk] * b[kk][jj];
										}
									}
								}
							}
						}
					}
					}));
			}

			// 等待所有线程完成
			for (auto& future : futures)
			{
				future.get();
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

		
		float sigmoid(float x);
		float sigmoid_derivative(float x);
		float LeakyReLU(float x,float alpha = 0.01);
		float LeakyReLU_derivative(float x, float alpha = 0.01);
		float Tanh(float x);
		float Tanh_derivative(float x);
	}
}
