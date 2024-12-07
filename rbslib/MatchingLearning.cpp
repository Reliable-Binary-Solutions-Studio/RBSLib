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
            // ��������������� ��[L]
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
        result[i] = temp_layers.back().output.T()[0];
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
