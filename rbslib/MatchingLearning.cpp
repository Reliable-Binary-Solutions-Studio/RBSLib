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

    // ȷ��������͵����������������ƥ��
    if (activation.size() != layers.size() - 1 || activation_derivative.size() != layers.size() - 1)
    {
        throw std::invalid_argument("������͵��������������������ƥ��");
    }

    // ��ʼ��ÿһ��
    for (size_t i = 0; i < layers.size(); i++)
    {
        // ��һ����������㣬����ҪȨ��
        if (i == 0)
        {
            this->layers[i].w = RbsLib::Math::Matrix<float>(layers[i], 1); // �����û��Ȩ��
        }
        else
        {
            // �������ز������㣬ʹ�� Xavier ��ʼ��
            float limit = std::sqrt(6.0 / (layers[i - 1] + layers[i]));
            this->layers[i].w = RbsLib::Math::Matrix<float>(layers[i], layers[i - 1]);

            // Xavier ��ʼ�������ѡ���� [-limit, limit] ��Χ�ڵ�ֵ
            for (int row = 0; row < this->layers[i].w.Rows(); ++row)
            {
                for (int col = 0; col < this->layers[i].w.Cols(); ++col)
                {
                    float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] ��Χ�������
                    this->layers[i].w[row][col] = rand_val;
                }
            }
            // ���ü�����͵���
            this->layers[i].activation = activation[i-1];
            this->layers[i].activation_derivative = activation_derivative[i-1];
        }

        // ƫ�ó�ʼ��Ϊ��
        this->layers[i].b = RbsLib::Math::Matrix<float>(layers[i], 1);

        // ��ʼ�������delta����
        this->layers[i].output = RbsLib::Math::Matrix<float>(layers[i], 1);
        this->layers[i].delta = RbsLib::Math::Matrix<float>(layers[i], 1);


        //��ʼ��Z����
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
    //�ȼ����������Ƿ�ƥ��
    if (inputs.Rows() != target.Rows())
    {
        throw std::invalid_argument("�����������������ƥ��");
    }
    if (inputs.Cols() != layers[0].w.Rows())
    {
        throw std::invalid_argument("���������������������Ԫ������ƥ��");
    }
    if (target.Cols() != layers.back().w.Rows())
    {
        throw std::invalid_argument("��������������������Ԫ������ƥ��");
    }

    // ��ʼѵ��
    for (int e = 0; e < epochs; ++e)
    {
        // �� e ��
        float total_loss = 0.0;  // �����ۼ���ʧ
        for (int i = 0; i < inputs.Rows(); ++i)
        {
            int index = i; rand() % inputs.Rows();  // ���ѡ��һ������

            // ǰ�򴫲�
            // �� 1 ��������������
            for (int j = 0; j < layers[0].output.Rows(); ++j)
            {
                layers[0].output[j][0] = inputs[index][j];
            }

            // �ӵڶ��㿪ʼ
            for (int j = 1; j < layers.size(); ++j)
            {
                // ���㵱ǰ������ Z = W * A + b
                layers[j].z = layers[j].w * layers[j - 1].output + layers[j].b;  // ����Z[l]

                // Ӧ�ü����
                for (int k = 0; k < layers[j].output.Rows(); ++k)
                {
                    layers[j].output[k][0] = layers[j].activation(layers[j].z[k][0]);  // ʹ�ü�����������
                }
            }

            // ������ʧ��������
            float loss = 0.0;
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                float error = target[index][j] - layers.back().output[j][0]; // Ԥ�����
                loss += 0.5 * error * error; // MSE��ʧ
            }
            total_loss += loss;
         

            // ���򴫲�
			// ��������������� ��[L] = (A[L] - Y) * f'(Z[L])
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                float error = layers.back().output[j][0] - target[index][j];
                layers.back().delta[j][0] = error * layers.back().activation_derivative(layers.back().z[j][0]);  // ʹ�ü��������
            }

            // �������ز������� ��[L-1], ��[L-2], ..., ��[1]
            for (int l = layers.size() - 2; l >= 1; --l)
            {
                for (int j = 0; j < layers[l].delta.Rows(); ++j)
                {
                    float error_sum = 0.0;
                    for (int k = 0; k < layers[l + 1].delta.Rows(); ++k)
                    {
                        error_sum += layers[l + 1].delta[k][0] * layers[l + 1].w[k][j];
                    }
                    layers[l].delta[j][0] = error_sum * layers[l].activation_derivative(layers[l].z[j][0]);  // ʹ�ü��������
                }
            }
            // ����Ȩ�غ�ƫ��
            for (int l = 1; l < layers.size(); ++l)
            {
                // Ȩ�ظ��£� W[l] = W[l] - learning_rate * (��[l] * A[l-1]^T)
                
				layers[l].w = layers[l].w - learning_rate * layers[l].delta * layers[l - 1].output.T();


                // ƫ�ø��£� b[l] = b[l] - learning_rate * ��[l]
				layers[l].b = layers[l].b - learning_rate * layers[l].delta;
            }
        }
        // ���ÿ�ֵ���ʧ
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
    //�����������Ƿ�ƥ��
    if (inputs.Cols() != layers[0].w.Rows())
    {
        throw std::invalid_argument("���������������������Ԫ������ƥ��");
    }

    //������ǰģ��״̬
    auto temp_layers = layers;

    //��������������
    RbsLib::Math::Matrix<float> result(inputs.Rows(), layers.back().output.Rows());

    //���ÿ�����ģ��״̬����Ԥ��
    for (int i = 0; i < inputs.Rows(); ++i)
    {
        // �� 1 ��������������
        for (int j = 0; j < temp_layers[0].output.Rows(); ++j)
        {
            temp_layers[0].output[j][0] = inputs[i][j];
        }

        // �ӵڶ��㿪ʼ
        for (int j = 1; j < temp_layers.size(); ++j)
        {
            // ���㵱ǰ������ Z = W * A + b
            temp_layers[j].z = temp_layers[j].w * temp_layers[j - 1].output + temp_layers[j].b;  // ����Z[l]

            // Ӧ�ü����
            for (int k = 0; k < temp_layers[j].output.Rows(); ++k)
            {
                temp_layers[j].output[k][0] = temp_layers[j].activation(temp_layers[j].z[k][0]);  // ʹ�ü�����������
            }
        }

        // �洢���
		for (int k = 0; k < temp_layers.back().output.Rows(); ++k)
		{
			result[i][k] = temp_layers.back().output[k][0];
		}
    }
    return result;
}

void RbsLib::MatchingLearning::NeuralNetworks::Save(const RbsLib::Storage::StorageFile& path)
{
	//����ģ��
	auto fp = path.Open(RbsLib::Storage::FileIO::OpenMode::Write|RbsLib::Storage::FileIO::OpenMode::Bin,
        RbsLib::Storage::FileIO::SeekBase::begin,
        0);
	for (const auto& layer : layers)
	{
		//�洢w
		for (int i = 0; i < layer.w.Rows(); ++i)
		{
			for (int j = 0; j < layer.w.Cols(); ++j)
			{
				fp.WriteData<float>(layer.w[i][j]);
			}
		}
		//�洢b
		for (int i = 0; i < layer.b.Rows(); ++i)
		{
			fp.WriteData<float>(layer.b[i][0]);
		}
	}
}

void RbsLib::MatchingLearning::NeuralNetworks::Load(const RbsLib::Storage::StorageFile& path)
{
	//����ģ��
	auto fp = path.Open(RbsLib::Storage::FileIO::OpenMode::Read | RbsLib::Storage::FileIO::OpenMode::Bin,
		RbsLib::Storage::FileIO::SeekBase::begin,
		0);
	for (auto& layer : layers)
	{
		//����w
		for (int i = 0; i < layer.w.Rows(); ++i)
		{
			for (int j = 0; j < layer.w.Cols(); ++j)
			{
				fp.GetData<float>(layer.w[i][j]);
			}
		}
		//����b
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
	//��Ӿ���
	states[current_size] = state;
	actions[current_size] = action;
	rewards[current_size] = reward;
	next_states[current_size] = next_state;
	dones[current_size] = done;

	//���µ�ǰ��С
    current_size += 1;
}

std::tuple<RbsLib::Math::Matrix<float>, RbsLib::Math::Matrix<float>, std::vector<float>, RbsLib::Math::Matrix<float>, std::vector<bool>>  RbsLib::MatchingLearning::DDPG::ReplayBuffer::Sample(size_t batch_size,int random_state)
{
	if (current_size < batch_size)
	{
		throw std::runtime_error("Replay buffer size less than batch size");
	}
	//���ѡ��batch_size������
	std::vector<int> batched_index(batch_size);
	srand(random_state);
	for (size_t i = 0; i < batch_size; i++)
	{
		batched_index[i] = rand() % current_size;
	}
	//���췵��ֵ
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
	//���actor��critic��������Ƿ�ƥ��
	if (actor_layers.size() != actor_activation.size() + 1 || 
        actor_layers.size() != actor_activation_derivative.size() + 1)
		throw std::invalid_argument("actor��������뼤���������ƥ��");
	if (critic_layers.size() != critic_activation.size() + 1 ||
		critic_layers.size() != critic_activation_derivative.size() + 1)
		throw std::invalid_argument("critic��������뼤���������ƥ��");
	//���actor��critic��������Ƿ�ƥ��
    if (actor_layers.back()+actor_layers.front()!=critic_layers.front())
		throw std::invalid_argument("actor��critic�������������ƥ��");



    //���γ�ʼ��ÿ������ľ���
	//��ʼ��actor����
	srand(random_seed);
    for (int i = 0; i < this->actor_layers.size(); ++i)
    {
        if (i == 0)
        {
            this->actor_layers[i].w = RbsLib::Math::Matrix<float>(actor_layers[i], 1); // �����û��Ȩ��
        }
        else
        {
            // �������ز������㣬ʹ�� Xavier ��ʼ��
            float limit = std::sqrt(6.0 / (actor_layers[i - 1] + actor_layers[i]));
            this->actor_layers[i].w = RbsLib::Math::Matrix<float>(actor_layers[i], actor_layers[i - 1]);

            // Xavier ��ʼ�������ѡ���� [-limit, limit] ��Χ�ڵ�ֵ
            for (int row = 0; row < this->actor_layers[i].w.Rows(); ++row)
            {
                for (int col = 0; col < this->actor_layers[i].w.Cols(); ++col)
                {
                    float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] ��Χ�������
                    this->actor_layers[i].w[row][col] = rand_val;
                }
            }
            // ���ü�����͵���
            this->actor_layers[i].activation = actor_activation[i - 1];
            this->actor_layers[i].activation_derivative = actor_activation_derivative[i - 1];
        }

        // ƫ�ó�ʼ��Ϊ��
        this->actor_layers[i].b = RbsLib::Math::Matrix<float>(actor_layers[i], 1);

        // ��ʼ�������delta����
        this->actor_layers[i].output = RbsLib::Math::Matrix<float>(actor_layers[i], 1);
        this->actor_layers[i].delta = RbsLib::Math::Matrix<float>(actor_layers[i], 1);

        //��ʼ��Z����
		this->actor_layers[i].z = RbsLib::Math::Matrix<float>(actor_layers[i], 1);
    }
    //������actor_target����
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
	//��ʼ��critic����
	for (int i = 0; i < this->critic_layers.size(); ++i)
	{
		if (i == 0)
		{
			this->critic_layers[i].w = RbsLib::Math::Matrix<float>(critic_layers[i], 1); // �����û��Ȩ��
		}
		else
		{
			// �������ز������㣬ʹ�� Xavier ��ʼ��
			float limit = std::sqrt(6.0 / (critic_layers[i - 1] + critic_layers[i]));
			this->critic_layers[i].w = RbsLib::Math::Matrix<float>(critic_layers[i], critic_layers[i - 1]);

			// Xavier ��ʼ�������ѡ���� [-limit, limit] ��Χ�ڵ�ֵ
			for (int row = 0; row < this->critic_layers[i].w.Rows(); ++row)
			{
				for (int col = 0; col < this->critic_layers[i].w.Cols(); ++col)
				{
					float rand_val = ((float)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] ��Χ�������
					this->critic_layers[i].w[row][col] = rand_val;
				}
			}
			// ���ü�����͵���
			this->critic_layers[i].activation = critic_activation[i - 1];
			this->critic_layers[i].activation_derivative = critic_activation_derivative[i - 1];
		}

		// ƫ�ó�ʼ��Ϊ��
		this->critic_layers[i].b = RbsLib::Math::Matrix<float>(critic_layers[i], 1);

		// ��ʼ�������delta����
		this->critic_layers[i].output = RbsLib::Math::Matrix<float>(critic_layers[i], 1);
		this->critic_layers[i].delta = RbsLib::Math::Matrix<float>(critic_layers[i], 1);

		//��ʼ��Z����
		this->critic_layers[i].z = RbsLib::Math::Matrix<float>(critic_layers[i], 1);
	}
	//������critic_target����
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
	//��replay_buffer���������batch_size������
	auto [state, action, reward, next_state, done] = replay_buffer.Sample(batch_size, random_seed);
	//���ɼ���������ά���Ƿ�������ƥ��
	if (state.Cols() != actor_layers[0].w.Rows()||state.Cols()+action.Cols()!=critic_layers[0].w.Rows())
		throw std::invalid_argument("�ɼ���������ά�����������Ԫ������ƥ��");

    for (int i = 0; i < state.Rows(); ++i)
    {
        //����yi
		//��actor_target��ǰ����
		// �� 1 ��������������
		for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
			target_actor_layers[0].output[j][0] = next_state[i][j];
		//�ӵڶ��㿪ʼ
        for (int j = 1; j < target_actor_layers.size(); ++j)
        {
            //target���粻��Ҫ����ѵ�������Բ���Ҫ����Z
            target_actor_layers[i].output = target_actor_layers[i].w * target_actor_layers[i - 1].output + target_actor_layers[i].b;  // ����Z[l]
			target_actor_layers[i].output.Apply(target_actor_layers[i].activation);
        }
		//ƴ��������ΪQ��������룬A��ǰ��S�ں�ֱ����ΪCritic����������output
		for (int j = 0; j < target_actor_layers.back().output.Rows(); ++j)
			target_critic_layers[0].output[j][0] = target_actor_layers.back().output[j][0];
		for (int j = 0; j < state.Cols(); ++j)
			target_critic_layers[0].output[j + target_actor_layers.back().output.Rows()][0] = next_state[i][j];
		//�ӵڶ��㿪ʼ
        for (int j = 1; j < target_critic_layers.size(); ++j)
        {
			target_critic_layers[j].z = target_critic_layers[j].w * target_critic_layers[j - 1].output + target_critic_layers[j].b;  // ����Z[l]
			target_critic_layers[j].output = target_critic_layers[j].z;
        }
        //����yi�������target_critic��output��
        //��reward��չΪ��������ÿһ�=reward[i]
        RbsLib::Math::Matrix<float> reward_vector(target_critic_layers.back().output.Rows(), 1);
		for (int j = 0; j < reward_vector.Rows(); ++j) reward_vector[j][0] = reward[i];
        target_critic_layers.back().output = reward_vector + (1 - done[i]) * gamma * target_critic_layers.back().output;

		//����critic����
		// �� 1 �������������룬A��ǰ��S�ں�
		for (int j = 0; j < action.Cols(); ++j)
			critic_layers[0].output[j][0] = action[i][j];
		for (int j = 0; j < state.Cols(); ++j)
			critic_layers[0].output[j + action.Cols()][0] = state[i][j];
		//�ӵڶ��㿪ʼ
        for (int j = 1; j < critic_layers.size(); ++j)
        {
			critic_layers[j].z = critic_layers[j].w * critic_layers[j - 1].output + critic_layers[j].b;  // ����Z[l]
			critic_layers[j].output = critic_layers[j].z;//������output
			critic_layers[j].output.Apply(critic_layers[j].activation);//Ӧ�ü����
        }
		//����loss,�������˲�����
        //���򴫲�

		//���һ�������� ��[L] = (Y - A[L]) .* f'(Z[L])
        critic_layers.back().delta = (target_critic_layers.back().output - critic_layers.back().output).HadamardProduct(critic_layers.back().z.Apply(critic_layers.back().activation_derivative, true));
		//�������ز������� ��[L-1], ��[L-2], ..., ��[1]
        //��[L] = W[L + 1] ^ T * ��[L + 1].*f'(Z[L])
		for (int l = critic_layers.size() - 2; l >= 1; --l)
		{
			critic_layers[l].delta = (critic_layers[l + 1].w.T() * critic_layers[l + 1].delta).HadamardProduct(critic_layers[l].z.Apply(critic_layers[l].activation_derivative, true));
		}
		//����Ȩ�غ�ƫ��,�ݶ��½�
		// W[l] = W[l] - learning_rate * (��[l] * A[l-1]^T)
		for (int l = 1; l < critic_layers.size(); ++l)
		{
			critic_layers[l].w = critic_layers[l].w - critic_learning_rate * critic_layers[l].delta * critic_layers[l - 1].output.T();
			critic_layers[l].b = critic_layers[l].b - critic_learning_rate * critic_layers[l].delta;
		}

		//����actor����
		// �� 1 ��������������
		for (int j = 0; j < actor_layers[0].output.Rows(); ++j)
			actor_layers[0].output[j][0] = state[i][j];
		//�ӵڶ��㿪ʼ
		for (int j = 1; j < actor_layers.size(); ++j)
		{
			actor_layers[j].z = actor_layers[j].w * actor_layers[j - 1].output + actor_layers[j].b;  // ����Z[l]
			actor_layers[j].output = actor_layers[j].z.Apply(actor_layers[j].activation, true);
		}
		//������Critic�����м���Qֵ
		// �� 1 �������������룬A��ǰ��S�ں�
		for (int j = 0; j < action.Cols(); ++j)
			critic_layers[0].output[j][0] = actor_layers.back().output[j][0];
		for (int j = 0; j < state.Cols(); ++j)
			critic_layers[0].output[j + action.Cols()][0] = state[i][j];
		//�ӵڶ��㿪ʼ
		for (int j = 1; j < critic_layers.size(); ++j)
		{
			critic_layers[j].z = critic_layers[j].w * critic_layers[j - 1].output + critic_layers[j].b;  // ����Z[l]
			critic_layers[j].output = critic_layers[j].z.Apply(critic_layers[j].activation, true);
		}
		//���򴫲�
		//��������������� ��[L] = f'(Z[L])
		critic_layers.back().delta = critic_layers.back().z.Apply(critic_layers.back().activation_derivative, true);
		//�������ز������� ��[L-1], ��[L-2], ..., ��[1]
		for (int l = critic_layers.size() - 2; l >= 1; --l)
		{
			critic_layers[l].delta = (critic_layers[l + 1].w.T() * critic_layers[l + 1].delta).HadamardProduct(critic_layers[l].z.Apply(critic_layers[l].activation_derivative, true));
		}
		//����Actor���һ�������� 
		//�ü�Critic�����һ�����ز��Ȩֵ����
		RbsLib::Math::Matrix<float> critic_w(critic_layers[1].w.Rows(), action.Cols());
		actor_layers.back().delta = (critic_w.T() * critic_layers[1].delta).HadamardProduct(actor_layers.back().z.Apply(actor_layers.back().activation_derivative, true));
		//�������򴫲�
		for (int l = actor_layers.size() - 2; l >= 1; --l)
		{
			actor_layers[l].delta = (actor_layers[l + 1].w.T() * actor_layers[l + 1].delta).HadamardProduct(actor_layers[l].z.Apply(actor_layers[l].activation_derivative, true));
		}
		//����Ȩ�غ�ƫ�ã��ݶ�����
		// W[l] = W[l] + learning_rate * (��[l] * A[l-1]^T)
		for (int l = 1; l < actor_layers.size(); ++l)
		{
			actor_layers[l].w = actor_layers[l].w + actor_learning_rate * actor_layers[l].delta * actor_layers[l - 1].output.T();
			actor_layers[l].b = actor_layers[l].b + actor_learning_rate * actor_layers[l].delta;
		}
		//����target����
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
    //�����������Ƿ�ƥ��
    if (state.Cols() != target_actor_layers[0].output.Rows())
    {
        throw std::invalid_argument("���������������������Ԫ������ƥ��");
    }
    //����state�����action
    // �� 1 ��������������
    for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
        target_actor_layers[0].output[j][0] = state[0][j];
    //�ӵڶ��㿪ʼ
    for (int j = 1; j < target_actor_layers.size(); ++j)
    {
        target_actor_layers[j].output = 
            (target_actor_layers[j].w * target_actor_layers[j - 1].output + target_actor_layers[j].b)
            .Apply(target_actor_layers[j].activation, true);
    }

    //Ϊ���ÿһ���ּ�������
    for (int j = 0; j < target_actor_layers.back().output.Rows(); ++j)
    {
        target_actor_layers.back().output[j][0] += GaussianNoise(0, sigma);
    }
    return target_actor_layers.back().output.T();
}

RbsLib::Math::Matrix<float> RbsLib::MatchingLearning::DDPG::Predict(const RbsLib::Math::Matrix<float>& state)
{
	//�����������Ƿ�ƥ��
	if (state.Cols() != target_actor_layers[0].output.Rows())
	{
		throw std::invalid_argument("���������������������Ԫ������ƥ��");
	}
	//����state�����action
	// �� 1 ��������������
	for (int j = 0; j < target_actor_layers[0].output.Rows(); ++j)
		target_actor_layers[0].output[j][0] = state[0][j];
	//�ӵڶ��㿪ʼ
	for (int j = 1; j < target_actor_layers.size(); ++j)
	{
		target_actor_layers[j].output =
			(target_actor_layers[j].w * target_actor_layers[j - 1].output + target_actor_layers[j].b)
			.Apply(target_actor_layers[j].activation, true);
	}
	return target_actor_layers.back().output.T();
}


