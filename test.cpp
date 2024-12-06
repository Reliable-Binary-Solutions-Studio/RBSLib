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
	RbsLib::Math::Matrix<int> a({ {1,2,3} });
	std::vector<std::vector<int>> x({ {1},{2},{3} });
	RbsLib::Math::Matrix<int> b(x);
	//a[1][1] = 10;
	std::cout << (b).ToString() << std::endl;
	//生成训练数据
	try
	{
		int data_size = 6472;
		RbsLib::Math::Matrix<double> X(data_size,9);
		RbsLib::Math::Matrix<double> Y(data_size, 2);
		//读取数据
		FILE* fp = fopen("C:\\Users\\yangp\\Desktop\\info.txt", "rt");
		char line[1024];
		int i = 0;
		while (fgets(line, 1024, fp))
		{
			sscanf(line, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
				&X[i][0], &X[i][1], &X[i][2],
				&X[i][3], &X[i][4], &X[i][5],
				&X[i][6], &X[i][7], &X[i][8],
				&Y[i][0], &Y[i][1]);
			++i;
		}
		//拆分训练集测试集，训练集占90%
		int train_size = data_size * 0.9;
		int test_size = data_size - train_size;
		auto train_index = GenerateUniqueRandomValues(0, data_size, train_size);
		std::vector<int> test_index;
		for (int i = 0; i < data_size; ++i)
		{
			if (std::find(train_index.begin(), train_index.end(), i) == train_index.end())
			{
				test_index.push_back(i);
			}
		}
		RbsLib::Math::Matrix<double> X_train(train_size, 9);
		RbsLib::Math::Matrix<double> Y_train(train_size, 2);
		for (int i = 0; i < train_size; ++i)
		{
			X_train.Row(i, X[train_index[i]].ToVector());
			Y_train.Row(i, Y[train_index[i]].ToVector());
		}
		//归一化
		RbsLib::Math::Normalization<double> norm_x, norm_y;
		norm_x.Fit(X_train);
		norm_y.Fit(Y_train);
		norm_x.Normalize(X_train,true);
		norm_y.Normalize(Y_train,true);
		//构建神经网络
		RbsLib::MatchingLearning::NeuralNetworks nn({ 9, 120, 80, 2 },
			{ RbsLib::Math::sigmoid, RbsLib::Math::sigmoid, RbsLib::Math::sigmoid },
			{ RbsLib::Math::sigmoid_derivative, RbsLib::Math::sigmoid_derivative, RbsLib::Math::sigmoid_derivative },
			0);
		//训练
		std::vector<double> loss;
		nn.Train(X_train, Y_train, 0.01, 1000, [&loss](int epoch, double los) {
			static int i = 0;
			if (i > 1)
			{
				std::cout << std::format("epoch:{},loss:{}", epoch, los) << std::endl;
				i = 0;
				loss.push_back(los);
			}
			++i;
			});
		nn.Save("car_model");
		norm_x.Save("car_model_norm_x");
		norm_y.Save("car_model_norm_y");
		//测试
		RbsLib::Math::Matrix<double> X_test(test_size, 9), Y_test(test_size, 2);
		for (int i = 0; i < test_size; ++i)
		{
			X_test.Row(i, X[test_index[i]].ToVector());
			Y_test.Row(i, Y[test_index[i]].ToVector());
		}
		norm_x.Normalize(X_test, true);
		auto y_pred = nn.Predict(X_test);
		//反归一化
		norm_y.Denormalize(y_pred, true);
		//输出
		for (int i = 0; i < test_size; ++i)
		{
			std::cout << std::format("y_true:{:<10},{:<10},y_pred:{:<10},{:<10}", Y_test[i][0],Y_test[i][1], y_pred[i][0],y_pred[i][1]) << std::endl;
		}
		//绘制损失曲线
		RbsLib::Windows::Graph::Plot plot;
		auto loss_x = std::vector<double>(loss.size());
		for (int i = 0; i < loss.size(); ++i)
		{
			loss_x[i] = i;
		}
		plot.AddPlot(loss_x, loss,D2D1::ColorF::Red);	
		plot.Show("loss");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}