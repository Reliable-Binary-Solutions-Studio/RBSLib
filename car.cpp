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
	return sin(2 * x);
}


std::vector<int> GenerateUniqueRandomValues(int min, int max, size_t count) {
	std::set<int> unique_values; // �ü�������֤���ظ�
	std::random_device rd; // ��ȡ����豸
	std::mt19937 gen(rd()); // �������������
	std::uniform_real_distribution<> dis(min, max); // ���ȷֲ�

	// ��������ֱ����������ָ��������Ψһֵ
	while (unique_values.size() < count) {
		float value = dis(gen); // ���������
		unique_values.insert(value); // ���뼯�ϣ��Զ�ȥ��
	}

	// �����ת��Ϊ vector ����
	return std::vector<int>(unique_values.begin(), unique_values.end());
}

int min_index(float a, float b, float c) {
	if (a <= b && a <= c) {
		return 1;  // a ����С�ģ�λ���ǵ�һ��
	}
	else if (b <= a && b <= c) {
		return 2;  // b ����С�ģ�λ���ǵڶ���
	}
	else {
		return 3;  // c ����С�ģ�λ���ǵ�����
	}
}

int max_index(float a, float b, float c) {
	if (a >= b && a >= c) {
		return 1;  // a �����ģ�λ���ǵ�һ��
	}
	else if (b >= a && b >= c) {
		return 2;  // b �����ģ�λ���ǵڶ���
	}
	else {
		return 3;  // c �����ģ�λ���ǵ�����
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
	//����ѵ������
	try
	{
		int data_size = 6472;
		RbsLib::Math::Matrix<float> X(data_size, 9);
		RbsLib::Math::Matrix<float> Y(data_size, 2);
		//��ȡ����
		FILE* fp = fopen("C:\\Users\\yangp\\Desktop\\info.txt", "rt");
		char line[1024];
		int i = 0;
		while (fgets(line, 1024, fp))
		{
			sscanf(line, "%f%f%f%f%f%f%f%f%f%f%f",
				&X[i][0], &X[i][1], &X[i][2],
				&X[i][3], &X[i][4], &X[i][5],
				&X[i][6], &X[i][7], &X[i][8],
				&Y[i][0], &Y[i][1]);
			++i;
		}
		//���ѵ�������Լ���ѵ����ռ90%
		int train_size = data_size * 0.1;
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
		RbsLib::Math::Matrix<float> X_train(train_size, 9);
		RbsLib::Math::Matrix<float> Y_train(train_size, 2);
		for (int i = 0; i < train_size; ++i)
		{
			X_train.Row(i, X[train_index[i]].ToVector());
			Y_train.Row(i, Y[train_index[i]].ToVector());
		}
		//��һ��
		RbsLib::MatchingLearning::Normalization<float> norm_x, norm_y;
		norm_x.Fit(X_train);
		norm_y.Fit(Y_train);
		norm_x.Normalize(X_train, true);
		norm_y.Normalize(Y_train, true);
		//����������
		RbsLib::MatchingLearning::NeuralNetworks nn({ 9, 120, 80, 2 },
			{ RbsLib::Math::sigmoid, RbsLib::Math::sigmoid, RbsLib::Math::sigmoid },
			{ RbsLib::Math::sigmoid_derivative, RbsLib::Math::sigmoid_derivative, RbsLib::Math::sigmoid_derivative },
			0);
		//ѵ��
		std::vector<double> loss;
		nn.TrainCUDA(X_train, Y_train, 0.01, 1000, [&loss](int epoch, float los) {
			static int i = 0;
			if (i > 1)
			{
				std::cout << std::format("epoch:{},loss:{}", epoch, los) << std::endl;
				i = 0;
				loss.push_back(los);
			}
			++i;
			}, 1);
		nn.Save("car_model");
		norm_x.Save("car_model_norm_x");
		norm_y.Save("car_model_norm_y");
		//����
		RbsLib::Math::Matrix<float> X_test(test_size, 9), Y_test(test_size, 2);
		for (int i = 0; i < test_size; ++i)
		{
			X_test.Row(i, X[test_index[i]].ToVector());
			Y_test.Row(i, Y[test_index[i]].ToVector());
		}
		norm_x.Normalize(X_test, true);
		auto y_pred = nn.Predict(X_test);
		//����һ��
		norm_y.Denormalize(y_pred, true);
		//���
		for (int i = 0; i < test_size; ++i)
		{
			std::cout << std::format("y_true:{:<10},{:<10},y_pred:{:<10},{:<10}", Y_test[i][0], Y_test[i][1], y_pred[i][0], y_pred[i][1]) << std::endl;
		}
		//������ʧ����
		RbsLib::Windows::Graph::Plot plot;
		auto loss_x = std::vector<double>(loss.size());
		for (int i = 0; i < loss.size(); ++i)
		{
			loss_x[i] = i;
		}
		plot.AddPlot(loss_x, loss, D2D1::ColorF::Red);
		plot.Show("loss");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}