#include "MatchingLearning.h"
#include <cmath>

RbsLib::MatchingLearning::NeuralNetworks::NeuralNetworks(std::vector<int> layers, int random_seed)
    : layers(layers.size())
{
    srand(random_seed);
    for (size_t i = 0; i < layers.size(); i++)
    {
        // ��һ����������㣬����ҪȨ��
        if (i == 0)
        {
            this->layers[i].w = RbsLib::Math::Matrix<double>(layers[i], 1); // �����û��Ȩ��
        }
        else
        {
            // �������ز������㣬ʹ�� Xavier ��ʼ��
            double limit = std::sqrt(6.0 / (layers[i - 1] + layers[i]));
            this->layers[i].w = RbsLib::Math::Matrix<double>(layers[i], layers[i - 1]);

            // Xavier ��ʼ�������ѡ���� [-limit, limit] ��Χ�ڵ�ֵ
            for (int row = 0; row < this->layers[i].w.Rows(); ++row)
            {
                for (int col = 0; col < this->layers[i].w.Cols(); ++col)
                {
                    double rand_val = ((double)rand() / RAND_MAX) * 2.0 * limit - limit; // [-limit, limit] ��Χ�������
                    this->layers[i].w[row][col] = rand_val;
                }
            }
        }

        // ƫ�ó�ʼ��Ϊ��
        this->layers[i].b = RbsLib::Math::Matrix<double>(layers[i], 1);

        // ��ʼ�������delta����
        this->layers[i].output = RbsLib::Math::Matrix<double>(layers[i], 1);
        this->layers[i].delta = RbsLib::Math::Matrix<double>(layers[i], 1);
    }
}

void RbsLib::MatchingLearning::NeuralNetworks::Train(RbsLib::Math::Matrix<double> inputs, RbsLib::Math::Matrix<double> target, double learning_rate, int epochs, void (*loss_callback)(int,double))
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
        double total_loss = 0.0;  // �����ۼ���ʧ
        for (int i = 0; i < inputs.Rows(); ++i)
        {
            int index = i;//rand() % inputs.Rows();  // ���ѡ��һ������

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
                layers[j].output = layers[j].w * layers[j - 1].output + layers[j].b;

                // Ӧ�ü���� (Sigmoid)
                for (int k = 0; k < layers[j].output.Rows(); ++k)
                {
                    layers[j].output[k][0] = 1.0 / (1.0 + std::exp(-layers[j].output[k][0]));
                }
            }

            // ������ʧ������������ѧϰ����������
            double loss = 0.0;
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                double error = target[index][j] - layers.back().output[j][0]; // Ԥ�����
                loss += 0.5 * error * error; // MSE��ʧ
            }
			total_loss += loss;

            // ���򴫲�
            // ��������������� ��[L]
            for (int j = 0; j < layers.back().output.Rows(); ++j)
            {
                double error = layers.back().output[j][0] - target[index][j];
                layers.back().delta[j][0] = error * (layers.back().output[j][0] * (1 - layers.back().output[j][0])); // sigmoid ����
            }

            // �������ز������� ��[L-1], ��[L-2], ..., ��[1]
            for (int l = layers.size() - 2; l >= 1; --l)
            {
                for (int j = 0; j < layers[l].delta.Rows(); ++j)
                {
                    // �������� ��[l] = (W[l+1]^T * ��[l+1]) * sigmoid'(Z[l])
                    double error_sum = 0.0;
                    for (int k = 0; k < layers[l + 1].delta.Rows(); ++k)
                    {
                        error_sum += layers[l + 1].delta[k][0] * layers[l + 1].w[k][j];
                    }
                    layers[l].delta[j][0] = error_sum * layers[l].output[j][0] * (1 - layers[l].output[j][0]); // sigmoid ����
                }
            }

            // ����Ȩ�غ�ƫ��
            for (int l = 1; l < layers.size(); ++l)
            {
                // Ȩ�ظ��£� W[l] = W[l] - learning_rate * (��[l] * A[l-1]^T)
                for (int j = 0; j < layers[l].w.Rows(); ++j)
                {
                    for (int k = 0; k < layers[l].w.Cols(); ++k)
                    {
                        layers[l].w[j][k] -= learning_rate * layers[l].delta[j][0] * layers[l - 1].output[k][0];
                    }
                }

                // ƫ�ø��£� b[l] = b[l] - learning_rate * ��[l]
                for (int j = 0; j < layers[l].b.Rows(); ++j)
                {
                    layers[l].b[j][0] -= learning_rate * layers[l].delta[j][0];
                }
            }
        }
        // ���ÿ�ֵ���ʧ
		loss_callback(e, total_loss/inputs.Rows());
    }
}

auto RbsLib::MatchingLearning::NeuralNetworks::Predict(RbsLib::Math::Matrix<double> inputs) -> RbsLib::Math::Matrix<double>
{
	//�����������Ƿ�ƥ��
	if (inputs.Cols() != layers[0].w.Rows())
	{
		throw std::invalid_argument("���������������������Ԫ������ƥ��");
	}
    //������ǰģ��״̬
	auto temp_layers = layers;
    //��������������
	RbsLib::Math::Matrix<double> result(inputs.Rows(), layers.back().output.Rows());
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
			temp_layers[j].output = temp_layers[j].w * temp_layers[j - 1].output + temp_layers[j].b;

			// Ӧ�ü���� (Sigmoid)
			for (int k = 0; k < temp_layers[j].output.Rows(); ++k)
			{
				temp_layers[j].output[k][0] = 1.0 / (1.0 + std::exp(-temp_layers[j].output[k][0]));
			}
		}
        //�洢���
		result[i] = temp_layers.back().output.T()[0];
	}
    return result;
}
