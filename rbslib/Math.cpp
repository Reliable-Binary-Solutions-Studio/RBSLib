#include "Math.h"
#include <cmath>

double RbsLib::Math::sigmoid(double x)
{
	return 1.0 / (1.0 + std::exp(-x));
}

double RbsLib::Math::sigmoid_derivative(double x)
{
	return sigmoid(x) * (1 - sigmoid(x));
}

double RbsLib::Math::LeakyReLU(double x,double alpha)
{
	return (x > 0) ? x : alpha * x;
}

double RbsLib::Math::LeakyReLU_derivative(double x,double alpha)
{
	return (x > 0) ? 1 : alpha;
}

double RbsLib::Math::Tanh(double x)
{
	return std::tanh(x);
}

double RbsLib::Math::Tanh_derivative(double x)
{
	double tanh_x = std::tanh(x);
	return 1.0 - tanh_x * tanh_x; // 1 - tanh^2(x)
}
