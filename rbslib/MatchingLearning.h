#pragma once
#include <vector>
#include "Math.h"
#include <functional>
#include "Storage.h"

namespace RbsLib::MatchingLearning 
{
	class NeuralNetworks
	{
	private:
		struct Layer
		{
			RbsLib::Math::Matrix<double> w;
			RbsLib::Math::Matrix<double> b;
			RbsLib::Math::Matrix<double> output;
			RbsLib::Math::Matrix<double> delta;
			RbsLib::Math::Matrix<double> z;
			std::function<double(double)> activation;
			std::function<double(double)> activation_derivative;
		};
		std::vector<Layer> layers;
	public:
		NeuralNetworks(std::vector<int> layers, std::vector<std::function<double(double)>> activation, std::vector<std::function<double(double)>> activation_derivative, int random_seed);//参数表示每层神经元的个数
		void Train(RbsLib::Math::Matrix<double> inputs, RbsLib::Math::Matrix<double> target, double learning_rate, int epochs, std::function<void(int, double)> loss_callback);
		auto Predict(RbsLib::Math::Matrix<double> inputs) -> RbsLib::Math::Matrix<double>;
		void Save(const RbsLib::Storage::StorageFile& path);//将模型存储到文件，但不会保存神经网络结构和激活函数等信息
		void Load(const RbsLib::Storage::StorageFile& path);
	};
}