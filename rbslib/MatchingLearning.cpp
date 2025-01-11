#include "MatchingLearning.h"
#include <cmath>
#include "FileIO.h"
#include <random>

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
			// 计算输出层的误差项 δ[L] = (A[L] - Y) * f'(Z[L])
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
		for (int k = 0; k < temp_layers.back().output.Rows(); ++k)
		{
			result[i][k] = temp_layers.back().output[k][0];
		}
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


RbsLib::MatchingLearning::DDPG::ReplayBuffer::ReplayBuffer(size_t max_size)
	: max_size(max_size), states(max_size), actions(max_size), rewards(max_size), next_states(max_size), dones(max_size), current_size(0)
{
}

void RbsLib::MatchingLearning::DDPG::ReplayBuffer::Add(const RbsLib::Math::Matrix<float>& state, const RbsLib::Math::Matrix<float>& action, float reward, const RbsLib::Math::Matrix<float>& next_state, bool done)
{
	if (current_size == max_size)
	{
		throw std::runtime_error("Replay buffer full");
	}
	//添加经验
	states[current_size] = state;
	actions[current_size] = action;
	rewards[current_size] = reward;
	next_states[current_size] = next_state;
	dones[current_size] = done;

	//更新当前大小
    current_size += 1;
}

std::tuple<RbsLib::Math::Matrix<float>, RbsLib::Math::Matrix<float>, std::vector<float>, RbsLib::Math::Matrix<float>, std::vector<bool>>  RbsLib::MatchingLearning::DDPG::ReplayBuffer::Sample(size_t batch_size,int random_state)
{
	if (current_size < batch_size)
	{
		throw std::runtime_error("Replay buffer size less than batch size");
	}
	//随机选择batch_size个经验
	std::vector<int> batched_index(batch_size);
	srand(random_state);
	for (size_t i = 0; i < batch_size; i++)
	{
		batched_index[i] = rand() % current_size;
	}
	//构造返回值
	RbsLib::Math::Matrix<float> state(batch_size, states[0].Cols());
	RbsLib::Math::Matrix<float> action(batch_size, actions[0].Cols());
	RbsLib::Math::Matrix<float> next_state(batch_size, next_states[0].Cols());
	std::vector<float> reward(batch_size);
	std::vector<bool> done(batch_size);
	for (size_t i = 0; i < batch_size; i++)
	{
		for (int j = 0; j < states[0].Cols(); j++)
		{
			state[i][j] = states[batched_index[i]][0][j];
		}
		for (int j = 0; j < actions[0].Cols(); j++)
		{
			action[i][j] = actions[batched_index[i]][0][j];
		}
		reward[i] = rewards[batched_index[i]];
		for (int j = 0; j < next_states[0].Cols(); j++)
		{
			next_state[i][j] = next_states[batched_index[i]][0][j];
		}
		done[i] = dones[batched_index[i]];
	}
	return std::make_tuple(state, action, reward, next_state, done);
}

size_t RbsLib::MatchingLearning::DDPG::ReplayBuffer::CurrentSize()
{
	return current_size;
}

size_t RbsLib::MatchingLearning::DDPG::ReplayBuffer::MaxSize()
{
	return max_size;
}

void RbsLib::MatchingLearning::DDPG::ReplayBuffer::Clear()
{
	current_size = 0;
}

float RbsLib::MatchingLearning::DDPG::GaussianNoise(float mu, float sigma)
{
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(mu, sigma);
    return distribution(generator);
}

RbsLib::MatchingLearning::DDPG::DDPG(std::vector<int> actor_layers, std::vector<int> critic_layers, std::vector<std::function<float(float)>> actor_activation, std::vector<std::function<float(float)>> critic_activation, std::vector<std::function<float(float)>> actor_activation_derivative, std::vector<std::function<float(float)>> critic_activation_derivative, std::vector<float> actor_max, std::vector<float> actor_min, float gamma, float tau, float actor_learning_rate, float critic_learning_rate, int random_seed, size_t replay_buffer_size)
    :actor_layers( actor_layers.size()),
	critic_layers(critic_layers.size()),
	actor_activation(actor_activation),
	critic_activation(critic_activation),
	actor_activation_derivative(actor_activation_derivative),
	critic_activation_derivative(critic_activation_derivative),
	target_actor_layers(actor_layers.size()),
	target_critic_layers(critic_layers.size()),
	replay_buffer(replay_buffer_size),
	gamma(gamma),
	tau(tau),
	actor_learning_rate(actor_learning_rate),
	critic_learning_rate(critic_learning_rate),
	random_seed(random_seed),
	actor_max(actor_max),
	actor_min(actor_min)
{
	//检查actor和critic网络层数是否匹配
	if (actor_layers.size() != actor_activation.size() + 1 || 
        actor_layers.size() != actor_activation_derivative.size() + 1)
		throw std::invalid_argument("actor网络层数与激活函数数量不匹配");
	if (critic_layers.size() != critic_activation.size() + 1 ||
		critic_layers.size() != critic_activation_derivative.size() + 1)
		throw std::invalid_argument("critic网络层数与激活函数数量不匹配");
	//检查actor和critic网络层数是否匹配
    if (actor_layers.back()+actor_layers.front()!=critic_layers.front())
		throw std::invalid_argument("actor和critic网络输入输出不匹配");



    //依次初始化每个网络的矩阵
	//初始化actor网络
	srand(random_seed);
    for (int i = 0; i < this->actor_layers.size(); ++i)
    {
        if (i == 0)
        {
            this->actor_layers[i].w = RbsLib::Math::Matrix<float>(actor_layers[i], 1); // 输入层没有权重
        }
        else
        {
            // 对于隐藏层和输出层，使用 Xavier 初始化
            float limit = std::sqrt(6.0 / (actor_layers[i - 1] + actor_layers[i]));
            this->actor_layers[i].w = RbsLib::Math::Matrix<float>(actor_layers[i], actor_layers[i - 1]);

            // Xavier 初始化：随机选择在 [-limit, limit] 范围内的值
            for (int row = 0; row < this->actor_layers[i].w.Rows(); ++row)
            {
                for (int col = 0; col < this->actor_layers[i].w.Cols(); ++col)
                {
                    float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] 范围的随机数
                    this->actor_layers[i].w[row][col] = rand_val;
                }
            }
            // 设置激活函数和导数
            this->actor_layers[i].activation = actor_activation[i - 1];
            this->actor_layers[i].activation_derivative = actor_activation_derivative[i - 1];
        }

        // 偏置初始化为零
        this->actor_layers[i].b = RbsLib::Math::Matrix<float>(actor_layers[i], 1);

        // 初始化输出和delta矩阵
        this->actor_layers[i].output = RbsLib::Math::Matrix<float>(actor_layers[i], 1);
        this->actor_layers[i].delta = RbsLib::Math::Matrix<float>(actor_layers[i], 1);

        //初始化Z矩阵
		this->actor_layers[i].z = RbsLib::Math::Matrix<float>(actor_layers[i], 1);
    }
    //拷贝到actor_target网络
	for (int i = 0; i < this->actor_layers.size(); ++i)
	{
		this->target_actor_layers[i].w = this->actor_layers[i].w;
		this->target_actor_layers[i].b = this->actor_layers[i].b;
		this->target_actor_layers[i].output = this->actor_layers[i].output;
		this->target_actor_layers[i].delta = this->actor_layers[i].delta;
		this->target_actor_layers[i].z = this->actor_layers[i].z;
		this->target_actor_layers[i].activation = this->actor_layers[i].activation;
		this->target_actor_layers[i].activation_derivative = this->actor_layers[i].activation_derivative;
	}
	//初始化critic网络
	for (int i = 0; i < this->critic_layers.size(); ++i)
	{
		if (i == 0)
		{
			this->critic_layers[i].w = RbsLib::Math::Matrix<float>(critic_layers[i], 1); // 输入层没有权重
		}
		else
		{
			// 对于隐藏层和输出层，使用 Xavier 初始化
			float limit = std::sqrt(6.0 / (critic_layers[i - 1] + critic_layers[i]));
			this->critic_layers[i].w = RbsLib::Math::Matrix<float>(critic_layers[i], critic_layers[i - 1]);

			// Xavier 初始化：随机选择在 [-limit, limit] 范围内的值
			for (int row = 0; row < this->critic_layers[i].w.Rows(); ++row)
			{
				for (int col = 0; col < this->critic_layers[i].w.Cols(); ++col)
				{
					float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] 范围的随机数
					this->critic_layers[i].w[row][col] = rand_val;
				}
			}
			// 设置激活函数和导数
			this->critic_layers[i].activation = critic_activation[i - 1];
			this->critic_layers[i].activation_derivative = critic_activation_derivative[i - 1];
		}

		// 偏置初始化为零
		this->critic_layers[i].b = RbsLib::Math::Matrix<float>(critic_layers[i], 1);

		// 初始化输出和delta矩阵
		this->critic_layers[i].output = RbsLib::Math::Matrix<float>(critic_layers[i], 1);
		this->critic_layers[i].delta = RbsLib::Math::Matrix<float>(critic_layers[i], 1);

		//初始化Z矩阵
		this->critic_layers[i].z = RbsLib::Math::Matrix<float>(critic_layers[i], 1);
	}
	//拷贝到critic_target网络
	for (int i = 0; i < this->critic_layers.size(); ++i)
	{
		this->target_critic_layers[i].w = this->critic_layers[i].w;
		this->target_critic_layers[i].b = this->critic_layers[i].b;
		this->target_critic_layers[i].output = this->critic_layers[i].output;
		this->target_critic_layers[i].delta = this->critic_layers[i].delta;
		this->target_critic_layers[i].z = this->critic_layers[i].z;
		this->target_critic_layers[i].activation = this->critic_layers[i].activation;
		this->target_critic_layers[i].activation_derivative = this->critic_layers[i].activation_derivative;
	}
}

void RbsLib::MatchingLearning::DDPG::Train(size_t batch_size)
{
	//从replay_buffer中随机采样batch_size个经验
	auto [state, action, reward, next_state, done] = replay_buffer.Sample(batch_size, random_seed);
	//检查采集到的数据维数是否与网络匹配
	if (state.Cols() != actor_layers[0].w.Rows()||state.Cols()+action.Cols()!=critic_layers[0].w.Rows())
		throw std::invalid_argument("采集到的数据维数与输入层神经元个数不匹配");

    for (int i = 0; i < state.Rows(); ++i)
    {
        //计算yi
		//对actor_target向前传播
		// 第 1 层的输出就是输入
		for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
			target_actor_layers[0].output[j][0] = next_state[i][j];
		//从第二层开始
        for (int j = 1; j < target_actor_layers.size(); ++j)
        {
            //target网络不需要参与训练，所以不需要保留Z
            target_actor_layers[i].output = target_actor_layers[i].w * target_actor_layers[i - 1].output + target_actor_layers[i].b;  // 计算Z[l]
			target_actor_layers[i].output.Apply(target_actor_layers[i].activation);
        }
		//拼接向量作为Q网络的输入，A在前，S在后并直接作为Critic网络输入层的output
		for (int j = 0; j < target_actor_layers.back().output.Rows(); ++j)
			target_critic_layers[0].output[j][0] = target_actor_layers.back().output[j][0];
		for (int j = 0; j < state.Cols(); ++j)
			target_critic_layers[0].output[j + target_actor_layers.back().output.Rows()][0] = next_state[i][j];
		//从第二层开始
        for (int j = 1; j < target_critic_layers.size(); ++j)
        {
			target_critic_layers[j].z = target_critic_layers[j].w * target_critic_layers[j - 1].output + target_critic_layers[j].b;  // 计算Z[l]
			target_critic_layers[j].output = target_critic_layers[j].z;
        }
        //计算yi并存放在target_critic的output中
        //将reward扩展为列向量，每一项都=reward[i]
        RbsLib::Math::Matrix<float> reward_vector(target_critic_layers.back().output.Rows(), 1);
		for (int j = 0; j < reward_vector.Rows(); ++j) reward_vector[j][0] = reward[i];
        target_critic_layers.back().output = reward_vector + (1 - done[i]) * gamma * target_critic_layers.back().output;

		//更新critic网络
		// 第 1 层的输出就是输入，A在前，S在后
		for (int j = 0; j < action.Cols(); ++j)
			critic_layers[0].output[j][0] = action[i][j];
		for (int j = 0; j < state.Cols(); ++j)
			critic_layers[0].output[j + action.Cols()][0] = state[i][j];
		//从第二层开始
        for (int j = 1; j < critic_layers.size(); ++j)
        {
			critic_layers[j].z = critic_layers[j].w * critic_layers[j - 1].output + critic_layers[j].b;  // 计算Z[l]
			critic_layers[j].output = critic_layers[j].z;//拷贝到output
			critic_layers[j].output.Apply(critic_layers[j].activation);//应用激活函数
        }
		//计算loss,不输出因此不计算
        //反向传播

		//最后一层的误差项 δ[L] = (Y - A[L]) .* f'(Z[L])
        critic_layers.back().delta = (target_critic_layers.back().output - critic_layers.back().output).HadamardProduct(critic_layers.back().z.Apply(critic_layers.back().activation_derivative, true));
		//计算隐藏层的误差项 δ[L-1], δ[L-2], ..., δ[1]
        //δ[L] = W[L + 1] ^ T * δ[L + 1].*f'(Z[L])
		for (int l = critic_layers.size() - 2; l >= 1; --l)
		{
			critic_layers[l].delta = (critic_layers[l + 1].w.T() * critic_layers[l + 1].delta).HadamardProduct(critic_layers[l].z.Apply(critic_layers[l].activation_derivative, true));
		}
		//更新权重和偏置,梯度下降
		// W[l] = W[l] - learning_rate * (δ[l] * A[l-1]^T)
		for (int l = 1; l < critic_layers.size(); ++l)
		{
			critic_layers[l].w = critic_layers[l].w - critic_learning_rate * critic_layers[l].delta * critic_layers[l - 1].output.T();
			critic_layers[l].b = critic_layers[l].b - critic_learning_rate * critic_layers[l].delta;
		}

		//更新actor网络
		// 第 1 层的输出就是输入
		for (int j = 0; j < actor_layers[0].output.Rows(); ++j)
			actor_layers[0].output[j][0] = state[i][j];
		//从第二层开始
		for (int j = 1; j < actor_layers.size(); ++j)
		{
			actor_layers[j].z = actor_layers[j].w * actor_layers[j - 1].output + actor_layers[j].b;  // 计算Z[l]
			actor_layers[j].output = actor_layers[j].z.Apply(actor_layers[j].activation, true);
		}
		//继续在Critic网络中计算Q值
		// 第 1 层的输出就是输入，A在前，S在后
		for (int j = 0; j < action.Cols(); ++j)
			critic_layers[0].output[j][0] = actor_layers.back().output[j][0];
		for (int j = 0; j < state.Cols(); ++j)
			critic_layers[0].output[j + action.Cols()][0] = state[i][j];
		//从第二层开始
		for (int j = 1; j < critic_layers.size(); ++j)
		{
			critic_layers[j].z = critic_layers[j].w * critic_layers[j - 1].output + critic_layers[j].b;  // 计算Z[l]
			critic_layers[j].output = critic_layers[j].z.Apply(critic_layers[j].activation, true);
		}
		//反向传播
		//计算输出层的误差项 δ[L] = f'(Z[L])
		critic_layers.back().delta = critic_layers.back().z.Apply(critic_layers.back().activation_derivative, true);
		//计算隐藏层的误差项 δ[L-1], δ[L-2], ..., δ[1]
		for (int l = critic_layers.size() - 2; l >= 1; --l)
		{
			critic_layers[l].delta = (critic_layers[l + 1].w.T() * critic_layers[l + 1].delta).HadamardProduct(critic_layers[l].z.Apply(critic_layers[l].activation_derivative, true));
		}
		//计算Actor最后一层的误差项 
		//裁剪Critic网络第一个隐藏层的权值矩阵
		RbsLib::Math::Matrix<float> critic_w(critic_layers[1].w.Rows(), action.Cols());
		actor_layers.back().delta = (critic_w.T() * critic_layers[1].delta).HadamardProduct(actor_layers.back().z.Apply(actor_layers.back().activation_derivative, true));
		//继续反向传播
		for (int l = actor_layers.size() - 2; l >= 1; --l)
		{
			actor_layers[l].delta = (actor_layers[l + 1].w.T() * actor_layers[l + 1].delta).HadamardProduct(actor_layers[l].z.Apply(actor_layers[l].activation_derivative, true));
		}
		//更新权重和偏置，梯度上升
		// W[l] = W[l] + learning_rate * (δ[l] * A[l-1]^T)
		for (int l = 1; l < actor_layers.size(); ++l)
		{
			actor_layers[l].w = actor_layers[l].w + actor_learning_rate * actor_layers[l].delta * actor_layers[l - 1].output.T();
			actor_layers[l].b = actor_layers[l].b + actor_learning_rate * actor_layers[l].delta;
		}
		//更新target网络
		for (int l = 1; l < target_actor_layers.size(); ++l)
		{
			target_actor_layers[l].w = tau * actor_layers[l].w + (1 - tau) * target_actor_layers[l].w;
			target_actor_layers[l].b = tau * actor_layers[l].b + (1 - tau) * target_actor_layers[l].b;
		}
		for (int l = 1; l < target_critic_layers.size(); ++l)
		{
			target_critic_layers[l].w = tau * critic_layers[l].w + (1 - tau) * target_critic_layers[l].w;
			target_critic_layers[l].b = tau * critic_layers[l].b + (1 - tau) * target_critic_layers[l].b;
		}
    }
}

RbsLib::Math::Matrix<float> RbsLib::MatchingLearning::DDPG::PredictTrain(const RbsLib::Math::Matrix<float>& state, float sigma)
{
    //检查输入输出是否匹配
    if (state.Cols() != target_actor_layers[0].output.Rows())
    {
        throw std::invalid_argument("输入数据列数与输入层神经元个数不匹配");
    }
    //传入state，输出action
    // 第 1 层的输出就是输入
    for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
        target_actor_layers[0].output[j][0] = state[0][j];
    //从第二层开始
    for (int j = 1; j < target_actor_layers.size(); ++j)
    {
        target_actor_layers[j].output = 
            (target_actor_layers[j].w * target_actor_layers[j - 1].output + target_actor_layers[j].b)
            .Apply(target_actor_layers[j].activation, true);
    }

    //为输出每一部分加上噪声
    for (int j = 0; j < target_actor_layers.back().output.Rows(); ++j)
    {
        target_actor_layers.back().output[j][0] += GaussianNoise(0, sigma);
    }
    return target_actor_layers.back().output.T();
}

RbsLib::Math::Matrix<float> RbsLib::MatchingLearning::DDPG::Predict(const RbsLib::Math::Matrix<float>& state)
{
	//检查输入输出是否匹配
	if (state.Cols() != target_actor_layers[0].output.Rows())
	{
		throw std::invalid_argument("输入数据列数与输入层神经元个数不匹配");
	}
	//传入state，输出action
	// 第 1 层的输出就是输入
	for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
		target_actor_layers[0].output[j][0] = state[0][j];
	//从第二层开始
	for (int j = 1; j < target_actor_layers.size(); ++j)
	{
		target_actor_layers[j].output =
			(target_actor_layers[j].w * target_actor_layers[j - 1].output + target_actor_layers[j].b)
			.Apply(target_actor_layers[j].activation, true);
	}
	return target_actor_layers.back().output.T();
}


