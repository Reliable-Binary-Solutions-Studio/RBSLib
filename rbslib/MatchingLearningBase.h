#pragma once
#include "Math.h"

namespace RbsLib
{
	namespace MatchingLearning
	{
		struct Layer
		{
			RbsLib::Math::Matrix<float> w;
			RbsLib::Math::Matrix<float> b;
			RbsLib::Math::Matrix<float> output;
			RbsLib::Math::Matrix<float> delta;
			RbsLib::Math::Matrix<float> z;
			std::function<float(float)> activation;
			std::function<float(float)> activation_derivative;
		};
	}
}
