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
	return std::sinf(5*x);
}

double LeakyReLU(double x) 
{
	return (x > 0) ? x : 0.01 * x;
}

double LeakyReLU_derivative(double x) {
	return (x > 0) ? 1 : 0.01;
}
double Tanh(double x)
{
	return std::tanh(x);
}

double Tanh_derivative(double x) 
{
	double tanh_x = std::tanh(x);
	return 1.0 - tanh_x * tanh_x; // 1 - tanh^2(x)
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
	//生成训练数据
	auto start_time = time(nullptr);
	//读取数据集
	FILE* fp = fopen("E:\\code\\cpp\\rbslib\\iris.txt", "r");
	if (fp == NULL)
	{
		std::cout << "文件打开失败" << std::endl;
		return 1;
	}
	char line[1024];
	RbsLib::Math::Matrix<double> x_data(150, 4), y_data(150, 3);
	//每次读取一行
	int is_read_line_1 = false;
	while (fgets(line, 1024, fp))
	{
		if (is_read_line_1 == false) {
			is_read_line_1 = true;
			continue;
		}
		static int i = 0;
		char str[1024];
		char type[128];
		//将读取的数据转换为double类型
		sscanf(line, "%s%lf%lf%lf%lf%s", str,&x_data[i][0], &x_data[i][1], &x_data[i][2], &x_data[i][3], &type);
		if (strcmp(type, "\"setosa\"") == 0)
		{
			y_data[i][0] = 1;
		}
		else if (strcmp(type, "\"versicolor\"") == 0)
		{
			y_data[i][1] = 1;
		}
		else if (strcmp(type, "\"virginica\"") == 0)
		{
			y_data[i][2] = 2;
		}
		i++;
	}
	//生成80%训练集
	auto train_index = GenerateUniqueRandomValues(0, 150, 120);
	RbsLib::Math::Matrix<double> x_train(120, 4), y_train(120, 3), x_test(30, 4), y_test(30, 3);
	for (int i = 0; i < 120; i++)
	{
		x_train[i] = x_data[train_index[i]];
		y_train[i] = y_data[train_index[i]];
	}
	//生成20%测试集
	int j = 0;
	for (int i = 0; i < 150; i++)
	{
		if (j >= x_test.Rows()) break;
		if (std::find(train_index.begin(), train_index.end(), i) == train_index.end())
		{
			x_test[j] = x_data[i];
			y_test[j] = y_data[i];
			j++;
		}
	}
	//数据归一化
	RbsLib::Math::Normalization<double> normalization_x, normalization_y;
	normalization_x.Fit(x_train);
	normalization_x.Normalize(x_train, true);
	//创建神经网络
	RbsLib::MatchingLearning::NeuralNetworks net(
		{ 4,20,3 },
		{ RbsLib::Math::sigmoid,RbsLib::Math::sigmoid,RbsLib::Math::sigmoid },
		{ RbsLib::Math::sigmoid_derivative,RbsLib::Math::sigmoid_derivative,RbsLib::Math::sigmoid_derivative },
		0);
	//训练
	std::vector<double> loss_out;
	net.Train(x_train, y_train, 0.01, 20000, [&loss_out](int epoch, double loss)
		{
			static int n = 0;
			n += 1;
			if (n % 100 == 0)
			{
				loss_out.push_back(loss);
				std::cout << std::format("epoch:{},loss:{}\n", epoch, loss);
			}
		});
	//预测数据
	//将测试集归一化
	normalization_x.Normalize(x_test, true);
	auto y_predict = net.Predict(x_test);
	for (int i = 0; i < y_predict.Rows(); ++i)
	{
		//查找真实值是哪一个
		int real_index = max_index(y_test[i][0], y_test[i][1], y_test[i][2])-1;
		int predict_index = max_index(y_predict[i][0], y_predict[i][1], y_predict[i][2]) - 1;
		std::cout << std::format("真实值:{} 预测值:{}\n", real_index, predict_index);
	}
	RbsLib::Windows::Graph::Plot plot_loss;
	plot_loss.AddPlot(RbsLib::Math::Matrix<double>::LinearSpace(0, loss_out.size(), loss_out.size()).Column(0), loss_out, D2D1::ColorF::Red);
	plot_loss.Show("Loss");
	/*
	//归一化数据集
	RbsLib::Math::Normalization<double> normalization_x,normalization_y;
	normalization_x.Fit(inputs);
	normalization_x.Normalize(inputs,true);
	normalization_y.Fit(target);
	normalization_y.Normalize(target,true);

	//创建神经网络
	RbsLib::MatchingLearning::NeuralNetworks nn({ 1,5,5,5,5,1 },
		{ Tanh,Tanh,Tanh,Tanh,Tanh,Tanh },
		{ Tanh_derivative,Tanh_derivative,Tanh_derivative,Tanh_derivative,Tanh_derivative,Tanh_derivative },
		0);
	//训练
	std::vector<double> loss_out;
	nn.Train(inputs, target, 0.02, 100000, [&loss_out](int epoch, double loss)
		{
			static int n = 0;
			n += 1;
			if (n % 100 == 0)
			{
				loss_out.push_back(loss);
				std::cout << std::format("epoch:{},loss:{}\n", epoch, loss);
			}
		});
	//预测
	auto x_predict = RbsLib::Math::Matrix<double>::LinearSpace(2, 8, 100);
	normalization_x.Normalize(x_predict,true);
	auto y_predict = nn.Predict(x_predict);
	normalization_y.Denormalize(y_predict,true);
	normalization_x.Denormalize(x_predict,true);
	RbsLib::Windows::Graph::Plot plot;
	plot.AddPlot(x_predict.Column(0), y_predict.Column(0), D2D1::ColorF::Red);
	std::vector<double> y_real;
	for (auto i : x_predict.Column(0))
	{
		y_real.push_back(func(i));
	}
	plot.AddPlot(x_predict.Column(0), y_real, D2D1::ColorF::Blue);
	std::cout << "共使用时间:" << time(nullptr) - start_time << "s" << std::endl;
	std::thread t([&plot, &loss_out]()
		{
			RbsLib::Windows::Graph::Plot plot_loss;
			plot_loss.AddPlot(RbsLib::Math::Matrix<double>::LinearSpace(0, loss_out.size(), loss_out.size()).Column(0), loss_out, D2D1::ColorF::Red);
			plot_loss.Show("Loss");
		});
	plot.Show("Result");
	t.join();
	*/
	return 0;
}