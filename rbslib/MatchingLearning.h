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
		NeuralNetworks(std::vector<int> layers, std::vector<std::function<double(double)>> activation, std::vector<std::function<double(double)>> activation_derivative, int random_seed);//������ʾÿ����Ԫ�ĸ���
		void Train(RbsLib::Math::Matrix<double> inputs, RbsLib::Math::Matrix<double> target, double learning_rate, int epochs, std::function<void(int, double)> loss_callback);
		auto Predict(RbsLib::Math::Matrix<double> inputs) -> RbsLib::Math::Matrix<double>;
		void Save(const RbsLib::Storage::StorageFile& path);//��ģ�ʹ洢���ļ��������ᱣ��������ṹ�ͼ��������Ϣ
		void Load(const RbsLib::Storage::StorageFile& path);
	};
}