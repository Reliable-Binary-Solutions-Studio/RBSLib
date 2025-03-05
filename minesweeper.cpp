#include "rbslib/Windows/BasicUI.h"
#include <vector>
#include "rbslib/Math.h"
#include <string>
int sz = 0;
int mine_num = 0;
int mine;
RbsLib::Math::Matrix<std::uint32_t> mine_map,button_map;
RbsLib::Math::Matrix<bool> mine_map_status;
std::shared_ptr< RbsLib::Windows::BasicUI::Button> sz10;
std::shared_ptr< RbsLib::Windows::BasicUI::Button> sz20;
std::shared_ptr< RbsLib::Windows::BasicUI::Button> sz40;
std::shared_ptr<RbsLib::Windows::BasicUI::Label> mine_num_label,used_time_label;
bool is_start = false;
int seconds = 0;

class Timer :public RbsLib::Windows::BasicUI::UIElement
{
private:
public:
	void OnTimer(RbsLib::Windows::BasicUI::Window& window, UINT_PTR timer_id) override
	{
		if (timer_id != 2) return;
		++seconds;
		used_time_label->Text(std::wstring(L"Used Time: ") + std::to_wstring(seconds));
	}
};


void GenerateMine(int i,int j)
{
	//在整个地图中生成雷,但以i,j为中心3*3的区域内不允许有雷
	auto generated = 0;
	while (generated < mine_num)
	{
		int x = rand() % sz;
		int y = rand() % sz;
		if (x >= i - 1 && x <= i + 1 && y >= j - 1 && y <= j + 1)
		{
			continue;
		}
		if (mine_map(x, y) == 0)
		{
			mine_map(x, y) = 1;
			++generated;
		}
	}
}

void GenerateMineMap(RbsLib::Windows::BasicUI::Window& window)
{
	if (sz == 0)
	{
		MessageBoxA(NULL, "Please select the size of the map first.","ERROR", MB_OK);
		exit(1);
	}
	mine_map = RbsLib::Math::Matrix<std::uint32_t>(sz, sz);
	button_map = RbsLib::Math::Matrix<std::uint32_t>(sz, sz);
	mine_map_status = RbsLib::Math::Matrix<bool>(sz, sz);
	auto rect = window.GetD2D1RenderTargetRect();
	mine_num_label = std::make_shared<RbsLib::Windows::BasicUI::Label>(
		RbsLib::Windows::BasicUI::Point{ ((rect.right-rect.left)-(rect.bottom-rect.top))/2 + (rect.bottom - rect.top) -50,100},
		RbsLib::Windows::BasicUI::BoxSize{ 100,50 }, std::wstring(L"Mine Num: ") + std::to_wstring(mine_num));
	used_time_label = std::make_shared<RbsLib::Windows::BasicUI::Label>(
		RbsLib::Windows::BasicUI::Point{ ((rect.right - rect.left) - (rect.bottom - rect.top)) / 2 + (rect.bottom - rect.top) - 50,150 },
		RbsLib::Windows::BasicUI::BoxSize{ 100,50 }, std::wstring(L"Used Time: ") + std::to_wstring(seconds));
	mine = mine_num;
	window.AddUIElement(mine_num_label);
	window.AddUIElement(used_time_label);
	//根据数量创建对应的button，整个地图区域位于(0,0)到(400,400)
	int width = (window.GetD2D1RenderTargetRect().bottom-window.GetD2D1RenderTargetRect().top) / sz;
	int height = (window.GetD2D1RenderTargetRect().bottom - window.GetD2D1RenderTargetRect().top) / sz;
	int real_width = width * 0.8;
	int real_height = height * 0.8;
	int width_tab_space = (width - real_width) / 2;
	int height_tab_space = (height - real_height) / 2;
	for (int i = 0; i < sz; ++i)
	{
		for (int j = 0; j < sz; ++j)
		{
			auto button = std::make_shared<RbsLib::Windows::BasicUI::Button>(RbsLib::Windows::BasicUI::Point{j*width+width_tab_space,i*height+height_tab_space}, RbsLib::Windows::BasicUI::BoxSize{real_width,real_height}, L"");
			button->events.OnClick += [i, j](RbsLib::Windows::BasicUI::EventArgs& args) {
				auto& window = args.window;
				auto& sender = args.sender;
				//若是第一次点击，生成雷
				if (!is_start)
				{
					GenerateMine(i, j);
					is_start = true;
					window.SetTimer(2, 1000);
				}
				if (dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text() != L"")
				{
					return;
				}
				//改变按钮颜色
				dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->ButtonColor(D2D1::ColorF::SlateGray);
				if (mine_map(i, j) == 1)
				{
					window.KillTimer(2);
					MessageBoxA(window.GetHWND(), "You lose!", "Game Over", MB_OK);
					exit(0);
				}
				else
				{
					mine_map_status(i, j) = true;//标记为已经点击
					//检查周围的雷的数量
					int mine_num = 0;
					for (int x = i - 1; x <= i + 1; ++x)
					{
						for (int y = j - 1; y <= j + 1; ++y)
						{
							if (x >= 0 && x < sz && y >= 0 && y < sz)
							{
								if (mine_map(x, y) == 1)
								{
									++mine_num;
								}
							}
						}
					}
					if (mine_num == 0)
					{
						dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text(L" ");//不用空字符，影响判定
					}
					else
						dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text(std::to_wstring(mine_num));
					//如果当前块为0，递归触发周围的点击事件
					if (mine_num == 0)
					{
						for (int x = i - 1; x <= i + 1; ++x)
						{
							for (int y = j - 1; y <= j + 1; ++y)
							{
								if (x >= 0 && x < sz && y >= 0 && y < sz)
								{
									if (!mine_map_status(x, y))
									{
										RbsLib::Windows::BasicUI::EventArgs e{ window,*window.GetUIElement(button_map(x, y)) };
										std::dynamic_pointer_cast<RbsLib::Windows::BasicUI::Button>(window.GetUIElement(button_map(x, y)))->events.OnClick(e);
									}
								}
							}
						}
					}
				}
				//检查是否胜利
				bool win = true;
				for (int x = 0; x < sz; ++x)
				{
					for (int y = 0; y < sz; ++y)
					{
						if (mine_map(x, y) == 0 && !mine_map_status(x, y))
						{
							win = false;
							break;
						}
					}
				}
				if (win)
				{
					window.KillTimer(2);
					MessageBoxA(window.GetHWND(), (std::string("You win!\nUsed time: ")+std::to_string(seconds)).c_str(), "Game Over", MB_OK);
					exit(0);
				}
				};
			//window.AddUIElement(button);
			button->events.OnRightButtonDown += [i, j](RbsLib::Windows::BasicUI::EventArgs& args) {
				auto& window = args.window;
				auto& sender = args.sender;
				if (dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text() == L"")
				{
					dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text(L"🚩");
					mine--;
					mine_num_label->Text(std::wstring(L"Mine Num: ") + std::to_wstring(mine));
				}
				else if (dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text() == L"🚩")
				{
					dynamic_cast<RbsLib::Windows::BasicUI::Button*>(&args.sender)->Text(L"");
					mine++;
					mine_num_label->Text(std::wstring(L"Mine Num: ") + std::to_wstring(mine));
				}
				};
			button_map(i, j) = window.AddUIElement(button);
			//立即无效化窗口区域
			InvalidateRect(window.GetHWND(), nullptr, FALSE);
		}
		
	}
	window.AddMainThreadTask([](RbsLib::Windows::BasicUI::Window &window) {
		window.RemoveUIElement(sz10->id);
		window.RemoveUIElement(sz20->id);
		window.RemoveUIElement(sz40->id);
		InvalidateRect(window.GetHWND(), nullptr, FALSE);
		});
}

int main()
{
	int width=1000;
	int height=720;
	RbsLib::Windows::BasicUI::BoxSize box_size{ 160,30 };
	int button_bum = 3;
	int button_x = width / 2 - box_size.width / 2;
	int first_button_y = height / 2 - (button_bum * (box_size.height+60)) / 2;
	srand(time(NULL));
	sz10 = std::make_shared<RbsLib::Windows::BasicUI::Button>(RbsLib::Windows::BasicUI::Point{ button_x,first_button_y }, box_size,L"10x10");
	sz20 = std::make_shared<RbsLib::Windows::BasicUI::Button>(RbsLib::Windows::BasicUI::Point{ button_x,first_button_y+=60 }, box_size, L"20x20");
	sz40 = std::make_shared<RbsLib::Windows::BasicUI::Button>(RbsLib::Windows::BasicUI::Point{ button_x,first_button_y+=60 }, box_size, L"30x30");
	sz10->events.OnClick += [](RbsLib::Windows::BasicUI::EventArgs& args) {
		auto& window = args.window;
		auto& sender = args.sender;
		sz = 10;
		mine_num = 20;
		GenerateMineMap(window);
		};
	sz20->events.OnClick += [](RbsLib::Windows::BasicUI::EventArgs& args) {
		auto& window = args.window;
		auto& sender = args.sender;
		sz = 20;
		mine_num = 80;
		GenerateMineMap(window);
		};
	sz40->events.OnClick += [](RbsLib::Windows::BasicUI::EventArgs& args) {
		auto& window = args.window;
		auto& sender = args.sender;
		sz = 30;
		mine_num = 300;
		GenerateMineMap(window);
		};
	RbsLib::Windows::BasicUI::Window window{ "扫雷",1000,720,RbsLib::Windows::BasicUI::Color::NONE ,WS_OVERLAPPEDWINDOW^ WS_MAXIMIZEBOX^WS_THICKFRAME};
	window.AddUIElement(std::make_shared<RbsLib::Windows::BasicUI::UIRoot>(D2D1::ColorF::White));
	window.AddUIElement(sz10);
	window.AddUIElement(sz20);
	window.AddUIElement(sz40);
	window.AddUIElement(std::make_shared<Timer>());
	window.Show();
	return 0;
}