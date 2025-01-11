#include <iostream>
#include "rbslib/Windows/BasicUI.h"
#include "rbslib/FileIO.h"
const int CanvasSize = 40;

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
class DrawNumber : public RbsLib::Windows::BasicUI::UIElement
{
	// 通过 UIElement 继承
private:
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
	bool canvas[CanvasSize][CanvasSize] = {0 };
	void OnKeyDown(RbsLib::Windows::BasicUI::Window& window, unsigned int vkey, unsigned int param) override
	{
		if (vkey == VK_ESCAPE)
		{
			memset(canvas, 0, sizeof(canvas));
		}
		if (vkey >= '0' && vkey <= '9')
		{
			MoveToCenter(canvas);
			RbsLib::Storage::FileIO::File f("number.bin", RbsLib::Storage::FileIO::OpenMode::Write | RbsLib::Storage::FileIO::OpenMode::Bin,RbsLib::Storage::FileIO::SeekBase::end);
			std::cout << "您输入的数字是：" << vkey - '0' << std::endl;
			f.WriteData<char>(vkey - '0');
			for (int i = 0; i < CanvasSize; i++)
			{
				for (int j = 0; j < CanvasSize; j++)
				{
					std::cout << (canvas[i][j] ? "1" : "0");
					f.WriteData<char>(canvas[i][j]?1:0);
				}
				std::cout << std::endl;
			}
			memset(canvas, 0, sizeof(canvas));
		}
		InvalidateRect(window.GetHWND(), NULL, FALSE);
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

int main()
{
	printf("在画板画出0-9之间的数字，画完后按下对应的数字键，将会保存到number.bin文件中\n");
	RbsLib::Windows::BasicUI::Window window("绘制数字", 500, 500, RbsLib::Windows::BasicUI::Color::BLACK);
	window.AddUIElement(std::make_shared<DrawNumber>());
	window.Show();
}