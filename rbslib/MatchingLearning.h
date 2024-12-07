#pragma once
#include <vector>
#include "Math.h"
#include <functional>
#include "Storage.h"
#include "FileIO.h"
#include "MatchingLearningBase.h"


namespace RbsLib::MatchingLearning 
{
	class NeuralNetworks
	{
	private:
		
		std::vector<RbsLib::MatchingLearning::Layer> layers;
	public:
		NeuralNetworks(std::vector<int> layers, std::vector<std::function<float(float)>> activation, std::vector<std::function<float(float)>> activation_derivative, int random_seed);//参数表示每层神经元的个数
		void Train(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback);
		void TrainCUDA(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback, int activite_func_index);
		auto Predict(RbsLib::Math::Matrix<float> inputs) -> RbsLib::Math::Matrix<float>;
		void Save(const RbsLib::Storage::StorageFile& path);//将模型存储到文件，但不会保存神经网络结构和激活函数等信息
		void Load(const RbsLib::Storage::StorageFile& path);
	};

	template <typename T>
	class Normalization
	{
	private:
		struct MaxMinPack
		{
			float max;
			float min;
		};
		std::vector<MaxMinPack> MaxMin;
	public:
		void Fit(const RbsLib::Math::Matrix<T>& data)
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
		RbsLib::Math::Matrix<T> Normalize(const RbsLib::Math::Matrix<T>& data)
		{
			RbsLib::Math::Matrix<T> result(data.Rows(), data.Cols());
			for (size_t i = 0; i < data.Cols(); i++)
			{
				for (size_t j = 0; j < data.Rows(); j++)
				{
					result[j][i] = (data[j][i] - MaxMin[i].min) / (MaxMin[i].max - MaxMin[i].min);
				}
			}
			return result;
		}
		RbsLib::Math::Matrix<T> Denormalize(const RbsLib::Math::Matrix<T>& data)
		{
			RbsLib::Math::Matrix<T> result(data.Rows(), data.Cols());
			for (size_t i = 0; i < data.Cols(); i++)
			{
				for (size_t j = 0; j < data.Rows(); j++)
				{
					result[j][i] = data[j][i] * (MaxMin[i].max - MaxMin[i].min) + MaxMin[i].min;
				}
			}
			return result;
		}

		RbsLib::Math::Matrix<T>& Normalize(RbsLib::Math::Matrix<T>& data, bool)
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

		RbsLib::Math::Matrix<T>& Denormalize(RbsLib::Math::Matrix<T>& data, bool)
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

		void Save(const std::string& path)
		{
			auto fp = RbsLib::Storage::StorageFile(path).Open(RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Bin,
				RbsLib::Storage::FileIO::SeekBase::begin,
				0);
			for (const auto& mm : MaxMin)
			{
				fp.WriteData<T>(mm.max);
				fp.WriteData<T>(mm.min);
			}
		}
		void Load(const std::string& path)
		{
			auto fp = RbsLib::Storage::StorageFile(path).Open(RbsLib::Storage::FileIO::OpenMode::Read | RbsLib::Storage::FileIO::OpenMode::Bin,
				RbsLib::Storage::FileIO::SeekBase::begin,
				0);
			MaxMin.resize(RbsLib::Storage::StorageFile(path).GetFileSize() / (sizeof(T) * 2));
			if (MaxMin.size() == 0) throw std::invalid_argument("File size is 0");
			for (size_t i = 0; i < MaxMin.size(); i++)
			{
				fp.GetData<T>(MaxMin[i].max);
				fp.GetData<T>(MaxMin[i].min);
			}
		}

	};
}