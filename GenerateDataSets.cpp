#include <iostream>
#include "rbslib/Windows/BasicUI.h"
#include "rbslib/FileIO.h"
const int CanvasSize = 40;

void MoveToCenter(bool canvas[CanvasSize][CanvasSize])
{
	//��ֱ�����ҵ��ѻ�����������±߽�
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
	//ˮƽ�����ҵ��ѻ�����������ұ߽�
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
	//��������λ��
	int center_x = (left + right) / 2;
	int center_y = (top + bottom) / 2;
	//����ƫ����
	int offset_x = CanvasSize / 2 - center_x;
	int offset_y = CanvasSize / 2 - center_y;
	//�ƶ�����
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
	// ͨ�� UIElement �̳�
private:
	RbsLib::Windows::BasicUI::Point prev_point;
	bool is_prev_point_valid = false;
	std::vector<RbsLib::Windows::BasicUI::Point> GetLinePoints(const RbsLib::Windows::BasicUI::Point& x, const RbsLib::Windows::BasicUI::Point& y, float interval) const
	{
		//���㹲�ж��ٸ���
		int count = RbsLib::Windows::BasicUI::GetDistance(x, y) / interval;
		if (count == 0)
		{
			return std::vector<RbsLib::Windows::BasicUI::Point>();
		}
		//����dx,dy
		float dx = (y.x - x.x) / static_cast<float>(count);
		float dy = (y.y - x.y) / static_cast<float>(count);
		//�������е�
		std::vector<RbsLib::Windows::BasicUI::Point> points(count);
		for (int i = 0; i < count; i++)
		{
			points[i].x = x.x + dx * i;
			points[i].y = x.y + dy * i;
		}
		return points;
	}
	//����
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
			std::cout << "������������ǣ�" << vkey - '0' << std::endl;
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
		//����������Ƿ���
		if (!(key_status & MK_LBUTTON))
		{
			is_prev_point_valid = false;
			return;
		}
		//�������λ������Ч�ģ�����ϴ����λ���Ƿ���Ч
		if (is_prev_point_valid == false)
		{
			//�ϴ����λ����Ч������Ϊ�������λ��
			prev_point.x = x;
			prev_point.y = y;
			is_prev_point_valid = true;
			return;
		}
		//���ξ���Ч������ֱ�ߵ�
		auto line = GetLinePoints(prev_point, RbsLib::Windows::BasicUI::Point{ x,y }, 1);
		//����ÿ���ض�Ӧ�Ŀ�Ⱥ͸߶�		
		int width = window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left;
		int height = window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top;
		double pixel_width = width / (double)CanvasSize;
		double pixel_height = height / (double)CanvasSize;
		for (const auto& it : line)
		{
			//��������ڵ�����λ��
			int i = it.y / pixel_height;
			int j = it.x / pixel_width;
			//�ж��Ƿ��ڻ�����
			if (i >= 0 && i < CanvasSize && j >= 0 && j < CanvasSize)
			{
				canvas[i][j] = true;
			}
		}
		//�����ϴ����λ��
		prev_point.x = x;
		prev_point.y = y;
		is_prev_point_valid = true;
		InvalidateRect(window.GetHWND(), NULL, FALSE);
	}
	void Draw(RbsLib::Windows::BasicUI::Window& window) override
	{
		//����ÿ���ض�Ӧ�Ŀ�Ⱥ͸߶�
		int width = window.GetD2D1RenderTargetRect().right - window.GetD2D1RenderTargetRect().left;
		int height = window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top;
		double pixel_width = width / (double)CanvasSize;
		double pixel_height = height / (double)CanvasSize;


		//��������
		ID2D1SolidColorBrush* brush_black, * brush_white;
		window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush_black);
		window.d2d1_render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush_white);
		//������������Ϊ��ɫ
		window.d2d1_render_target->FillRectangle(D2D1::RectF(0, 0, width, height), brush_white);
		for (int i = 0; i < CanvasSize; i++)
		{
			for (int j = 0; j < CanvasSize; j++)
			{
				if (canvas[i][j])
				{
					//������Ӧ������
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
	printf("�ڻ��廭��0-9֮������֣�������¶�Ӧ�����ּ������ᱣ�浽number.bin�ļ���\n");
	RbsLib::Windows::BasicUI::Window window("��������", 500, 500, RbsLib::Windows::BasicUI::Color::BLACK);
	window.AddUIElement(std::make_shared<DrawNumber>());
	window.Show();
}