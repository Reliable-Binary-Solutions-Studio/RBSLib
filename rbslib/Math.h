#pragma once
#include "BaseType.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <thread>
#include <future>

namespace RbsLib
{
	namespace Math
	{
		template <typename Tp>
		class Matrix
		{
		private:
			std::vector<std::vector<Tp>> data;
			size_t rows;
			size_t cols;
		public:
			Matrix() : rows(0), cols(0)
			{
			}
			Matrix(size_t rows, size_t cols, Tp value = 0)
				: data(rows, std::vector<Tp>(cols, value)), rows(rows), cols(cols)
			{
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
				if (column.size() != rows)
				{
					throw std::invalid_argument("Column size mismatch");
				}
				for (size_t i = 0; i < rows; i++)
				{
					data[i][index] = column[i];
				}
			}
			std::vector<Tp> Column(int index)
			{
				std::vector<Tp> column(rows);
				for (size_t i = 0; i < rows; i++)
				{
					column[i] = data[i][index];
				}
				return column;
			}

			const std::vector<Tp>& Row(int index) const
			{
				return data[index];
			}

			void Row(int index, const std::vector<Tp>& row)
			{
				if (row.size() != cols)
				{
					throw std::invalid_argument("Row size mismatch");
				}
				data[index] = row;
			}
			std::vector<Tp>& operator[](size_t index)
			{
				return data[index];
			}
			const std::vector<Tp>& operator[](size_t index) const
			{
				return data[index];
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
						result += std::to_string(data[i][j]);
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
						result[j][i] = data[i][j];
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

		template <typename T>
		class Normalization
		{
		private:
			struct MaxMinPack
			{
				double max;
				double min;
			};
			std::vector<MaxMinPack> MaxMin;
		public:
			void Fit(const Matrix<T>& data)
			{
				MaxMin.resize(data.Cols());
				for (size_t i = 0; i < data.Cols(); i++)
				{
					MaxMin[i].max = data[0][i];
					MaxMin[i].min = data[0][i];
					for (size_t j = 1; j < data.Rows(); j++)
					{
						if (data[j][i] > MaxMin[i].max)
						{
							MaxMin[i].max = data[j][i];
						}
						if (data[j][i] < MaxMin[i].min)
						{
							MaxMin[i].min = data[j][i];
						}
					}
				}
			}
			Matrix<T> Normalize(const Matrix<T>& data)
			{
				Matrix<T> result(data.Rows(), data.Cols());
				for (size_t i = 0; i < data.Cols(); i++)
				{
					for (size_t j = 0; j < data.Rows(); j++)
					{
						result[j][i] = (data[j][i] - MaxMin[i].min) / (MaxMin[i].max - MaxMin[i].min);
					}
				}
				return result;
			}
			Matrix<T> Denormalize(const Matrix<T>& data)
			{
				Matrix<T> result(data.Rows(), data.Cols());
				for (size_t i = 0; i < data.Cols(); i++)
				{
					for (size_t j = 0; j < data.Rows(); j++)
					{
						result[j][i] = data[j][i] * (MaxMin[i].max - MaxMin[i].min) + MaxMin[i].min;
					}
				}
				return result;
			}

			Matrix<T>& Normalize(Matrix<T>& data, bool)
			{
				for (size_t i = 0; i < data.Cols(); i++)
				{
					for (size_t j = 0; j < data.Rows(); j++)
					{
						data[j][i] = (data[j][i] - MaxMin[i].min) / (MaxMin[i].max - MaxMin[i].min);
					}
				}
				return data;
			}

			Matrix<T>& Denormalize(Matrix<T>& data, bool)
			{
				for (size_t i = 0; i < data.Cols(); i++)
				{
					for (size_t j = 0; j < data.Rows(); j++)
					{
						data[j][i] = data[j][i] * (MaxMin[i].max - MaxMin[i].min) + MaxMin[i].min;
					}
				}
				return data;
			}

		};
		double sigmoid(double x);
		double sigmoid_derivative(double x);
	}
}
