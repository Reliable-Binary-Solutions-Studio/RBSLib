#include "rbslib/Windows/BasicUI.h"
#include <list>
#include <iostream>
#include <random>

class FlappyBird : public RbsLib::Windows::BasicUI::UIElement
{
	struct Rectangle
	{
		RbsLib::Windows::BasicUI::Point position;
		RbsLib::Windows::BasicUI::BoxSize size;
	};
	RbsLib::Windows::BasicUI::Point bird = {50,200};
	int bird_radius = 10;
	int pipe_width = 50;
	double pipe_speed = 1;
	double bird_speed = 0;
	std::list<Rectangle> pipes;
	bool is_running = false;
	float birt_acceleration = 0.1;
	ID2D1SolidColorBrush* bird_brush = nullptr, * pipe_brush = nullptr;
	ID2D1Factory* factory = nullptr;
	ID2D1RenderTarget* render_target = nullptr;
	IDWriteFactory* dwrite_factory = nullptr;
	IDWriteTextFormat* text_format = nullptr;
	int score = 0;

public:
	void OnWindowCreate(RbsLib::Windows::BasicUI::Window& window) override
	{
		//创建画笔
		HRESULT hr;
		hr = window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &bird_brush);
		if (!SUCCEEDED(hr)) throw std::runtime_error("Create bird brush failed");
		hr = window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &pipe_brush);
		if (!SUCCEEDED(hr)) throw std::runtime_error("Create pipe brush failed");
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&this->dwrite_factory)
		);
		if (!SUCCEEDED(hr))
		{
			this->dwrite_factory = nullptr;
			throw std::runtime_error("Create dwrite factory failed");
		}

		hr = dwrite_factory->CreateTextFormat(
			L"宋体",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			50,
			L"",
			&this->text_format
		);
		if (!SUCCEEDED(hr))
		{
			this->text_format = nullptr;
			throw std::runtime_error("Create text format failed");
		}
		this->text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		this->text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		window.SetTimer(1, 10);
		srand(GetTickCount());
		is_running = true;
	}
	void Reset(void)
	{
		bird = { 100,100 };
		pipes.clear();
		bird_speed = 0;
		score = 0;
	}
	void OnTimer(RbsLib::Windows::BasicUI::Window& window, UINT_PTR timer_id) override
	{
		//管道高度均为屏幕高度，可以将其部分移出屏幕留出空间
		if (is_running)
		{
			auto rect = window.GetD2D1RenderTargetRect();
			int height = rect.bottom - rect.top;
			//检查是否需要生成新的管道
			if (pipes.empty() || pipes.back().position.x < rect.right - rect.left - 200)
			{
				Rectangle pipe, pipe2;
				pipe.position.x = rect.right - rect.left;
				pipe.position.y = rand() % (height-200) + 200;
				pipe.size.width = pipe_width;
				pipe.size.height = height;
				pipe2 = pipe;
				pipe2.position.y -= 200 + height;
				pipes.push_back(pipe);
				pipes.push_back(pipe2);
			}
			//移动管道
			for (auto& x : pipes)
			{
				x.position.x -= pipe_speed;
			}
			//移除不可见的管道
			while (!pipes.empty() && pipes.front().position.x + pipe_width < 0)
			{
				pipes.pop_front();
				pipes.pop_front();
				score++;
			}
			//移动小鸟
			bird_speed += birt_acceleration;
			bird.y += bird_speed;
			//碰撞检测在绘制时进行
		}
		InvalidateRect(window.GetHWND(), nullptr, FALSE);
	}
	void OnLeftButtonDown(RbsLib::Windows::BasicUI::Window& window, int x, int y, int key_status) override
	{
		if (is_running)
		{
			bird_speed = -5;
		}
	}
	void OnKeyDown(RbsLib::Windows::BasicUI::Window& window, unsigned int vkey, unsigned int param) override
	{
		if (vkey == VK_ESCAPE)
		{
			this->Reset();
			is_running = true;
		}
	}
	void Draw(RbsLib::Windows::BasicUI::Window& window) override
	{
		//假设画笔已经创建
		//创建小鸟的几何图像
		if (!is_running)
		{
			//显示得分
			std::wstring text = L"得分：" + std::to_wstring(score);
			text += L"\n按ESC键重新开始";
			window.d2d1_render_target->DrawText(text.c_str(), text.size(), this->text_format, D2D1::RectF(0, 0, window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left, window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top), bird_brush);
			return;
		}
		this->factory = window.d2d1_factory;
		this->render_target = window.d2d1_render_target;
		HRESULT hr;
		ID2D1EllipseGeometry* bird_geometry = nullptr;
		hr = this->factory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(bird.x, bird.y), bird_radius, bird_radius), &bird_geometry);
		if (!SUCCEEDED(hr)) throw std::runtime_error("Create bird geometry failed");
		//创建管道的几何图像
		std::list<ID2D1RectangleGeometry*> pipe_geometry_list;
		for (const auto& x : pipes)
		{
			ID2D1RectangleGeometry* pipe_geometry = nullptr;
			hr = this->factory->CreateRectangleGeometry(D2D1::RectF(x.position.x, x.position.y, x.position.x + x.size.width, x.position.y + x.size.height), &pipe_geometry);
			if (!SUCCEEDED(hr)) throw std::runtime_error("Create pipe geometry failed");
			pipe_geometry_list.push_back(pipe_geometry);
		}
		//绘制图形
		this->render_target->FillGeometry(bird_geometry, bird_brush);
		for (auto x : pipe_geometry_list)
		{
			this->render_target->FillGeometry(x, pipe_brush);
		}
		//碰撞检测
		if (bird.y - bird_radius < 0 || bird.y + bird_radius > window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top)
		{
			is_running = false;
		}
		D2D1_GEOMETRY_RELATION relation;
		for (auto x : pipe_geometry_list)
		{
			bird_geometry->CompareWithGeometry(x, D2D1::IdentityMatrix(), &relation);
			if (relation != D2D1_GEOMETRY_RELATION::D2D1_GEOMETRY_RELATION_DISJOINT)
			{
				is_running = false;
				break;
			}
		}
		//右上角显示得分
		std::wstring text = std::to_wstring(score);
		this->render_target->DrawText(text.c_str(), text.size(), this->text_format, D2D1::RectF(window.GetD2D1RenderTargetRect().right - 100, 0, window.GetD2D1RenderTargetRect().right, 50), bird_brush);
		//释放资源
		bird_geometry->Release();
		for (auto x : pipe_geometry_list)
		{
			x->Release();
		}
	}
};

int main()
{
	RbsLib::Windows::BasicUI::Window window("flappy bird", 800, 600, RbsLib::Windows::BasicUI::Color::WHITE);
	window.AddUIElement(std::make_shared<RbsLib::Windows::BasicUI::UIRoot>(D2D1::ColorF(D2D1::ColorF::White)));
	auto flappy_bird = std::make_shared<FlappyBird>();
	window.AddUIElement(flappy_bird);
	window.Show();
	return 0;
}