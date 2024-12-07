#include "Math.h"
#include <cmath>

float RbsLib::Math::sigmoid(float x)
{
	return 1.0 / (1.0 + std::exp(-x));
}

float RbsLib::Math::sigmoid_derivative(float x)
{
	return sigmoid(x) * (1 - sigmoid(x));
}

float RbsLib::Math::LeakyReLU(float x,float alpha)
{
	return (x > 0) ? x : alpha * x;
}

float RbsLib::Math::LeakyReLU_derivative(float x,float alpha)
{
	return (x > 0) ? 1 : alpha;
}

float RbsLib::Math::Tanh(float x)
{
	return std::tanh(x);
}

float RbsLib::Math::Tanh_derivative(float x)
{
	float tanh_x = std::tanh(x);
	return 1.0 - tanh_x * tanh_x; // 1 - tanh^2(x)
}
