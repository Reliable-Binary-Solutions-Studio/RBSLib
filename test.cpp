#include "rbslib/Storage.h"
#include "rbslib/Math.h"
#include <stdlib.h>
#include <iostream>
#include <format>
#include "rbslib/MatchingLearning.h"
#include "rbslib/Windows/Plot.h"
#include <stdio.h>
#include <set>
#include <random>
#include <initializer_list>
#include <algorithm>
#include <map>
#include <numeric>
const int CanvasSize = 40;

float func(float x)
{
	return sin(x);
}

void MoveToCenter(bool canvas[CanvasSize][CanvasSize])
{
	//垂直方向找到已绘制区域的上下边界
	int top = 0, bottom = CanvasSize - 1;
	for (int i = 0; i < CanvasSize; i++)
	{
		bool flag = false;
		for (int j = 0; j < CanvasSize; j++)
		{
			if (canvas[i][j])
			{
				flag = true;
				break;
			}
		}
		if (flag)
		{
			top = i;
			break;
		}
	}
	for (int i = CanvasSize - 1; i >= 0; i--)
	{
		bool flag = false;
		for (int j = 0; j < CanvasSize; j++)
		{
			if (canvas[i][j])
			{
				flag = true;
				break;
			}
		}
		if (flag)
		{
			bottom = i;
			break;
		}
	}
	//水平方向找到已绘制区域的左右边界
	int left = 0, right = CanvasSize - 1;
	for (int j = 0; j < CanvasSize; j++)
	{
		bool flag = false;
		for (int i = 0; i < CanvasSize; i++)
		{
			if (canvas[i][j])
			{
				flag = true;
				break;
			}
		}
		if (flag)
		{
			left = j;
			break;
		}
	}
	for (int j = CanvasSize - 1; j >= 0; j--)
	{
		bool flag = false;
		for (int i = 0; i < CanvasSize; i++)
		{
			if (canvas[i][j])
			{
				flag = true;
				break;
			}
		}
		if (flag)
		{
			right = j;
			break;
		}
	}
	//计算中心位置
	int center_x = (left + right) / 2;
	int center_y = (top + bottom) / 2;
	//计算偏移量
	int offset_x = CanvasSize / 2 - center_x;
	int offset_y = CanvasSize / 2 - center_y;
	//移动画布
	bool new_canvas[CanvasSize][CanvasSize] = { 0 };
	for (int i = 0; i < CanvasSize; i++)
	{
		for (int j = 0; j < CanvasSize; j++)
		{
			int new_i = i + offset_y;
			int new_j = j + offset_x;
			if (new_i >= 0 && new_i < CanvasSize && new_j >= 0 && new_j < CanvasSize)
			{
				new_canvas[new_i][new_j] = canvas[i][j];
			}
		}
	}
	memcpy(canvas, new_canvas, sizeof(new_canvas));
}

std::vector<int> GenerateUniqueRandomValues(int min, int max, size_t count) {
	std::set<int> unique_values; // 用集合来保证不重复
	std::random_device rd; // 获取随机设备
	std::mt19937 gen(rd()); // 生成随机数引擎
	std::uniform_real_distribution<> dis(min, max); // 均匀分布

	// 不断生成直到集合中有指定个数的唯一值
	while (unique_values.size() < count) {
		float value = dis(gen); // 生成随机数
		unique_values.insert(value); // 插入集合，自动去重
	}

	// 将结果转化为 vector 返回
	return std::vector<int>(unique_values.begin(), unique_values.end());
}

int min_index(float a, float b, float c) {
	if (a <= b && a <= c) {
		return 1;  // a 是最小的，位置是第一个
	}
	else if (b <= a && b <= c) {
		return 2;  // b 是最小的，位置是第二个
	}
	else {
		return 3;  // c 是最小的，位置是第三个
	}
}

int max_index(float a, float b, float c) {
	if (a >= b && a >= c) {
		return 1;  // a 是最大的，位置是第一个
	}
	else if (b >= a && b >= c) {
		return 2;  // b 是最大的，位置是第二个
	}
	else {
		return 3;  // c 是最大的，位置是第三个
	}
}

class DrawNumber : public RbsLib::Windows::BasicUI::UIElement
{
	// 通过 UIElement 继承
public:
	DrawNumber(RbsLib::MatchingLearning::NeuralNetworks* model):model(model){}
private:
	RbsLib::MatchingLearning::NeuralNetworks* model;
	RbsLib::Windows::BasicUI::Point prev_point;
	bool is_prev_point_valid = false;
	std::vector<RbsLib::Windows::BasicUI::Point> GetLinePoints(const RbsLib::Windows::BasicUI::Point& x, const RbsLib::Windows::BasicUI::Point& y, float interval) const
	{
		//计算共有多少个点
		int count = RbsLib::Windows::BasicUI::GetDistance(x, y) / interval;
		if (count == 0)
		{
			return std::vector<RbsLib::Windows::BasicUI::Point>();
		}
		//计算dx,dy
		float dx = (y.x - x.x) / static_cast<float>(count);
		float dy = (y.y - x.y) / static_cast<float>(count);
		//计算所有点
		std::vector<RbsLib::Windows::BasicUI::Point> points(count);
		for (int i = 0; i < count; i++)
		{
			points[i].x = x.x + dx * i;
			points[i].y = x.y + dy * i;
		}
		return points;
	}
	//画布
	bool canvas[CanvasSize][CanvasSize] = { 0 };
	void OnKeyDown(RbsLib::Windows::BasicUI::Window& window, unsigned int vkey, unsigned int param) override
	{
		if (vkey == VK_SPACE)
		{
			MoveToCenter(canvas);
			InvalidateRect(window.GetHWND(), NULL, FALSE);
			RbsLib::Math::Matrix<float> X(1, CanvasSize * CanvasSize);
			for (int i = 0; i < CanvasSize; i++)
			{
				for (int j = 0; j < CanvasSize; j++)
				{
					X[0][i * CanvasSize + j] = canvas[i][j];
				}
			}
			auto Y = model->Predict(X);
			int max_index = 0;
			for (int j = 0; j < 10; j++)
			{
				printf("%d:%.6f\n", j, Y[0][j]);
				if (Y[0][j] > Y[0][max_index])
				{
					max_index = j;
				}
			}
			printf("预测：%d\n", max_index);
		}
		else if (vkey == VK_ESCAPE)
		{
			memset(canvas, 0, sizeof(canvas));
			InvalidateRect(window.GetHWND(), NULL, FALSE);
		}
	}
	void MouseMove(RbsLib::Windows::BasicUI::Window& window, int x, int y, int key_status) override
	{
		//检查鼠标左键是否按下
		if (!(key_status & MK_LBUTTON))
		{
			is_prev_point_valid = false;
			return;
		}
		//本次鼠标位置是有效的，检查上次鼠标位置是否有效
		if (is_prev_point_valid == false)
		{
			//上次鼠标位置无效，更新为本次鼠标位置
			prev_point.x = x;
			prev_point.y = y;
			is_prev_point_valid = true;
			return;
		}
		//两次均有效，构造直线点
		auto line = GetLinePoints(prev_point, RbsLib::Windows::BasicUI::Point{ x,y }, 1);
		//计算每像素对应的宽度和高度		
		int width = window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left;
		int height = window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top;
		double pixel_width = width / (double)CanvasSize;
		double pixel_height = height / (double)CanvasSize;
		for (const auto& it : line)
		{
			//计算点所在的像素位置
			int i = it.y / pixel_height;
			int j = it.x / pixel_width;
			//判断是否在画布内
			if (i >= 0 && i < CanvasSize && j >= 0 && j < CanvasSize)
			{
				canvas[i][j] = true;
			}
		}
		//更新上次鼠标位置
		prev_point.x = x;
		prev_point.y = y;
		is_prev_point_valid = true;
		InvalidateRect(window.GetHWND(), NULL, FALSE);
	}
	void Draw(RbsLib::Windows::BasicUI::Window& window) override
	{
		//计算每像素对应的宽度和高度
		int width = window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left;
		int height = window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top;
		double pixel_width = width / (double)CanvasSize;
		double pixel_height = height / (double)CanvasSize;


		//创建画笔
		ID2D1SolidColorBrush* brush_black, * brush_white;
		window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush_black);
		window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush_white);
		//整个背景绘制为白色
		window.d2d1_render_target->FillRectangle(D2D1::RectF(0, 0, width, height), brush_white);
		for (int i = 0; i < CanvasSize; i++)
		{
			for (int j = 0; j < CanvasSize; j++)
			{
				if (canvas[i][j])
				{
					//画出对应的像素
					window.d2d1_render_target->FillRectangle(D2D1::RectF(j * pixel_width, i * pixel_height, (j + 1) * pixel_width, (i + 1) * pixel_height), brush_black);
				}
				else
				{
					window.d2d1_render_target->FillRectangle(D2D1::RectF(j * pixel_width, i * pixel_height, (j + 1) * pixel_width, (i + 1) * pixel_height), brush_white);
				}
			}
		}
		brush_black->Release();
		brush_white->Release();
	}
};

std::map<int, std::vector<std::vector<int>>> Nums;
int main()
{
	try
	{
		int canvas_size = CanvasSize;
		//读取数据集
		RbsLib::Storage::FileIO::File f("number.bin", RbsLib::Storage::FileIO::OpenMode::Read | RbsLib::Storage::FileIO::OpenMode::Bin);
		try
		{
			while (1)
			{
				char ch, b;
				f.GetData<char>(ch);
				std::vector<int> d;
				for (int i = 0; i < canvas_size * canvas_size; ++i)
				{
					f.GetData<char>(b);
					d.push_back(b);
				}
				Nums[ch].push_back(d);
			}
		}
		catch (...)
		{
		}
		//创建数据集
		int datasets_size = 0;
		for (auto& it : Nums)
		{
			datasets_size += it.second.size();
		}

		RbsLib::Math::Matrix<float> X(datasets_size, canvas_size * canvas_size), Y(datasets_size, 10);
		//将数据存入矩阵
		int index = 0;
		for (auto& it : Nums)
		{
			for (auto& x : it.second)
			{
				for (int i = 0; i < canvas_size * canvas_size; i++)
				{
					X[index][i] = x[i];
				}
				for (int i = 0; i < 10; i++)
				{
					Y[index][i] = 0;
				}
				Y[index][it.first] = 1;
				index++;
			}
		}
		//全部作为训练集
		int layers_num;
		std::cout << "请输入神经网络隐藏层的层数：";
		std::cin >> layers_num;
		std::vector<int> layers(layers_num + 2);
		layers[0] = canvas_size * canvas_size;
		layers[layers_num + 1] = 10;
		for (int i = 1; i <= layers_num; i++)
		{
			std::cout << "请输入第" << i << "层的神经元个数：";
			std::cin >> layers[i];
		}
		double learning_rate;
		std::cout << "请输入学习率：";
		std::cin >> learning_rate;
		int epochs;
		std::cout << "请输入训练次数：";
		std::cin >> epochs;
		std::vector<std::function<float(float)>> activation_functions(layers_num + 1);
		std::vector<std::function<float(float)>> activation_functions_derivative(layers_num + 1);
		for (int i = 0; i <= layers_num; i++)
		{
			activation_functions[i] = RbsLib::Math::sigmoid;
			activation_functions_derivative[i] = RbsLib::Math::sigmoid_derivative;
		}
		RbsLib::MatchingLearning::NeuralNetworks nn(
			layers, activation_functions, activation_functions_derivative,
			5);
		std::vector<double> losss;
		nn.Train(X, Y, learning_rate, epochs, [&losss](int e, float loss) {
			static int x = 0;
			if (x > 10)
			{
				printf("epoch:%d loss: %f\n", e, loss);
				x = 0;
				losss.push_back(loss);
			}
			x++;
			});
		RbsLib::Windows::Graph::Plot plot;
		std::vector<double> x_axis(losss.size());
		std::iota(x_axis.begin(), x_axis.end(), 0);
		plot.AddPlot(x_axis, losss, D2D1::ColorF::Red);
		plot.Show("loss");
		//用训练集作为测试集
		auto Ypred = nn.Predict(X);
		int correct = 0;
		for (int i = 0; i < Ypred.Rows(); ++i)
		{
			auto y = Ypred[i];
			auto ytrue = Y[i];
			int max_index = 0;
			int max_index_true = 0;
			for (int j = 0; j < 10; j++)
			{
				if (y[j] > y[max_index])
				{
					max_index = j;
				}
				if (ytrue[j] > ytrue[max_index_true])
				{
					max_index_true = j;
				}
			}
			printf("预测：%d 真实：%d\n", max_index, max_index_true);
			if (max_index != max_index_true) correct += 1;
		}
		printf("正确率：%f\n", 1 - correct / (double)datasets_size);
		//现场绘图测试
		std::cout << "请在窗口中绘制数字，绘制完毕后按空格键进行预测，按ESC键清空画布\n" << std::endl;
		RbsLib::Windows::BasicUI::Window window("请输入数字", 500, 500, RbsLib::Windows::BasicUI::Color::BLACK);
		window.AddUIElement(std::make_shared<DrawNumber>(&nn));
		window.Show();
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}