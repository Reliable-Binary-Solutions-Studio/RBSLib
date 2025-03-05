#include "BasicUI.h"
#ifdef WIN32
#include <unordered_map>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <windowsx.h>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "Imm32.lib")

static std::unordered_map<HWND, RbsLib::Windows::BasicUI::Window*> RunningWindowsList;

auto CALLBACK RbsLib::Windows::BasicUI::Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
{

	auto x = RunningWindowsList.find(hwnd);
	if (x == RunningWindowsList.end())
	{
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	auto self_ref = x->second;
	RECT rc;
	LRESULT result = -1;
	switch (message)
	{
	case WM_LBUTTONDOWN:
		self_ref->OnLeftButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
		result = 0;
		break;
	case WM_LBUTTONUP:
		self_ref->OnLeftButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
		result = 0;
		break;
	case WM_RBUTTONDOWN:
		self_ref->OnRightButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
		result = 0;
		break;
	case WM_MOUSEMOVE:
		self_ref->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
		result = 0;
		break;
	case WM_TIMER:
		self_ref->OnTimer(wParam);
		result = 0;
		break;
	case WM_PAINT:
		self_ref->PaintWindow();
		result = 0;
		break;
	case WM_SIZE:
		self_ref->OnWindowSizeChanged();
		result = 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		result = 0;
		break;
	case WM_KEYDOWN:
		self_ref->OnKeyDown(wParam, lParam);
		result = 0;
		break;
	case WM_CREATE:
		self_ref->OnWindowCreate();
		result = 0;
		break;
	}
	self_ref->main_thread_tasks(*self_ref);
	self_ref->main_thread_tasks.Clear();
	if (result != -1)
	{
		return result;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void RbsLib::Windows::BasicUI::Window::CreateD2D1Factory(void)
{
	auto hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&this->d2d1_factory
	);
	if (!SUCCEEDED(hr)) throw BasicUIException("Create factory failed");
}

void RbsLib::Windows::BasicUI::Window::CreateD2D1RenderTarget(void)
{
	RECT rc;
	GetClientRect(this->hwnd, &rc);
	auto hr = this->d2d1_factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			this->hwnd,
			D2D1::SizeU(
				rc.right - rc.left,
				rc.bottom - rc.top)
		),
		&this->d2d1_render_target
	);
	if (!SUCCEEDED(hr)) throw BasicUIException("Create render target failed");
	this->BindedRect = rc;
}

void RbsLib::Windows::BasicUI::Window::ReleaseD2D1RenderTarget(void)
{
	if (this->d2d1_render_target != nullptr)
	{
		this->d2d1_render_target->Release();
		this->d2d1_render_target = nullptr;
	}
}

void RbsLib::Windows::BasicUI::Window::ReleaseD2D1Factory(void)
{
	if (this->d2d1_factory != nullptr)
	{
		this->d2d1_factory->Release();
		this->d2d1_factory = nullptr;
	}
}

bool RbsLib::Windows::BasicUI::Window::IsD2D1RenderTargetInitialized(void) const noexcept
{
	return this->d2d1_render_target;
}

std::uint32_t RbsLib::Windows::BasicUI::Window::GenerateUIElementID(void)
{
	std::uint32_t id;
	for (id = 1; id != 0; ++id)
	{
		if (std::find_if(this->ui_element_list.begin(), this->ui_element_list.end(), [id](const std::shared_ptr<UIElement>& x) {return x->id == id; }) == this->ui_element_list.end())
		{
			break;
		}
	}
	return id;
}

void RbsLib::Windows::BasicUI::Window::_OnWindowCreateHandler(Window& window)
{
	for (auto& x : window.ui_element_list)
	{
		x->OnWindowCreate(window);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnPaintHandler(Window& window)
{
	for (auto& x : window.ui_element_list)
	{
		x->Draw(window);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnTimerHandler(Window& window, UINT_PTR id)
{
	for (auto& x : window.ui_element_list)
	{
		x->OnTimer(window,id);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnWindowSizeChangedHandler(Window& window)
{
	for (auto& x : window.ui_element_list)
	{
		x->OnWindowSizeChanged(window);
	}
}

void RbsLib::Windows::BasicUI::Window::_MouseMoveHandler(Window& window, int x, int y, int key_status)
{
	for (auto& element : window.ui_element_list)
	{
		element->MouseMove(window, x, y, key_status);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnLeftButtonDownHandler(Window& window, int x, int y, int key_status)
{
	for (auto& element : window.ui_element_list)
	{
		element->OnLeftButtonDown(window, x, y, key_status);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnKeyDownHandler(Window& window, unsigned int vkey, unsigned int param)
{
	for (auto& element : window.ui_element_list)
	{
		element->OnKeyDown(window, vkey, param);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnLeftButtonUpHandler(Window& window, int x, int y, int key_status)
{
	for (auto& element : window.ui_element_list)
	{
		element->OnLeftButtonUp(window, x, y, key_status);
	}
}

void RbsLib::Windows::BasicUI::Window::_OnRightButtonDownHandler(Window& window, int x, int y, int key_status)
{
	for (auto& element : window.ui_element_list)
	{
		element->OnRightButtonDown(window, x, y, key_status);
	}
}


void RbsLib::Windows::BasicUI::Window::OnWindowCreate(void)
{
	this->_OnWindowCreateHandler(*this);
}

void RbsLib::Windows::BasicUI::Window::PaintWindow(void)
{
	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	//检查呈现器是否已初始化
	if (!this->IsD2D1RenderTargetInitialized())
	{
		this->CreateD2D1RenderTarget();
	}
	this->d2d1_render_target->BeginDraw();
	//进行绘图操作

	this->OnPaintHandler(*this);

	//完成绘图
	if (SUCCEEDED(this->d2d1_render_target->EndDraw()))
	{
		//绘图成功
	}
	else
	{
		//绘图失败
		this->ReleaseD2D1RenderTarget();
	}
	EndPaint(hwnd, &ps);
}

void RbsLib::Windows::BasicUI::Window::OnWindowSizeChanged(void)
{
	this->WindowSizeChangedHandler(*this);
}

void RbsLib::Windows::BasicUI::Window::OnTimer(UINT_PTR id)
{
	this->OnTimerHandler(*this, id);
}

void RbsLib::Windows::BasicUI::Window::MouseMove(int x, int y, int Key_status)
{
	this->MouseMoveHandler(*this, x, y, Key_status);
}

void RbsLib::Windows::BasicUI::Window::OnLeftButtonDown(int x, int y, int key_status)
{
	this->OnLeftButtonDownHandler(*this, x, y, key_status);
}

void RbsLib::Windows::BasicUI::Window::OnKeyDown(unsigned int vkey, unsigned int param)
{
	this->OnKeyDownHandler(*this, vkey, param);
}

void RbsLib::Windows::BasicUI::Window::OnLeftButtonUp(int x, int y, int key_status)
{
	this->LeftButtonUpHandler(*this, x, y, key_status);
}

void RbsLib::Windows::BasicUI::Window::OnRightButtonDown(int x, int y, int key_status)
{
	this->RightButtonDownHandler(*this, x, y, key_status);
}


void RbsLib::Windows::BasicUI::Window::ResetD2D1RenderTarget(void)
{
	this->ReleaseD2D1RenderTarget();
}

void RbsLib::Windows::BasicUI::Window::WindowSize(int x, int y, int width, int height)
{
	::MoveWindow(this->hwnd, x, y, width, height, TRUE);
}

RECT RbsLib::Windows::BasicUI::Window::WindowSize() const
{
	RECT rc;
	GetWindowRect(this->hwnd, &rc);
	return rc;
}

HWND RbsLib::Windows::BasicUI::Window::GetHWND(void)
{
	return this->hwnd;
}

void RbsLib::Windows::BasicUI::Window::AddMainThreadTask(const std::function<void(Window&)>& task)
{
	this->main_thread_tasks += task;
}


std::uint32_t RbsLib::Windows::BasicUI::Window::AddUIElement(std::shared_ptr<UIElement> ui_element)
{
	if (ui_element->id != 0)
	{
		throw BasicUIException("UIElement already in window");
	}
	ui_element->id = this->GenerateUIElementID();
	if (ui_element->id == 0)
	{
		throw BasicUIException("UIElement id overflow");
	}
	ui_element->window = this;
	this->ui_element_list.push_back(ui_element);
	ui_element->OnUIElementAdd(*this);
	return ui_element->id;
}

std::shared_ptr<RbsLib::Windows::BasicUI::UIElement> RbsLib::Windows::BasicUI::Window::GetUIElement(std::uint32_t id)
{
	auto x = std::find_if(this->ui_element_list.begin(), this->ui_element_list.end(), [id](const std::shared_ptr<UIElement>& x) {return x->id == id; });
	if (x == this->ui_element_list.end())
	{
		throw BasicUIException("UIElement not found");
	}
	return *x;
}

void RbsLib::Windows::BasicUI::Window::RemoveUIElement(std::uint32_t id)
{
	auto x = std::find_if(this->ui_element_list.begin(), this->ui_element_list.end(), [id](const std::shared_ptr<UIElement>& x) {return x->id == id; });
	if (x == this->ui_element_list.end())
	{
		throw BasicUIException("UIElement not found");
	}
	(*x)->OnUIElementRemove(*this);
	auto t = *x;
	this->ui_element_list.erase(x);
	t->id = 0;
	t->window = nullptr;
}

void RbsLib::Windows::BasicUI::Window::Draw(void)
{
	//::SendMessage(this->hwnd, WM_PAINT, 0, 0);
	//this->PaintWindow();
	InvalidateRect(
		this->hwnd,
		NULL,
		FALSE
	);
}

const RECT& RbsLib::Windows::BasicUI::Window::GetD2D1RenderTargetRect(void) const
{
	return this->BindedRect;
}

RECT RbsLib::Windows::BasicUI::Window::GetRect(void) const
{
	RECT rc;
	GetClientRect(this->hwnd, &rc);
	return rc;
}

RbsLib::Windows::BasicUI::Window::Window(const std::string& window_name, int width, int heigth, Color color,std::uint64_t style)
	:window_name(window_name)
{
	std::string class_name = window_name + "RBSBasicUI";
	WNDCLASSA wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;				//窗口样式;旗标组合;--水平或垂直改变重绘REDRAW;
	wndclass.lpfnWndProc = this->WindowProc;
	wndclass.cbClsExtra = 0;									//为类对象预留的额外计数字节cb;[=couter byte;]
	wndclass.cbWndExtra = 0;									//为窗体预留的额外计数字节cb;
	wndclass.hInstance = 0;							//App实例句柄;--当前窗口实例的ID代号;由Windows提供;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		//Icon图标句柄;--普通应用图标;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);			//Cursor光标句柄;--箭头鼠标指针;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//HBRUSH背景画刷句柄;--选择预设的标准画刷--白色;
	wndclass.lpszMenuName = NULL;									//菜单名;--没有菜单为NULL;
	wndclass.lpszClassName = class_name.c_str();
	if (!RegisterClass(&wndclass))
	{
		throw BasicUIException("RegisterClass failed");
	}
	this->CreateD2D1Factory();
	this->hwnd = CreateWindow(class_name.c_str(),
		window_name.c_str(),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width, heigth,
		NULL, NULL, NULL, NULL);
	if (!this->hwnd)
	{
		UnregisterClass(class_name.c_str(), 0);
		this->d2d1_factory->Release();
		throw BasicUIException("CreateWindow failed");
	}

	this->OnPaintHandler += this->_OnPaintHandler;
	this->OnTimerHandler += this->_OnTimerHandler;
	this->WindowSizeChangedHandler += this->_OnWindowSizeChangedHandler;
	this->MouseMoveHandler += this->_MouseMoveHandler;
	this->OnLeftButtonDownHandler += this->_OnLeftButtonDownHandler;
	this->LeftButtonUpHandler += this->_OnLeftButtonUpHandler;
	this->OnKeyDownHandler += this->_OnKeyDownHandler;
	this->RightButtonDownHandler += this->_OnRightButtonDownHandler;

	RunningWindowsList.insert({ this->hwnd,this });
	this->CreateD2D1RenderTarget();
	
}

RbsLib::Windows::BasicUI::Window::~Window()
{
	RunningWindowsList.erase(this->hwnd);
	DestroyWindow(this->hwnd);
	UnregisterClass((window_name + "RBSBasicUI").c_str(), 0);
	this->ReleaseD2D1RenderTarget();
	this->ReleaseD2D1Factory();
}

void RbsLib::Windows::BasicUI::Window::Show()
{
	SendMessage(this->hwnd, WM_CREATE, 0, 0);
	ShowWindow(this->hwnd, SW_SHOW);
	UpdateWindow(this->hwnd);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

HWND RbsLib::Windows::BasicUI::Window::GetHWND(void) const noexcept
{
	return this->hwnd;
}

void RbsLib::Windows::BasicUI::Window::SetWindowSize(int width, int height)
{
	SetWindowPos(this->hwnd, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void RbsLib::Windows::BasicUI::Window::SetTimer(UINT_PTR id, UINT elapse)
{
	//检查是否已经有相同ID的定时器
	for (auto& x : this->timer_list)
	{
		if (x == id)
		{
			throw BasicUIException("Timer id conflict");
		}
	}
	::SetTimer(this->hwnd, id, elapse, nullptr);
	this->timer_list.push_back(id);
}

void RbsLib::Windows::BasicUI::Window::KillTimer(UINT_PTR id)
{
	//检查是否有这个定时器
	if (std::find(this->timer_list.begin(), this->timer_list.end(), id) == this->timer_list.end())
	{
		throw BasicUIException("Timer not found");
	}
	if (::KillTimer(this->hwnd, id))
	{
		this->timer_list.remove(id);
	}
	else
		throw BasicUIException("Kill timer failed");
}

void RbsLib::Windows::BasicUI::Window::ClearTimer(void)
{
	for (auto& x : this->timer_list)
	{
		::KillTimer(this->hwnd, x);
	}
	this->timer_list.clear();
}



const char* RbsLib::Windows::BasicUI::BasicUIException::what(void) const noexcept
{
	return str.c_str();
}

RbsLib::Windows::BasicUI::BasicUIException::BasicUIException(const std::string& str)
	:str(str)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnUIElementAdd(Window& window)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnUIElementRemove(Window& window)
{
}

void RbsLib::Windows::BasicUI::UIElement::Draw(Window& window)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnTimer(Window&,UINT_PTR)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnWindowSizeChanged(Window& window)
{
}

void RbsLib::Windows::BasicUI::UIElement::MouseMove(Window& window, int x, int y, int key_status)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnLeftButtonDown(Window& window, int x, int y, int key_status)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnLeftButtonUp(Window& window, int x, int y, int key_status)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnRightButtonDown(Window& window, int x, int y, int key_status)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnKeyDown(Window& window, unsigned int vkey, unsigned int param)
{
}

void RbsLib::Windows::BasicUI::UIElement::OnWindowCreate(Window& window)
{
}


#endif

double RbsLib::Windows::BasicUI::GetDistance(const Point& a, const Point& b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

void RbsLib::Windows::BasicUI::Button::CreateTextFormat(ID2D1Factory* factory,const wchar_t* font_family,const wchar_t* font_name, int width, int height, float font_size)
{
	HRESULT hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&this->dwrite_factory)
	);
	if (!SUCCEEDED(hr))
	{
		this->dwrite_factory = nullptr;
		throw BasicUIException("Create dwrite factory failed");
	}

	hr = dwrite_factory->CreateTextFormat(
		font_family,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		font_size,
		L"",
		&this->text_format
	);
	if (!SUCCEEDED(hr))
	{
		this->text_format = nullptr;
		throw BasicUIException("Create text format failed");
	}
	this->text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	this->text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

RbsLib::Windows::BasicUI::Button::Button(const Point& point, const BoxSize& box_size, const std::wstring& text, const D2D1::ColorF& button_color,const D2D1::ColorF& text_color)
	:button_color(button_color), text_color(text_color)
{
	this->point = point;
	this->box_size = box_size;
	this->text = text;
	this->CreateTextFormat(nullptr, L"宋体", L"宋体", box_size.width, box_size.height, 20);
	this->background_brush = nullptr;
	this->text_brush = nullptr;
}

RbsLib::Windows::BasicUI::Button::~Button()
{
	if (this->text_format != nullptr)
	{
		this->text_format->Release();
	}
	if (this->dwrite_factory != nullptr)
	{
		this->dwrite_factory->Release();
	}
	if (this->background_brush != nullptr)
	{
		this->background_brush->Release();
	}
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
	}
}

void RbsLib::Windows::BasicUI::Button::Draw(Window& window)
{
	if (this->text_format && this->background_brush && this->text_brush)
	{
		window.d2d1_render_target->FillRectangle(D2D1::RectF(this->point.x, this->point.y, this->point.x + this->box_size.width, this->point.y + this->box_size.height), this->background_brush);
		window.d2d1_render_target->DrawText(this->text.c_str(), this->text.length(), this->text_format, D2D1::RectF(this->point.x, this->point.y, this->point.x + this->box_size.width, this->point.y + this->box_size.height), this->text_brush);
	}
}

void RbsLib::Windows::BasicUI::Button::Text(const std::wstring& text)
{
	this->text = text;
	//立即无效化窗口区域
	if (this->window != nullptr)
		InvalidateRect(this->window->GetHWND(), nullptr, FALSE);
}

const std::wstring& RbsLib::Windows::BasicUI::Button::Text(void) const
{
	return this->text;
}

void RbsLib::Windows::BasicUI::Button::Move(const Point& point)
{
	this->point = point;
}

void RbsLib::Windows::BasicUI::Button::Size(const BoxSize& box_size)
{
}

void RbsLib::Windows::BasicUI::Button::ButtonColor(const D2D1::ColorF& color)
{
	this->button_color = color;
	if (this->background_brush != nullptr)
	{
		this->background_brush->Release();
		this->background_brush = nullptr;
	}
	this->window->d2d1_render_target->CreateSolidColorBrush(this->button_color, &this->background_brush);
}

D2D1::ColorF RbsLib::Windows::BasicUI::Button::ButtonColor(void) const
{
	return this->button_color;
}

void RbsLib::Windows::BasicUI::Button::OnUIElementAdd(Window& window)
{
	if (this->background_brush != nullptr)
	{
		this->background_brush->Release();
		this->background_brush = nullptr;
	}
	window.d2d1_render_target->CreateSolidColorBrush(this->button_color, &this->background_brush);
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
		this->text_brush = nullptr;
	}
	window.d2d1_render_target->CreateSolidColorBrush(this->text_color, &this->text_brush);
}

void RbsLib::Windows::BasicUI::Button::MouseMove(Window& window, int x, int y, int key_status)
{
	//判断鼠标是否在按钮上
	if (x >= this->point.x && x <= this->point.x + this->box_size.width && y >= this->point.y && y <= this->point.y + this->box_size.height)
	{
		if (this->is_last_mouse_in_button == false)
		{
			if (this->color_lock == false)
			{
				if (this->background_brush != nullptr)
				{
					this->background_brush->Release();
					this->background_brush = nullptr;
				}
				window.d2d1_render_target->CreateSolidColorBrush(this->hover_color, &this->background_brush);
				InvalidateRect(window.GetHWND(), nullptr, FALSE);
			}
		}
		this->is_last_mouse_in_button = true;
	}
	else
	{
		if (this->is_last_mouse_in_button == true)
		{
			if (this->color_lock == false)
			{
				if (this->background_brush != nullptr)
				{
					this->background_brush->Release();
					this->background_brush = nullptr;
				}
				window.d2d1_render_target->CreateSolidColorBrush(this->button_color, &this->background_brush);
				InvalidateRect(window.GetHWND(), nullptr, FALSE);
			}
		}
		this->is_last_mouse_in_button = false;
	}
}

void RbsLib::Windows::BasicUI::Button::OnLeftButtonDown(Window& window, int x, int y, int key_status)
{
	//检查鼠标是否在按钮上
	if (x >= this->point.x && x <= this->point.x + this->box_size.width && y >= this->point.y && y <= this->point.y + this->box_size.height)
	{
		//重新创建画刷
		if (this->background_brush != nullptr)
		{
			this->background_brush->Release();
			this->background_brush = nullptr;
		}
		window.d2d1_render_target->CreateSolidColorBrush(this->click_color, &this->background_brush);
		//设置鼠标状态
		this->is_lbutton_down_in_button = true;
		//锁定颜色
		this->color_lock = true;
		//立即无效化窗口区域
		InvalidateRect(window.GetHWND(), nullptr, FALSE);
	}
	else
	{
		is_last_mouse_in_button = false;
	}
}

void RbsLib::Windows::BasicUI::Button::OnLeftButtonUp(Window& window, int x, int y, int key_status)
{
	//检查鼠标是否在按钮上
	if (x >= this->point.x && x <= this->point.x + this->box_size.width && y >= this->point.y && y <= this->point.y + this->box_size.height)
	{
		//重新创建画刷
		if (this->background_brush != nullptr)
		{
			this->background_brush->Release();
			this->background_brush = nullptr;
		}
		window.d2d1_render_target->CreateSolidColorBrush(this->hover_color, &this->background_brush);
		//调用Click事件
		if (this->is_lbutton_down_in_button)
		{
			EventArgs e{ window,*this };
			this->events.OnClick(e);
		}
		//立即无效化窗口区域
		InvalidateRect(window.GetHWND(), nullptr, FALSE);
		this->is_lbutton_down_in_button = false;
	}
	else if (this->is_lbutton_down_in_button)
	{
		//颜色变化
		//重新创建画刷
		if (this->background_brush != nullptr)
		{
			this->background_brush->Release();
			this->background_brush = nullptr;
		}
		window.d2d1_render_target->CreateSolidColorBrush(this->button_color, &this->background_brush);
		//设置鼠标状态
		this->is_lbutton_down_in_button = false;
		//立即无效化窗口区域
		InvalidateRect(window.GetHWND(), nullptr, FALSE);
	}
	//解锁颜色
	this->color_lock = false;

}

void RbsLib::Windows::BasicUI::Button::OnRightButtonDown(Window& window, int x, int y, int key_status)
{
	if (x >= this->point.x && x <= this->point.x + this->box_size.width && y >= this->point.y && y <= this->point.y + this->box_size.height)
	{
		EventArgs e{ window,*this };
		this->events.OnRightButtonDown(e);
	}
}

RbsLib::Windows::BasicUI::UIRoot::UIRoot(const D2D1::ColorF& color)
	:color(color)
{
	this->brush = nullptr;
}

void RbsLib::Windows::BasicUI::UIRoot::OnWindowCreate(Window& window)
{
	if (this->brush != nullptr)
	{
		this->brush->Release();
		this->brush = nullptr;
	}
	window.d2d1_render_target->CreateSolidColorBrush(this->color, &this->brush);
}


RbsLib::Windows::BasicUI::UIRoot::~UIRoot()
{
	if (this->brush != nullptr)
	{
		this->brush->Release();
	}
}

void RbsLib::Windows::BasicUI::UIRoot::Draw(Window& window)
{
	auto rect = window.GetD2D1RenderTargetRect();
	window.d2d1_render_target->FillRectangle(D2D1::RectF(0, 0, rect.right - rect.left, rect.bottom - rect.top), this->brush);
}

void RbsLib::Windows::BasicUI::Label::CreateTextFormat(ID2D1Factory* factory, const wchar_t* font_family, const wchar_t* font_name, int width, int height, float font_size)
{
	HRESULT hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&this->dwrite_factory)
	);
	if (!SUCCEEDED(hr))
	{
		this->dwrite_factory = nullptr;
		throw BasicUIException("Create dwrite factory failed");
	}
	hr = dwrite_factory->CreateTextFormat(
		font_family,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		font_size,
		L"",
		&this->text_format
	);
	if (!SUCCEEDED(hr))
	{
		this->text_format = nullptr;
		throw BasicUIException("Create text format failed");
	}
	this->text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	this->text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

void RbsLib::Windows::BasicUI::Label::OnUIElementAdd(Window& window)
{
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
		this->text_brush = nullptr;
	}
	window.d2d1_render_target->CreateSolidColorBrush(this->text_color, &this->text_brush);
	if (this->text_format != nullptr)
	{
		this->text_format->Release();
		this->text_format = nullptr;
	}
	this->CreateTextFormat(window.d2d1_factory, L"宋体", L"宋体", this->box_size.width, this->box_size.height, 20);
}

void RbsLib::Windows::BasicUI::Label::OnUIElementRemove(Window& window)
{
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
		this->text_brush = nullptr;
	}
	if (this->text_format != nullptr)
	{
		this->text_format->Release();
		this->text_format = nullptr;
	}
}

RbsLib::Windows::BasicUI::Label::Label(const Point& point, const BoxSize& box_size, const std::wstring& text, const D2D1::ColorF& text_color)
	:text_color(text_color), text(text), box_size(box_size), point(point), text_brush(nullptr), text_format(nullptr), dwrite_factory(nullptr)
{
}

RbsLib::Windows::BasicUI::Label::~Label()
{
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
	}
	if (this->text_format != nullptr)
	{
		this->text_format->Release();
	}
	if (this->dwrite_factory != nullptr)
	{
		this->dwrite_factory->Release();
	}
}

void RbsLib::Windows::BasicUI::Label::Draw(Window& window)
{
	if (this->text_brush != nullptr && this->text_format != nullptr)
	{
		window.d2d1_render_target->DrawText(this->text.c_str(), this->text.length(), this->text_format, D2D1::RectF(this->point.x, this->point.y, this->point.x + this->box_size.width, this->point.y + this->box_size.height), this->text_brush);
	}

}

void RbsLib::Windows::BasicUI::Label::Text(const std::wstring& text)
{
	this->text = text;
	if (this->window)
	{
		InvalidateRect(this->window->GetHWND(), nullptr, FALSE);
	}
}

const std::wstring& RbsLib::Windows::BasicUI::Label::Text(void) const
{
	return this->text;
}

void RbsLib::Windows::BasicUI::Label::Move(const Point& point)
{
	this->point = point;
	if (this->window)
	{
		InvalidateRect(this->window->GetHWND(), nullptr, FALSE);
	}
}

void RbsLib::Windows::BasicUI::Label::Size(const BoxSize& box_size)
{
	this->box_size = box_size;
	if (this->window)
	{
		InvalidateRect(this->window->GetHWND(), nullptr, FALSE);
	}
}

void RbsLib::Windows::BasicUI::Label::TextColor(const D2D1::ColorF& color)
{
	this->text_color = color;
	if (this->text_brush != nullptr)
	{
		this->text_brush->Release();
		this->text_brush = nullptr;
	}
	if (this->window)
	{
		this->window->d2d1_render_target->CreateSolidColorBrush(this->text_color, &this->text_brush);
	}
}

D2D1::ColorF RbsLib::Windows::BasicUI::Label::TextColor(void) const
{
	return this->text_color;
}

const RbsLib::Windows::BasicUI::BoxSize& RbsLib::Windows::BasicUI::Label::Size(void) const
{
	return this->box_size;
}
