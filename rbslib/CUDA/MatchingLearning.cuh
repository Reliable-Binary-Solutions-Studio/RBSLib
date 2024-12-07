#pragma once
#include <vector>
#include "../Math.h"
#include "../MatchingLearningBase.h"
#include <functional>

void __TrainCUDA(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback, std::vector<RbsLib::MatchingLearning::Layer>& layers,int activite_index);