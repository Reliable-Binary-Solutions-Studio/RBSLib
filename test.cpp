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
#include <cuda_runtime.h>


float func(float x)
{
	return sin(x);
}


std::vector<int> GenerateUniqueRandomValues(int min, int max, size_t count) {
	std::set<int> unique_values; // 用集合来保证不重复
	std::random_device rd; // 获取随机设备
	std::mt19937 gen(rd()); // 生成随机数引擎
	std::uniform_real_distribution<> dis(min, max); // 均匀分布

	// 不断生成直到集合中有指定个数的唯一值
	while (unique_values.size() < count) {
		float value = dis(gen); // 生成随机数
		unique_values.insert(value); // 插入集合，自动去重
	}

	// 将结果转化为 vector 返回
	return std::vector<int>(unique_values.begin(), unique_values.end());
}

int min_index(float a, float b, float c) {
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

int max_index(float a, float b, float c) {
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

int getThreadNum()
{
	cudaDeviceProp prop;
	int count;

	cudaGetDeviceCount(&count);
	printf("gpu num %d\n", count);
	cudaGetDeviceProperties(&prop, 0);
	printf("max thread num: %d\n", prop.maxThreadsPerBlock);
	printf("max grid dimensions: %d, %d, %d)\n",
		prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
	return prop.maxThreadsPerBlock;
}
int main()
{
	
	getThreadNum();
	srand(time(0));
	//生成训练数据
	try
	{
		auto X_train = RbsLib::Math::Matrix<float>::LinearSpace(0, 10, 100);
		RbsLib::Math::Matrix<float> Y_train(100, 1);
		for (int i = 0; i < 100; ++i)
		{
			Y_train[i][0] = func(X_train[i][0]);
		}
		//归一化
		RbsLib::MatchingLearning::Normalization<float> norm_x, norm_y;
		norm_x.Fit(X_train);
		norm_y.Fit(Y_train);
		norm_x.Normalize(X_train,true);
		norm_y.Normalize(Y_train,true);
		//构建神经网络
		RbsLib::MatchingLearning::NeuralNetworks nn({ 1, 120,80, 1 },
			{ RbsLib::Math::sigmoid, RbsLib::Math::sigmoid, RbsLib::Math::sigmoid },
			{  RbsLib::Math::sigmoid_derivative,RbsLib::Math::sigmoid_derivative, RbsLib::Math::sigmoid_derivative },
			0);
		auto nn1 = nn;
		//训练
		std::vector<double> loss;
		nn1.Train(X_train, Y_train, 0.1,50000, [&loss](int epoch, float los) {
			static int i = 0;
			if (i > 1)
			{
				std::cout << std::format("epoch:{},loss:{}", epoch, los) << std::endl;
				i = 0;
				loss.push_back(los);
			}
			++i;
			});
		std::cout << "-----------------------------------------" << std::endl;
		auto start_time = std::chrono::high_resolution_clock::now();
		nn.TrainCUDA(X_train, Y_train, 0.1, 0, [&loss](int epoch, float los) {
			static int i = 0;
			if (i > 1)
			{
				std::cout << std::format("epoch:{},loss:{}", epoch, los) << std::endl;
				i = 0;
				loss.push_back(los);
			}
			++i;
			},1);
		auto end_time = std::chrono::high_resolution_clock::now();
		std::cout << "cuda time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;
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