#include "rbslib/Storage.h"
#include "rbslib/Math.h"
#include <stdlib.h>
#include <iostream>
#include <format>
#include "rbslib/MatchingLearning.h"
#include "rbslib/Windows/Plot.h"
#include <stdio.h>
#include <set>
#include <random>
#include <initializer_list>
#include <algorithm>

double func(double x)
{
	return sin(2 * x);
}


std::vector<int> GenerateUniqueRandomValues(int min, int max, size_t count) {
	std::set<int> unique_values; // 用集合来保证不重复
	std::random_device rd; // 获取随机设备
	std::mt19937 gen(rd()); // 生成随机数引擎
	std::uniform_real_distribution<> dis(min, max); // 均匀分布

	// 不断生成直到集合中有指定个数的唯一值
	while (unique_values.size() < count) {
		double value = dis(gen); // 生成随机数
		unique_values.insert(value); // 插入集合，自动去重
	}

	// 将结果转化为 vector 返回
	return std::vector<int>(unique_values.begin(), unique_values.end());
}

int min_index(double a, double b, double c) {
	if (a <= b && a <= c) {
		return 1;  // a 是最小的，位置是第一个
	}
	else if (b <= a && b <= c) {
		return 2;  // b 是最小的，位置是第二个
	}
	else {
		return 3;  // c 是最小的，位置是第三个
	}
}

int max_index(double a, double b, double c) {
	if (a >= b && a >= c) {
		return 1;  // a 是最大的，位置是第一个
	}
	else if (b >= a && b >= c) {
		return 2;  // b 是最大的，位置是第二个
	}
	else {
		return 3;  // c 是最大的，位置是第三个
	}
}

int main()
{
	srand(time(0));
	//生成训练数据
	try
	{
		auto start_time = time(nullptr);
		//生成数据为目标函数加上随机噪声
		auto x_data = RbsLib::Math::Matrix<double>::LinearSpace(0, 10, 100);
		auto y_data = x_data;
		for (int i = 0; i < x_data.Rows(); ++i)
		{
			x_data[i][0] = x_data[i][0] + 0.1 * (rand() % 100) / 100;
		}
		for (int i = 0; i < y_data.Rows(); ++i)
		{
			y_data[i][0] = func(y_data[i][0]) + 0.1 * (rand() % 100) / 100;
		}
		//数据归一化
		RbsLib::Math::Normalization<double> normalization_x, normalization_y;
		normalization_x.Fit(x_data);
		normalization_x.Normalize(x_data, true);
		normalization_y.Fit(y_data);
		normalization_y.Normalize(y_data, true);
		//创建神经网络
		auto lrelu = [](double x) {return RbsLib::Math::LeakyReLU(x); };
		auto lrelu_d = [](double x) {return RbsLib::Math::LeakyReLU_derivative(x); };
		RbsLib::MatchingLearning::NeuralNetworks net(
			{ 1,20,5,1 },
			{ lrelu,lrelu,lrelu },
			{ lrelu_d,lrelu_d,lrelu_d },
			0);
		//训练
		std::vector<double> loss_out;
		net.Train(x_data, y_data, 0.01, 10000, [&loss_out](int epoch, double loss)
			{
				static int n = 0;
				n += 1;
				if (n % 100 == 0)
				{
					loss_out.push_back(loss);
					std::cout << std::format("epoch:{},loss:{}\n", epoch, loss);
				}
			});
		srand(time(0));
		//预测数据
		auto x_test = RbsLib::Math::Matrix<double>::LinearSpace(0, 10, 100);
		auto y_predict = net.Predict(normalization_x.Normalize(x_test));
		normalization_y.Denormalize(y_predict,true);
		for (int i = 0; i < y_predict.Rows(); ++i)
		{
			std::cout << std::format("真实值:{} 预测值:{}\n", func(x_test[i][0]), y_predict[i][0]);
		}
		auto y_true = y_predict;
		for (int i = 0; i < y_true.Rows(); ++i)
		{
			y_true[i][0] = func(x_test[i][0]);
		}
		//绘制预测曲线
		normalization_x.Denormalize(x_data,true);
		normalization_y.Denormalize(y_data,true);
		RbsLib::Windows::Graph::Plot plot;
		plot.AddPlot(x_test.Column(0), y_predict.Column(0), D2D1::ColorF::Red);
		//绘制真实曲线
		plot.AddPlot(x_test.Column(0), y_true.Column(0), D2D1::ColorF::Blue);
		//绘制训练集散点
		plot.AddScatter(x_data.Column(0), y_data.Column(0), D2D1::ColorF::Green);
		//在子线程中绘制loss曲线
		std::thread t([&loss_out]()
			{
				RbsLib::Windows::Graph::Plot plot_loss;
				plot_loss.AddPlot(RbsLib::Math::Matrix<double>::LinearSpace(0, loss_out.size(), loss_out.size()).Column(0), loss_out, D2D1::ColorF::Red);
				plot_loss.Show("Loss");
			});
		//显示图像
		plot.Show("Function Fitting");
		t.join();
		
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}