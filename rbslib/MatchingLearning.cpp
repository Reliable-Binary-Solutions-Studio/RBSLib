#include "MatchingLearning.h"
#include <cmath>
#include "FileIO.h"

#ifdef ENABLE_CUDA
#include "./CUDA/MatchingLearning.cuh"
#endif

RbsLib::MatchingLearning::NeuralNetworks::NeuralNetworks(
    std::vector<int> layers,
    std::vector<std::function<float(float)>> activation,
    std::vector<std::function<float(float)>> activation_derivative,
    int random_seed)
    : layers(layers.size())
{
    srand(random_seed);

    // 确保激活函数和导数函数数量与层数匹配
    if (activation.size() != layers.size() - 1 || activation_derivative.size() != layers.size() - 1)
    {
        throw std::invalid_argument("激活函数和导数函数数量必须与层数匹配");
    }

    // 初始化每一层
    for (size_t i = 0; i < layers.size(); i++)
    {
        // 第一个层是输入层，不需要权重
        if (i == 0)
        {
            this->layers[i].w = RbsLib::Math::Matrix<float>(layers[i], 1); // 输入层没有权重
        }
        else
        {
            // 对于隐藏层和输出层，使用 Xavier 初始化
            float limit = std::sqrt(6.0 / (layers[i - 1] + layers[i]));
            this->layers[i].w = RbsLib::Math::Matrix<float>(layers[i], layers[i - 1]);

            // Xavier 初始化：随机选择在 [-limit, limit] 范围内的值
            for (int row = 0; row < this->layers[i].w.Rows(); ++row)
            {
                for (int col = 0; col < this->layers[i].w.Cols(); ++col)
                {
                    float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] 范围的随机数
                    this->layers[i].w[row][col] = rand_val;
                }
            }
            // 设置激活函数和导数
            this->layers[i].activation = activation[i-1];
            this->layers[i].activation_derivative = activation_derivative[i-1];
        }

        // 偏置初始化为零
        this->layers[i].b = RbsLib::Math::Matrix<float>(layers[i], 1);

        // 初始化输出和delta矩阵
        this->layers[i].output = RbsLib::Math::Matrix<float>(layers[i], 1);
        this->layers[i].delta = RbsLib::Math::Matrix<float>(layers[i], 1);


        //初始化Z矩阵
		this->layers[i].z = RbsLib::Math::Matrix<float>(layers[i], 1);
    }
}

void RbsLib::MatchingLearning::NeuralNetworks::Train(
    RbsLib::Math::Matrix<float> inputs,
    RbsLib::Math::Matrix<float> target,
    float learning_rate,
    int epochs,
	std::function<void(int,float)> loss_callback)
{
    //先检查输入输出是否匹配
    if (inputs.Rows() != target.Rows())
    {
        throw std::invalid_argument("输入输出数据行数不匹配");
    }
    if (inputs.Cols() != layers[0].w.Rows())
    {
        throw std::invalid_argument("输入数据列数与输入层神经元个数不匹配");
    }
    if (target.Cols() != layers.back().w.Rows())
    {
        throw std::invalid_argument("输出数据列数与输出层神经元个数不匹配");
    }

    // 开始训练
    for (int e = 0; e < epochs; ++e)
    {
        // 第 e 轮
        float total_loss = 0.0;  // 用来累计损失
        for (int i = 0; i < inputs.Rows(); ++i)
        {
            int index = i; rand() % inputs.Rows();  // 随机选择一个样本

            // 前向传播
            // 第 1 层的输出就是输入
            for (int j = 0; j < layers[0].output.Rows(); ++j)
            {
                layers[0].output[j][0] = inputs[index][j];
            }

            // 从第二层开始
            for (int j = 1; j < layers.size(); ++j)
            {
                // 计算当前层的输出 Z = W * A + b
                layers[j].z = layers[j].w * layers[j - 1].output + layers[j].b;  // 计算Z[l]

                // 应用激活函数
                for (int k = 0; k < layers[j].output.Rows(); ++k)
                {
                    layers[j].output[k][0] = layers[j].activation(layers[j].z[k][0]);  // 使用激活函数计算输出
                }
            }

            // 计算损失（均方误差）
            float loss = 0.0;
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                float error = target[index][j] - layers.back().output[j][0]; // 预测误差
                loss += 0.5 * error * error; // MSE损失
            }
            total_loss += loss;
         

            // 反向传播
            // 计算输出层的误差项 δ[L]
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                float error = layers.back().output[j][0] - target[index][j];
                layers.back().delta[j][0] = error * layers.back().activation_derivative(layers.back().z[j][0]);  // 使用激活函数导数
            }

            // 计算隐藏层的误差项 δ[L-1], δ[L-2], ..., δ[1]
            for (int l = layers.size() - 2; l >= 1; --l)
            {
                for (int j = 0; j < layers[l].delta.Rows(); ++j)
                {
                    float error_sum = 0.0;
                    for (int k = 0; k < layers[l + 1].delta.Rows(); ++k)
                    {
                        error_sum += layers[l + 1].delta[k][0] * layers[l + 1].w[k][j];
                    }
                    layers[l].delta[j][0] = error_sum * layers[l].activation_derivative(layers[l].z[j][0]);  // 使用激活函数导数
                }
            }
            // 更新权重和偏置
            for (int l = 1; l < layers.size(); ++l)
            {
                // 权重更新： W[l] = W[l] - learning_rate * (δ[l] * A[l-1]^T)
                
				layers[l].w = layers[l].w - learning_rate * layers[l].delta * layers[l - 1].output.T();


                // 偏置更新： b[l] = b[l] - learning_rate * δ[l]
				layers[l].b = layers[l].b - learning_rate * layers[l].delta;
            }
        }
        // 输出每轮的损失
        loss_callback(e, total_loss / inputs.Rows());
    }
}

void RbsLib::MatchingLearning::NeuralNetworks::TrainCUDA(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback,int activite_func_index)
{
#ifndef ENABLE_CUDA
	throw std::runtime_error("CUDA is not enabled");
#else
	__TrainCUDA(inputs, target, learning_rate, epochs, loss_callback,this->layers,activite_func_index);
#endif
}

auto RbsLib::MatchingLearning::NeuralNetworks::Predict(RbsLib::Math::Matrix<float> inputs) -> RbsLib::Math::Matrix<float>
{
    //检查输入输出是否匹配
    if (inputs.Cols() != layers[0].w.Rows())
    {
        throw std::invalid_argument("输入数据列数与输入层神经元个数不匹配");
    }

    //拷贝当前模型状态
    auto temp_layers = layers;

    //构造输出结果矩阵
    RbsLib::Math::Matrix<float> result(inputs.Rows(), layers.back().output.Rows());

    //利用拷贝的模型状态进行预测
    for (int i = 0; i < inputs.Rows(); ++i)
    {
        // 第 1 层的输出就是输入
        for (int j = 0; j < temp_layers[0].output.Rows(); ++j)
        {
            temp_layers[0].output[j][0] = inputs[i][j];
        }

        // 从第二层开始
        for (int j = 1; j < temp_layers.size(); ++j)
        {
            // 计算当前层的输出 Z = W * A + b
            temp_layers[j].z = temp_layers[j].w * temp_layers[j - 1].output + temp_layers[j].b;  // 计算Z[l]

            // 应用激活函数
            for (int k = 0; k < temp_layers[j].output.Rows(); ++k)
            {
                temp_layers[j].output[k][0] = temp_layers[j].activation(temp_layers[j].z[k][0]);  // 使用激活函数计算输出
            }
        }

        // 存储结果
        result[i] = temp_layers.back().output.T()[0];
    }
    return result;
}

void RbsLib::MatchingLearning::NeuralNetworks::Save(const RbsLib::Storage::StorageFile& path)
{
	//保存模型
	auto fp = path.Open(RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Bin,
        RbsLib::Storage::FileIO::SeekBase::begin,
        0);
	for (const auto& layer : layers)
	{
		//存储w
		for (int i = 0; i < layer.w.Rows(); ++i)
		{
			for (int j = 0; j < layer.w.Cols(); ++j)
			{
				fp.WriteData<float>(layer.w[i][j]);
			}
		}
		//存储b
		for (int i = 0; i < layer.b.Rows(); ++i)
		{
			fp.WriteData<float>(layer.b[i][0]);
		}
	}
}

void RbsLib::MatchingLearning::NeuralNetworks::Load(const RbsLib::Storage::StorageFile& path)
{
	//加载模型
	auto fp = path.Open(RbsLib::Storage::FileIO::OpenMode::Read | RbsLib::Storage::FileIO::OpenMode::Bin,
		RbsLib::Storage::FileIO::SeekBase::begin,
		0);
	for (auto& layer : layers)
	{
		//加载w
		for (int i = 0; i < layer.w.Rows(); ++i)
		{
			for (int j = 0; j < layer.w.Cols(); ++j)
			{
				fp.GetData<float>(layer.w[i][j]);
			}
		}
		//加载b
		for (int i = 0; i < layer.b.Rows(); ++i)
		{
			fp.GetData<float>(layer.b[i][0]);
		}
	}
}
