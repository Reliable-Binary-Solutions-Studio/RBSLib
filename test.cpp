#include "rbslib/Storage.h"
#include "rbslib/Math.h"
#include <stdlib.h>
#include <iostream>
#include <format>
#include "rbslib/MatchingLearning.h"
#include "rbslib/Windows/Plot.h"
double func(double x)
{
	return x * x * x + 2 * x * x + 3 * x + 4;
}
int main() 
{
	//生成训练数据
	RbsLib::Math::Matrix<double> inputs(100, 1);
	RbsLib::Math::Matrix<double> target(100, 1);
	double x = 0;
	for (int i = 0; i < 100; ++i)
	{
		inputs[i][0] = x;
		target[i][0] = func(x);
		x += 0.1;
	}
	//归一化数据集
	//找到最大值和最小值
	double max_x = inputs[0][0];
	double min_x = inputs[0][0];
	for (int i = 0; i < 100; ++i)
	{
		if (inputs[i][0] > max_x)
		{
			max_x = inputs[i][0];
		}
		if (inputs[i][0] < min_x)
		{
			min_x = inputs[i][0];
		}
	}
	//归一化
	for (int i = 0; i < 100; ++i)
	{
		inputs[i][0] = (inputs[i][0] - min_x) / (max_x - min_x);
	}
	//找到最大值和最小值
	double max_y = target[0][0];
	double min_y = target[0][0];
	for (int i = 0; i < 100; ++i)
	{
		if (target[i][0] > max_y)
		{
			max_y = target[i][0];
		}
		if (target[i][0] < min_y)
		{
			min_y = target[i][0];
		}
	}
	//归一化
	for (int i = 0; i < 100; ++i)
	{
		target[i][0] = (target[i][0] - min_y) / (max_y - min_y);
	}

	//创建神经网络
	RbsLib::MatchingLearning::NeuralNetworks nn({ 1,50,1 }, 0);
	//训练
	nn.Train(inputs, target, 0.01, 3000, [](int epoch, double loss)
		{
			std::cout << std::format("epoch:{},loss:{}\n", epoch, loss);
		});
	//预测
	
	std::vector<double> x_list, y_pred_list, y_list;
	for (int i = 0; i < 10; ++i)
	{
		RbsLib::Math::Matrix<double> test_input(1, 1);
		test_input[0][0] = i;
		test_input[0][0] = (test_input[0][0] - min_x) / (max_x - min_x);
		auto result = nn.Predict(test_input);
		std::cout << std::format("预测值:{}\n", result[0][0] * (max_y - min_y) + min_y);
		std::cout << std::format("真实值:{}\n", func(i));
		x_list.push_back(i);
		y_pred_list.push_back(result[0][0] * (max_y - min_y) + min_y);
		y_list.push_back(func(i));
		std::cout << std::endl;
	}
	RbsLib::Windows::Graph::Plot plot;
	plot.AddPlot(x_list, y_list, D2D1::ColorF::Red);
	plot.AddPlot(x_list, y_pred_list, D2D1::ColorF::Blue);
	plot.Show();
	return 0;
}