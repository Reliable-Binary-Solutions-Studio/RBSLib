#pragma once
#include <vector>
#include "Math.h"
#include <functional>
#include "Storage.h"
#include "FileIO.h"
#include "MatchingLearningBase.h"


namespace RbsLib::MatchingLearning 
{
	class NeuralNetworks
	{
	private:
		
		std::vector<RbsLib::MatchingLearning::Layer> layers;
	public:
		NeuralNetworks(std::vector<int> layers, std::vector<std::function<float(float)>> activation, std::vector<std::function<float(float)>> activation_derivative, int random_seed);//参数表示每层神经元的个数
		void Train(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback);
		void TrainMultiThread(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback);
		void TrainCUDA(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback, int activite_func_index);
		auto Predict(RbsLib::Math::Matrix<float> inputs) -> RbsLib::Math::Matrix<float>;
		void Save(const RbsLib::Storage::StorageFile& path);//将模型存储到文件，但不会保存神经网络结构和激活函数等信息
		void Load(const RbsLib::Storage::StorageFile& path);
	};

	class DDPG
	{
	private:
		std::vector<RbsLib::MatchingLearning::Layer> actor_layers;
		std::vector<RbsLib::MatchingLearning::Layer> critic_layers;
		std::vector<RbsLib::MatchingLearning::Layer> target_actor_layers;
		std::vector<RbsLib::MatchingLearning::Layer> target_critic_layers;
		std::vector<std::function<float(float)>> actor_activation;
		std::vector<std::function<float(float)>> critic_activation;
		std::vector<std::function<float(float)>> actor_activation_derivative;
		std::vector<std::function<float(float)>> critic_activation_derivative;
		std::vector<float> actor_max;
		std::vector<float> actor_min;
		float gamma;//折扣因子
		float tau;//软更新参数
		float actor_learning_rate;
		float critic_learning_rate;
		int random_seed;
		static float GaussianNoise(float mu, float sigma);
	public:
		class ReplayBuffer
		{
		private:
			std::vector<RbsLib::Math::Matrix<float>> states;
			std::vector<RbsLib::Math::Matrix<float>> actions;
			std::vector<float> rewards;
			std::vector<RbsLib::Math::Matrix<float>> next_states;
			std::vector<bool> dones;
			size_t max_size;
			size_t current_size;
		public:
			ReplayBuffer(size_t max_size);
			void Add(const RbsLib::Math::Matrix<float>& state, const RbsLib::Math::Matrix<float>& action, float reward, const RbsLib::Math::Matrix<float>& next_state, bool done);
			std::tuple<RbsLib::Math::Matrix<float>, RbsLib::Math::Matrix<float>, std::vector<float>, RbsLib::Math::Matrix<float>, std::vector<bool>> Sample(size_t batch_size, int random_seed);
			size_t CurrentSize();
			size_t MaxSize();
			void Clear();
		};
		ReplayBuffer replay_buffer;


		/*
		* @brief 构造函数请确保actor网络输出层的激活函数是tanh函数(输出范围可控)，critic网络输出层的激活函数是线性函数
		*/
		DDPG(std::vector<int> actor_layers, std::vector<int> critic_layers,
			std::vector<std::function<float(float)>> actor_activation, 
			std::vector<std::function<float(float)>> critic_activation, 
			std::vector<std::function<float(float)>> actor_activation_derivative,
			std::vector<std::function<float(float)>> critic_activation_derivative,
			std::vector<float> actor_max, std::vector<float> actor_min,//对应每个动作的最大值和最小值
			float gamma, float tau, float actor_learning_rate, 
			float critic_learning_rate, int random_seed, size_t replay_buffer_size);

		/*
		* @brief 用于训练DDPG模型
		* @param batch_size 从经验回放中取出的数据的数量
		* @return void
		*/
		void Train(size_t batch_size);
		/*
		* @brief 用于训练DDPG模型时预测动作，会加入噪声，需要确保传入参数已归一化(建议-1~1)
		* @param state 状态
		* @param sigma 噪声的标准差
		* @return 动作,输出为行向量，输出范围取决于最后一层的激活函数
		*/
		RbsLib::Math::Matrix<float> PredictTrain(const RbsLib::Math::Matrix<float>& state,float sigma);
        /*
        * @brief 用于预测动作
		* @param state 状态
		* @return 动作,输出为行向量，输出范围取决于最后一层的激活函数
        */
        RbsLib::Math::Matrix<float> Predict(const RbsLib::Math::Matrix<float>& state);
	};

    template<typename T>
    class Normalization
    {
    private:
        struct MaxMinPack
        {
            float max;
            float min;
        };
        std::vector<MaxMinPack> MaxMin;

        float target_min = 0.0f;
        float target_max = 1.0f;

    public:
        // 设置输入数据的最大值和最小值
        void SetBounds(const std::vector<std::pair<float, float>>& bounds)
        {
            MaxMin.resize(bounds.size());
            for (size_t i = 0; i < bounds.size(); ++i)
            {
                MaxMin[i].max = bounds[i].first;
                MaxMin[i].min = bounds[i].second;
            }
        }

        // 设置目标归一化区间
        void SetTargetRange(float t_min, float t_max)
        {
            target_min = t_min;
            target_max = t_max;
        }

		void Fit(const RbsLib::Math::Matrix<T>& data)
		{
			MaxMin.resize(data.Cols());
			for (size_t i = 0; i < data.Cols(); ++i)
			{
				MaxMin[i].max = data[0][i];
				MaxMin[i].min = data[0][i];
				for (size_t j = 1; j < data.Rows(); ++j)
				{
					if (data[j][i] > MaxMin[i].max) MaxMin[i].max = data[j][i];
					if (data[j][i] < MaxMin[i].min) MaxMin[i].min = data[j][i];
				}
			}
		}

        // 数据归一化到指定区间
        RbsLib::Math::Matrix<T> Normalize(const RbsLib::Math::Matrix<T>& data)
        {
            RbsLib::Math::Matrix<T> result(data.Rows(), data.Cols());
            for (size_t i = 0; i < data.Cols(); ++i)
            {
                for (size_t j = 0; j < data.Rows(); ++j)
                {
                    float range_input = MaxMin[i].max - MaxMin[i].min;
                    float range_target = target_max - target_min;
                    result[j][i] = ((data[j][i] - MaxMin[i].min) / range_input) * range_target + target_min;
                }
            }
            return result;
        }

        // 数据反归一化到原始范围
        RbsLib::Math::Matrix<T> Denormalize(const RbsLib::Math::Matrix<T>& data)
        {
            RbsLib::Math::Matrix<T> result(data.Rows(), data.Cols());
            for (size_t i = 0; i < data.Cols(); ++i)
            {
                for (size_t j = 0; j < data.Rows(); ++j)
                {
                    float range_input = MaxMin[i].max - MaxMin[i].min;
                    float range_target = target_max - target_min;
                    result[j][i] = ((data[j][i] - target_min) / range_target) * range_input + MaxMin[i].min;
                }
            }
            return result;
        }

        // 支持就地归一化
        RbsLib::Math::Matrix<T>& Normalize(RbsLib::Math::Matrix<T>& data, bool)
        {
            for (size_t i = 0; i < data.Cols(); ++i)
            {
                for (size_t j = 0; j < data.Rows(); ++j)
                {
                    float range_input = MaxMin[i].max - MaxMin[i].min;
                    float range_target = target_max - target_min;
                    data[j][i] = ((data[j][i] - MaxMin[i].min) / range_input) * range_target + target_min;
                }
            }
            return data;
        }

        // 支持就地反归一化
        RbsLib::Math::Matrix<T>& Denormalize(RbsLib::Math::Matrix<T>& data, bool)
        {
            for (size_t i = 0; i < data.Cols(); ++i)
            {
                for (size_t j = 0; j < data.Rows(); ++j)
                {
                    float range_input = MaxMin[i].max - MaxMin[i].min;
                    float range_target = target_max - target_min;
                    data[j][i] = ((data[j][i] - target_min) / range_target) * range_input + MaxMin[i].min;
                }
            }
            return data;
        }

        // 保存最大最小值
        void Save(const std::string& path)
        {
            auto fp = RbsLib::Storage::StorageFile(path).Open(RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Bin,
                RbsLib::Storage::FileIO::SeekBase::begin,
                0);
            for (const auto& mm : MaxMin)
            {
                fp.WriteData<T>(mm.max);
                fp.WriteData<T>(mm.min);
            }
        }

        // 加载最大最小值
        void Load(const std::string& path)
        {
            auto fp = RbsLib::Storage::StorageFile(path).Open(RbsLib::Storage::FileIO::OpenMode::Read | RbsLib::Storage::FileIO::OpenMode::Bin,
                RbsLib::Storage::FileIO::SeekBase::begin,
                0);
            MaxMin.resize(RbsLib::Storage::StorageFile(path).GetFileSize() / (sizeof(T) * 2));
            if (MaxMin.size() == 0) throw std::invalid_argument("File size is 0");
            for (size_t i = 0; i < MaxMin.size(); ++i)
            {
                fp.GetData<T>(MaxMin[i].max);
                fp.GetData<T>(MaxMin[i].min);
            }
        }
    };

}