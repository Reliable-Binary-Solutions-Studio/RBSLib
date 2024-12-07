#include "MatchingLearning.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <iostream>
struct LayerCUDA
{
	float* w;
	int w_rows;
	int w_cols;

	float* b;
	int b_rows;
	int b_cols;

	float* output;
	int output_rows;
	int output_cols;

	float* delta;
	int delta_rows;
	int delta_cols;

	float* z;
	int z_rows;
	int z_cols;

	int activation_func_index;//预先定义好的几个激活函数的下标

};

__global__ void matrix_add_kernel(const float* A, const float* B, float* C, int rows, int cols) 
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < rows && col < cols) {
        int index = row * cols + col;
        C[index] = A[index] + B[index];
    }
}

__global__ void matrix_sub_kernel(const float* A, const float* B, float* C, int rows, int cols)
{
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	int col = blockIdx.x * blockDim.x + threadIdx.x;

	if (row < rows && col < cols) {
		int index = row * cols + col;
		C[index] = A[index] - B[index];
	}
}

__global__ void matrix_multiply_kernel(const float* A, const float* B, float* C, int A_rows, int A_cols, int B_cols) 
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < A_rows && col < B_cols) {
        float sum = 0.0;
        for (int k = 0; k < A_cols; ++k) {
            sum += A[row * A_cols + k] * B[k * B_cols + col];
        }
        C[row * B_cols + col] = sum;
    }
}

__global__ void scalar_multiply_kernel(const float* A, float scalar, float* B, int rows, int cols) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < rows && col < cols) {
        int index = row * cols + col;
        B[index] = scalar * A[index];
    }
}

__global__ void matrix_transpose_kernel(const float* A, float* B, int rows, int cols) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < rows && col < cols) {
        // 原矩阵 A 的位置 (row, col) 在转置矩阵 B 中变为 (col, row)
        B[col * rows + row] = A[row * cols + col];
    }
}


__device__ float sigmoid(float x) {
    return 1.0 / (1.0 + exp(-x));
}

__device__ float sigmoid_derivative(float x) {
    float sig = sigmoid(x);
    return sig * (1.0 - sig);
}

__global__ void apply_function_kernel(float* matrix, int rows, int cols, int activation_func_index) 
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < rows && col < cols) {
        int index = row * cols + col;

        if (activation_func_index == 1) {
            // 应用 Sigmoid 激活函数
            matrix[index] = sigmoid(matrix[index]);
        }
        else if (activation_func_index==-1)
        {
            //对应的导数
			matrix[index] = sigmoid_derivative(matrix[index]);
        }
        // 可以在此处添加更多的激活函数，例如 Tanh、ReLU 等
    }
}

__global__ void transpose_multiply_kernel(
    const float* X,       // 输入矩阵 X (m x n)
    const float* Y,       // 输入矩阵 Y (n x p)
    float* Z,             // 输出矩阵 Z (n x p)
    int m,                 // X 的行数
    int n,                 // X 的列数，Y 的行数
    int p                  // Y 的列数
) {
    // 计算 Z 矩阵的元素位置 (i, j)
    int i = blockIdx.x * blockDim.x + threadIdx.x;  // i 表示 Z 的行
    int j = blockIdx.y * blockDim.y + threadIdx.y;  // j 表示 Z 的列

    if (i < n && j < p) {
        float sum = 0.0;
        for (int k = 0; k < m; ++k) {
			//sum += X[k][i] * Y[k][j];
            sum += X[k * n + i] * Y[k * p + j];  // X^T * Y 的元素计算
        }
        Z[i * p + j] = sum;
    }
}


// 核函数：逐元素相乘
__global__ void matrix_elementwise_multiply_kernel(const float* A, const float* B, float* C, int rows, int cols) {
    // 计算全局线程 ID
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    // 计算对应元素
    if (row < rows && col < cols) {
        int index = row * cols + col; // 计算矩阵元素的线性索引
        C[index] = A[index] * B[index];
    }
}

//前向传播 A[l] = W * A[l-1] + b
__global__ void forward_kernel(const float* W, const float* A, const float* b, float* Z, float* OUT, int W_rows, int W_cols, int act_index)
{
	//结果为单行或单列向量，直接展平计算
	int row = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < W_rows)
	{
		float sum = 0;
		for (int k = 0; k < W_cols; ++k)
		{
			sum += W[row * W_cols + k] * A[k];
		}
		Z[row] = sum + b[row];
		if (act_index == 1)
		{
			OUT[row] = sigmoid(Z[row]);
		}
	}
}

/*最后一层delta计算*/
__global__ void backward_last_kernel(const float* output, const float* target, float* delta, float* z, int rows, int act_index,const float* A)
{
	//均为单行或单列向量，直接展平计算
	int row = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < rows)
	{
		if (act_index == 1)
		{
			//delta[row] = (output[row] - target[row]) * sigmoid_derivative(z[row]);
			delta[row] = (output[row] - target[row]) * A[row] * (1 - A[row]);
		}
	}
}
//δ[L] = W[L + 1] ^ T * δ[L + 1].*f'(Z[L])
__global__ void backward_kernel(const float* W, const float* delta, float* z, float* next_delta, int W_rows, int W_cols, int act_index,const float* A)
{
	//目标矩阵为单列向量，直接x展平计算
	int row = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < W_cols)
	{
		float sum = 0;
		for (int k = 0; k < W_rows; ++k)
		{
			sum += W[k * W_cols + row] * delta[k];
		}
		if (act_index == 1)
		{
			//next_delta[row] = sum * sigmoid_derivative(z[row]);
			//f'(Z[row])可以用A[row]*(1-A[row])表示
			next_delta[row] = sum * A[row] * (1 - A[row]);
		}
	}
}
//W[l] = W[l] - η * δ[l] * A[l-1]^T
//b[l] = b[l] - η * δ[l]
__global__ void update_w_kernel(float* W, int W_row, int W_col, float learn, const float* delta, const float* AT)
{
	//二维数组计算，不展平
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	int col = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < W_row && col < W_col)
	{
		W[row * W_col + col] -= learn * delta[row] * AT[col];
	}
}

__global__ void update_b_kernel(float* b, int b_row, int b_col, float learn, const float* delta)
{
	//单列向量，直接展平计算
	int row = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < b_row)
	{
		b[row] -= learn * delta[row];
	}
}

__global__ void add_loss_kernel(float* A, float* target, float* loss, int rows)
{
	int row = blockIdx.x * blockDim.x + threadIdx.x;
	if (row < rows)
	{
		loss[row] += 0.5 * (A[row] - target[row]) * (A[row] - target[row]);
	}
}


void matrix_add(const float* A, const float* B, float* C, int rows, int cols)
{
	//已经确保A B C在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	matrix_add_kernel <<<grid, block >>> (A, B, C, rows, cols);
}

void matrix_sub(const float* A, const float* B, float* C, int rows, int cols)
{
	//已经确保A B C在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	matrix_sub_kernel <<<grid, block >>> (A, B, C, rows, cols);
}

void matrix_multiply(const float* A, const float* B, float* C, int A_rows, int A_cols, int B_cols)
{
    //已经确保A B C在GPU上
    dim3 block(32, 32);
    dim3 grid((B_cols + block.x - 1) / block.x, (A_rows + block.y - 1) / block.y);
    matrix_multiply_kernel <<<grid, block >>> (A, B, C, A_rows, A_cols, B_cols);
}

void scalar_multiply(const float* A, float scalar, float* B, int rows, int cols)
{
	//已经确保A B在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	scalar_multiply_kernel <<<grid, block >>> (A, scalar, B, rows, cols);
}

void matrix_transpose(const float* A, float* B, int rows, int cols)
{
	//已经确保A B在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	matrix_transpose_kernel <<<grid, block >>> (A, B, rows, cols);
}
void matrix_apply_function(float* matrix, int rows, int cols, int activation_func_index)
{
	//已经确保matrix在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	apply_function_kernel <<<grid, block >>> (matrix, rows, cols, activation_func_index);
}

void matrix_elementwise_multiply(const float* A, const float* B, float* C, int rows, int cols)
{
	//A B C已经在GPU上
	dim3 block(32, 32);
	dim3 grid((cols + block.x - 1) / block.x, (rows + block.y - 1) / block.y);
	matrix_elementwise_multiply_kernel <<<grid, block >>> (A, B, C, rows, cols);
}
//用于计算X^T*Y
void transpose_multiply(const float* X, const float* Y, float* Z, int m, int n, int p)
{
	//已经确保X Y Z在GPU上
	dim3 block(32, 32);
	dim3 grid((n + block.x - 1) / block.x, (p + block.y - 1) / block.y);
	transpose_multiply_kernel <<<grid, block >>> (X, Y, Z, m, n, p);
}

void forward(const float* W, const float* A, const float* b, float* Z, float* OUT, int rows, int cols, int act_index)
{
	//已经确保W A b Z OUT在GPU上
	forward_kernel <<<(rows + 1024 - 1) / 1024, 1024 >>> (W, A, b, Z, OUT, rows, cols, act_index);
}

void backward_last(const float* output, const float* target, float* delta, float* z, int rows, int act_index,const float* A)
{
	//已经确保output target delta z在GPU上
	backward_last_kernel <<<(rows + 1024 - 1) / 1024, 1024 >> > (output, target, delta, z, rows, act_index,A);
}

void backward(const float* W, const float* delta, float* z, float* next_delta, int W_rows, int W_cols, int act_index,const float* A)
{
	//已经确保W delta z next_delta在GPU上
	backward_kernel <<<(W_rows + 1024 - 1) / 1024, 1024 >>> (W, delta, z, next_delta, W_rows, W_cols,  act_index,A);
}

void update_w(float* W, int W_row, int W_col, float learn, const float* delta, const float* AT)
{
	//已经确保W delta AT在GPU上
	dim3 block(32, 32);
	dim3 grid((W_col + block.x - 1) / block.x, (W_row + block.y - 1) / block.y);
	update_w_kernel << <grid, block >> > (W, W_row, W_col, learn, delta, AT);
}

void update_b(float* b, int b_row, int b_col, float learn, const float* delta)
{
	//已经确保b delta在GPU上
	update_b_kernel <<<(b_row + 1024 - 1) / 1024, 1024 >>> (b, b_row, b_col, learn, delta);
}

void add_loss(float* A, float* target, float* loss, int rows)
{
	//已经确保A target loss在GPU上
	if (rows<=512)
		add_loss_kernel << <(rows + 512 - 1) / 512, 512 >> > (A, target, loss, rows);
	else 
		add_loss_kernel << <(rows + 1024 - 1) / 1024, 1024 >> > (A, target, loss, rows);
}

void __PrintGPUMatrix(float* matrix, int rows, int cols)
{
	using namespace std;
	RbsLib::Math::Matrix<float> mat(rows, cols);
	cudaMemcpy(mat.Data(), matrix, rows * cols * sizeof(float), cudaMemcpyDeviceToHost);
	cout << endl << mat.ToString() << endl;
}


void __TrainCUDA(RbsLib::Math::Matrix<float> inputs, RbsLib::Math::Matrix<float> target, float learning_rate, int epochs, std::function<void(int, float)> loss_callback, std::vector<RbsLib::MatchingLearning::Layer>& layers, int activite_index)
{
	//将layers转化为LayerCUDA
	std::vector<LayerCUDA> layers_cuda;
	for (int i = 0; i < layers.size(); i++)
	{
		LayerCUDA layer_cuda;
		layer_cuda.w = layers[i].w.Data();
		layer_cuda.w_rows = layers[i].w.Rows();
		layer_cuda.w_cols = layers[i].w.Cols();

		layer_cuda.b = layers[i].b.Data();
		layer_cuda.b_rows = layers[i].b.Rows();
		layer_cuda.b_cols = layers[i].b.Cols();

		layer_cuda.output = layers[i].output.Data();
		layer_cuda.output_rows = layers[i].output.Rows();
		layer_cuda.output_cols = layers[i].output.Cols();

		layer_cuda.delta = layers[i].delta.Data();
		layer_cuda.delta_rows = layers[i].delta.Rows();
		layer_cuda.delta_cols = layers[i].delta.Cols();

		layer_cuda.z = layers[i].z.Data();
		layer_cuda.z_rows = layers[i].z.Rows();
		layer_cuda.z_cols = layers[i].z.Cols();

		layer_cuda.activation_func_index = activite_index;

		layers_cuda.push_back(layer_cuda);
	}
	//将LayerCUDA中的数组拷贝到GPU，并将指针指向GPU
	int n = 0;
	for (auto& it : layers_cuda)
	{
		cudaMalloc(&it.w, it.w_rows * it.w_cols * sizeof(float));
		cudaMemcpy(it.w, layers[n].w.Data(), it.w_rows * it.w_cols * sizeof(float), cudaMemcpyHostToDevice);

		cudaMalloc(&it.b, it.b_rows * it.b_cols * sizeof(float));
		cudaMemcpy(it.b, layers[n].b.Data(), it.b_rows * it.b_cols * sizeof(float), cudaMemcpyHostToDevice);

		cudaMalloc(&it.output, it.output_rows * it.output_cols * sizeof(float));
		//cudaMemcpy(it.output, layers[n].output.Data(), it.output_rows * it.output_cols * sizeof(float), cudaMemcpyHostToDevice);

		cudaMalloc(&it.delta, it.delta_rows * it.delta_cols * sizeof(float));
		//cudaMemcpy(it.delta, layers[n].delta.Data(), it.delta_rows * it.delta_cols * sizeof(float), cudaMemcpyHostToDevice);

		cudaMalloc(&it.z, it.z_rows * it.z_cols * sizeof(float));
		//cudaMemcpy(it.z, layers[n].z.Data(), it.z_rows * it.z_cols * sizeof(float), cudaMemcpyHostToDevice);
		++n;
	}
    //在GPU上申请空间用于反向传播时存放target
	float* target_cuda;
	float* lossp;
	cudaMalloc(&target_cuda, target.Rows() * target.Cols() * sizeof(float));
	cudaMemcpy(target_cuda, target.Data(), target.Rows() * target.Cols() * sizeof(float), cudaMemcpyHostToDevice);
	//在GPU上申请空间用于存放loss
	cudaMalloc(&lossp, sizeof(float)*target.Rows());
	float* lossh = new float[target.Rows()];
	//将主机上的lossh拷贝到GPU(初始化为0)
	cudaMemcpy(lossp, lossh, sizeof(float) * target.Rows(), cudaMemcpyHostToDevice);
	//将输入拷贝到GPU
	float* inputs_cuda;
	cudaMalloc(&inputs_cuda, inputs.Rows() * inputs.Cols() * sizeof(float));
	cudaMemcpy(inputs_cuda, inputs.Data(), inputs.Rows() * inputs.Cols() * sizeof(float), cudaMemcpyHostToDevice);


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
			//将输入拷贝到GPU第一层的输出
			cudaMemcpyAsync(layers_cuda[0].output, inputs_cuda + index * inputs.Cols(), inputs.Cols() * sizeof(float), cudaMemcpyDeviceToDevice,0);

            // 从第二层开始
            for (int j = 1; j < layers.size(); ++j)
            {
				/*
                // 在GPU中计算当前层的输出 Z = W * A + b
				// Z = W * A
				//检查Z的内容
				matrix_multiply(layers_cuda[j].w, layers_cuda[j - 1].output, layers_cuda[j].z, layers_cuda[j].w_rows, layers_cuda[j].w_cols, layers_cuda[j - 1].output_cols);
                //cudaDeviceSynchronize();

				// Z = Z + b
				matrix_add(layers_cuda[j].z, layers_cuda[j].b, layers_cuda[j].z, layers_cuda[j].z_rows, layers_cuda[j].z_cols);
                //cudaDeviceSynchronize();
				//将Z拷贝到output
				
				cudaMemcpyAsync(layers_cuda[j].output, layers_cuda[j].z, layers_cuda[j].z_rows * layers_cuda[j].z_cols * sizeof(float), cudaMemcpyDeviceToDevice, 0);

                // 应用激活函数
				matrix_apply_function(layers_cuda[j].output, layers_cuda[j].output_rows, layers_cuda[j].output_cols, layers_cuda[j].activation_func_index);
                //cudaDeviceSynchronize();
				*/
				forward(layers_cuda[j].w, layers_cuda[j - 1].output, layers_cuda[j].b, layers_cuda[j].z, layers_cuda[j].output, layers_cuda[j].w_rows, layers_cuda[j].w_cols, layers_cuda[j].activation_func_index);
            }

            // 计算损失（均方误差）
			//将结果拷贝到CPU
			add_loss(layers_cuda.back().output, target_cuda + index * target.Cols(), lossp, target.Cols());

            // 反向传播
			/*
			// 计算输出层的误差项 δ[L] = ∇C ⊙ f'(Z[L])
			//target已经在GPU上
			//delta = output - target
			matrix_sub(layers_cuda.back().output, target_cuda+index*target.Cols() , layers_cuda.back().delta, layers_cuda.back().delta_rows, layers_cuda.back().delta_cols);
			// Z = f'(Z)
			matrix_apply_function(layers_cuda.back().z, layers_cuda.back().z_rows, layers_cuda.back().z_cols, -layers_cuda.back().activation_func_index);
            //cudaDeviceSynchronize();
			//delta = delta .* Z
			matrix_elementwise_multiply(layers_cuda.back().delta, layers_cuda.back().z, layers_cuda.back().delta, layers_cuda.back().delta_rows, layers_cuda.back().delta_cols);
			*/
			backward_last(layers_cuda.back().output, target_cuda + index * target.Cols(), layers_cuda.back().delta, layers_cuda.back().z, layers_cuda.back().delta_rows, layers_cuda.back().activation_func_index,layers_cuda.back().output);
            // 计算隐藏层的误差项 δ[L-1], δ[L-2], ..., δ[1]
			// δ[L] = W[L+1]^T * δ[L+1] .* f'(Z[L])
            for (int j = layers_cuda.size() - 2; j > 0; --j)
            {
				/*
				// δ[l] = W[l+1]^T * δ[l+1]
				transpose_multiply(layers_cuda[j + 1].w, layers_cuda[j + 1].delta, layers_cuda[j].delta, layers_cuda[j + 1].w_rows, layers_cuda[j + 1].w_cols, layers_cuda[j + 1].delta_cols);
				
				// Z[l] = f'(Z[l])
				matrix_apply_function(layers_cuda[j].z, layers_cuda[j].z_rows, layers_cuda[j].z_cols, -layers_cuda[j].activation_func_index);
				// δ[l] = δ[l] .* Z[l]
				matrix_elementwise_multiply(layers_cuda[j].delta, layers_cuda[j].z, layers_cuda[j].delta, layers_cuda[j].delta_rows, layers_cuda[j].delta_cols);
				*/
				backward(layers_cuda[j + 1].w, layers_cuda[j + 1].delta, layers_cuda[j].z, layers_cuda[j].delta, layers_cuda[j + 1].w_rows, layers_cuda[j + 1].w_cols, layers_cuda[j].activation_func_index, layers_cuda[j].output);
            }
            

            // 更新权重和偏置
            for (int l = 1; l < layers.size(); ++l)
            {
				/*
				// W[l] = W[l] - η * δ[l] * A[l-1]^T
				//temp = delta[l] * A[l-1]^T A是一维向量，其转置在内存中的结构是一样的，因此不需要转置
				float* temp;
				cudaMallocAsync(&temp, layers_cuda[l].w_rows * layers_cuda[l].w_cols * sizeof(float), 0);
				matrix_multiply(layers_cuda[l].delta, layers_cuda[l - 1].output, temp, layers_cuda[l].delta_rows, layers_cuda[l].delta_cols, 1);//转置只需要将原数组的行作为列即可
				//temp = -η * temp
				scalar_multiply(temp, -learning_rate, temp, layers_cuda[l].delta_rows, layers_cuda[l].delta_cols);

				//w = w + temp
				matrix_add(layers_cuda[l].w, temp, layers_cuda[l].w, layers_cuda[l].w_rows, layers_cuda[l].w_cols);
				cudaFreeAsync(temp,0);

				//b[l] = b[l] - η * δ[l] 这是最后一次使用delta，因此不需要再次申请空间
				scalar_multiply(layers_cuda[l].delta, -learning_rate, layers_cuda[l].delta, layers_cuda[l].delta_rows, layers_cuda[l].delta_cols);
				matrix_add(layers_cuda[l].b, layers_cuda[l].delta, layers_cuda[l].b, layers_cuda[l].b_rows, layers_cuda[l].b_cols);
				//输出权值矩阵
				*/
				update_w(layers_cuda[l].w, layers_cuda[l].w_rows, layers_cuda[l].w_cols, learning_rate, layers_cuda[l].delta, layers_cuda[l - 1].output);
				update_b(layers_cuda[l].b, layers_cuda[l].b_rows, layers_cuda[l].b_cols, learning_rate, layers_cuda[l].delta);
            }
        }
        // 输出每轮的损失
		//将loss拷贝到CPU
		cudaMemcpyAsync(lossh, lossp, sizeof(float) * target.Rows(), cudaMemcpyDeviceToHost,0);
		for (int i = 0; i < target.Rows(); i++) total_loss += lossh[i];
		//重置loss
		std::memset(lossh, 0, sizeof(float) * target.Rows());
		cudaMemcpyAsync(lossp, lossh, sizeof(float) * target.Rows(), cudaMemcpyHostToDevice,0);
        loss_callback(e, total_loss / inputs.Rows());
    }
	delete[] lossh;
	//将结果拷贝回Layers
	cudaDeviceSynchronize();
	for (int i = 0; i < layers.size(); i++)
	{
		cudaMemcpy(layers[i].w.Data(), layers_cuda[i].w, layers_cuda[i].w_rows * layers_cuda[i].w_cols * sizeof(float), cudaMemcpyDeviceToHost);
		cudaMemcpy(layers[i].b.Data(), layers_cuda[i].b, layers_cuda[i].b_rows * layers_cuda[i].b_cols * sizeof(float), cudaMemcpyDeviceToHost);
		cudaMemcpy(layers[i].output.Data(), layers_cuda[i].output, layers_cuda[i].output_rows * layers_cuda[i].output_cols * sizeof(float), cudaMemcpyDeviceToHost);
		cudaMemcpy(layers[i].delta.Data(), layers_cuda[i].delta, layers_cuda[i].delta_rows * layers_cuda[i].delta_cols * sizeof(float), cudaMemcpyDeviceToHost);
		cudaMemcpy(layers[i].z.Data(), layers_cuda[i].z, layers_cuda[i].z_rows * layers_cuda[i].z_cols * sizeof(float), cudaMemcpyDeviceToHost);
	}

	//释放GPU空间
	for (auto& it : layers_cuda)
	{
		cudaFree(it.w);
		cudaFree(it.b);
		cudaFree(it.output);
		cudaFree(it.delta);
		cudaFree(it.z);
	}
	cudaFree(target_cuda);
	cudaFree(lossp);
	cudaFree(inputs_cuda);
}