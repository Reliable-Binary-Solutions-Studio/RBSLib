#include "rbslib/Network.h"
#include "rbslib/Windows/BasicUI.h"
#include <iostream>
#include <thread>
#include <wincodec.h>
#include <stdio.h>
int count = 0;

class MyElement :public RbsLib::Windows::BasicUI::UIElement
{
	double x, y;
	double rx,ry;
	double speed_x = 4, speed_y = 1;
	double ax = 0;
	double ay = 1;
	std::vector<D2D1::ColorF> color_list = { D2D1::ColorF::Blue,D2D1::ColorF::Red,D2D1::ColorF::Green,D2D1::ColorF::Yellow,D2D1::ColorF::Purple,D2D1::ColorF::DarkGray };
	int color_index = 0;
public:
	MyElement(int x = 0, int y = 0,int rx=0,int ry=0)
		:x(x), y(y), rx(rx), ry(ry)
	{
	}
	void Draw(RbsLib::Windows::BasicUI::Window& window) override
	{
		count++;
		ID2D1SolidColorBrush* brush = nullptr;
		window.d2d1_render_target->Clear(D2D1::ColorF(D2D1::ColorF::White));
		window.d2d1_render_target->CreateSolidColorBrush(color_list[color_index], &brush);
		window.d2d1_render_target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), rx, ry), brush);
		
		brush->Release();
	}
	void OnTimer(RbsLib::Windows::BasicUI::Window& window,UINT_PTR id) override
	{
		double backup_x = x;
		double backup_y = y;
		double backup_speed_x = speed_x;
		double backup_speed_y = speed_y;
		double backup_ax = ax;
		double backup_ay = ay;
		speed_x += ax;
		speed_y += ay;
		x += speed_x;
		y += speed_y;
		
		//四边检测
		RECT rect = window.GetD2D1RenderTargetRect();
		//左边
		if (x - rx < rect.left)
		{
			//还原更改
			x = backup_x;
			speed_x = -speed_x;
			ax = backup_ax;
			color_index++;
		}
		//右边
		if (x + rx > rect.right)
		{
			x = backup_x;
			speed_x = -speed_x;
			ax = backup_ax;
			color_index++;
		}
		//上边
		if (y - ry < rect.top)
		{
			y = backup_y;
			speed_y = -speed_y;
			ay = backup_ay;
			color_index++;
		}
		//下边
		if (y + ry > rect.bottom)
		{
			y = backup_y;
			speed_y = -speed_y;
			speed_y *= 0.9;
			speed_x *= 0.9;
			ay = backup_ay;
			if (speed_y < 0.5 && speed_y>-0.5)
			{
				srand(time(0));
				speed_y = rand() % 800+100;
				speed_x = rand() % 800+100;
			}
			color_index++;
		}
		if (color_index >= color_list.size())
			color_index = 0;
		window.Draw();
	}
	void OnWindowSizeChanged(RbsLib::Windows::BasicUI::Window& window) override
	{
		RECT rc = window.WindowSize();
		if (rc.right-rc.left < rx*2 || rc.bottom-rc.top < ry*2)
			window.WindowSize(rc.left, rc.top, rx * 2, ry * 2);
		window.ResetD2D1RenderTarget();
	}
	void MouseMove(RbsLib::Windows::BasicUI::Window& window, int x, int y, int key_status) override
	{
		if (key_status == MK_LBUTTON)
		{
			this->x = x;
			this->y = y;
		}
	}
};

int main()
{
	IWICImagingFactory* m_pIWICImagingFactory = NULL;
	if (NULL == m_pIWICImagingFactory)
	{
		CoInitialize(NULL);
		CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_pIWICImagingFactory)
		);
	}
	IWICBitmapDecoder* m_pIWICBitmapDecoder = NULL;
	m_pIWICImagingFactory->CreateDecoderFromFilename(L"test.jpg", NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_pIWICBitmapDecoder);
	RbsLib::Windows::BasicUI::Window window("Test", 500, 500, RbsLib::Windows::BasicUI::Color::WHITE);
	window.SetTimer(1, 10);//设置定时器
	window.AddUIElement(std::make_shared<MyElement>(50,50,30,30));
	std::thread([]() {
		Sleep(2000);
		while (1)
		{
			int x = count;
			Sleep(1000);
			int y = count;
			std::cout << y - x << std::endl;
		}
		}).detach();
	window.Show();
	
}