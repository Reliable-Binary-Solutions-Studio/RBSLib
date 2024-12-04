#include "Plot.h"

void RbsLib::Windows::Graph::Plot::AddPlot(const std::vector<double>& x, const std::vector<double>& y, D2D1::ColorF color)
{
	if (x.size() != y.size())
	{
		throw BasicUI::BasicUIException("The size of x and y must be the same.");
	}
	x_list.push_back(x);
	y_list.push_back(y);
	color_list.push_back(color);
}

void RbsLib::Windows::Graph::Plot::AddScatter(const std::vector<double>& x, const std::vector<double>& y, D2D1::ColorF color)
{
	if (x.size() != y.size())
	{
		throw BasicUI::BasicUIException("The size of x and y must be the same.");
	}
	scatter_x_list.push_back(x);
	scatter_y_list.push_back(y);
	scatter_color_list.push_back(color);
}

void RbsLib::Windows::Graph::Plot::Show(void)
{
	if (x_list.size() == 0 && scatter_x_list.size() == 0)
		return;
	RbsLib::Windows::BasicUI::Window window("Plot", 800, 600, RbsLib::Windows::BasicUI::Color::WHITE);
	window.AddUIElement(std::make_shared<PlotElement>(x_list, y_list, color_list,scatter_x_list,scatter_y_list,scatter_color_list));
	window.Show();
}



RbsLib::Windows::Graph::Plot::PlotElement::PlotElement(const std::list<std::vector<double>>& x_list, const std::list<std::vector<double>>& y_list, const std::list<D2D1::ColorF>& color_list, const std::list<std::vector<double>>& scatter_x_list, const std::list<std::vector<double>>& scatter_y_list, const std::list<D2D1::ColorF>& scatter_color_list)
{
	this->x_list = x_list;
	this->y_list = y_list;
	this->color_list = color_list;
	this->scatter_x_list = scatter_x_list;
	this->scatter_y_list = scatter_y_list;
	this->scatter_color_list = scatter_color_list;
}

void RbsLib::Windows::Graph::Plot::PlotElement::Draw(RbsLib::Windows::BasicUI::Window& window)
{
	//在控件背景绘制白色背景
	ID2D1SolidColorBrush* brush;
	window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
	window.d2d1_render_target->FillRectangle(D2D1::RectF(0, 0, window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left, window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top),brush);
	brush->Release();

	//计算x最大值和最小值
	double max_x = x_list.front().front();
	double min_x = x_list.front().front();
	for (const auto& x : x_list)
	{
		for (const auto& x_value : x)
		{
			if (x_value > max_x)
			{
				max_x = x_value;
			}
			if (x_value < min_x)
			{
				min_x = x_value;
			}
		}
	}
	//计算y最大值和最小值
	double max_y = y_list.front().front();
	double min_y = y_list.front().front();
	for (const auto& y : y_list)
	{
		for (const auto& y_value : y)
		{
			if (y_value > max_y)
			{
				max_y = y_value;
			}
			if (y_value < min_y)
			{
				min_y = y_value;
			}
		}
	}
	//计算绘图区域大小
	int width = window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left;
	int height = window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top;
	//计算x轴和y轴的比例
	double x_ratio = width / (max_x - min_x);
	double y_ratio = height / (max_y - min_y);
	//绘制曲线
	auto render_target = window.d2d1_render_target;
	auto color_iter = color_list.begin();
	for (auto x_iter = x_list.begin(), y_iter = y_list.begin() ; x_iter != x_list.end(); ++x_iter, ++y_iter, ++color_iter)
	{
		auto& x = *x_iter;
		auto& y = *y_iter;
		auto& color = *color_iter;
		ID2D1SolidColorBrush* brush;
		render_target->CreateSolidColorBrush(D2D1::ColorF(color.r, color.g, color.b), &brush);
		for (int i = 1; i < x.size(); ++i)
		{
			window.d2d1_render_target->DrawLine(D2D1::Point2F((x[i - 1] - min_x) * x_ratio, height - (y[i - 1] - min_y) * y_ratio), D2D1::Point2F((x[i] - min_x) * x_ratio, height - (y[i] - min_y) * y_ratio), brush);
		}
		brush->Release();
	}
	//绘制散点
	color_iter = scatter_color_list.begin();
	for (auto x_iter = scatter_x_list.begin(), y_iter = scatter_y_list.begin(); x_iter != scatter_x_list.end(); ++x_iter, ++y_iter, ++color_iter)
	{
		auto& x = *x_iter;
		auto& y = *y_iter;
		auto& color = *color_iter;
		ID2D1SolidColorBrush* brush;
		render_target->CreateSolidColorBrush(D2D1::ColorF(color.r, color.g, color.b), &brush);
		for (int i = 0; i < x.size(); ++i)
		{
			render_target->FillEllipse(D2D1::Ellipse(D2D1::Point2F((x[i] - min_x) * x_ratio, height - (y[i] - min_y) * y_ratio), 2, 2), brush);
		}
		brush->Release();
	}
}
