#pragma once
#include "BaseType.h"
#include <vector>
#include <string>
#include <stdexcept>

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
			for (size_t i = 0; i < a.Rows(); i++)
			{
				for (size_t j = 0; j < b.Cols(); j++)
				{
					for (size_t k = 0; k < a.Cols(); k++)
					{
						result[i][j] += a[i][k] * b[k][j];
					}
				}
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
			T min_value;
			T max_value;
		public:
			Normalization() : min_value(0), max_value(0)
			{
			}
			void Fit(const std::vector<T>& data)
			{
				min_value = data.front();
				max_value = data.front();
				for (const auto& value : data)
				{
					if (value < min_value)
					{
						min_value = value;
					}
					if (value > max_value)
					{
						max_value = value;
					}
				}
			}
			T Normalize(T value)
			{
				return (value - min_value) / (max_value - min_value);
			}
			T Denormalize(T value)
			{
				return value * (max_value - min_value) + min_value;
			}
			std::vector<double> Normalize(const std::vector<T>& data)
			{
				std::vector<double> result;
				for (const auto& value : data)
				{
					result.push_back(Normalize(value));
				}
				return result;
			}
			std::vector<T> Denormalize(const std::vector<double>& data)
			{
				std::vector<T> result;
				for (const auto& value : data)
				{
					result.push_back(Denormalize(value));
				}
				return result;
			}
		};
	}
}
