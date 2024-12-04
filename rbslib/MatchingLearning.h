#pragma once
#include <vector>
#include "Math.h"

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
		};
		std::vector<Layer> layers;
	public:
		NeuralNetworks(std::vector<int> layers,int random_seed);//参数表示每层神经元的个数
		void Train(RbsLib::Math::Matrix<double> inputs, RbsLib::Math::Matrix<double> target, double learning_rate, int epochs,void (*loss_callback)(int,double));
		auto Predict(RbsLib::Math::Matrix<double> inputs) -> RbsLib::Math::Matrix<double>;
	};
}