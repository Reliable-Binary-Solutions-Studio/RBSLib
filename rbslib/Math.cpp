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
