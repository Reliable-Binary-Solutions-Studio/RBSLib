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
	switch (message)
	{
	case WM_LBUTTONDOWN:
		self_ref->_OnLeftButtonDownHandler(*self_ref, LOWORD(lParam), HIWORD(lParam), wParam);
		return 0;
	case WM_MOUSEMOVE:
		self_ref->MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
		return 0;
	case WM_TIMER:
		self_ref->OnTimer(wParam);
		return 0;
	case WM_PAINT:
		self_ref->PaintWindow();
		return 0;
	case WM_SIZE:
		self_ref->OnWindowSizeChanged();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		self_ref->_OnKeyDownHandler(*self_ref, wParam, lParam);
		return 0;
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


void RbsLib::Windows::BasicUI::Window::PaintWindow(void)
{
	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	//���������Ƿ��ѳ�ʼ��
	if (!this->IsD2D1RenderTargetInitialized())
	{
		this->CreateD2D1RenderTarget();
	}
	this->d2d1_render_target->BeginDraw();
	//���л�ͼ����

	this->OnPaintHandler(*this);

	//��ɻ�ͼ
	if (SUCCEEDED(this->d2d1_render_target->EndDraw()))
	{
		//��ͼ�ɹ�
	}
	else
	{
		//��ͼʧ��
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


void RbsLib::Windows::BasicUI::Window::AddUIElement(std::shared_ptr<UIElement> ui_element)
{
	this->ui_element_list.push_back(ui_element);
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

RbsLib::Windows::BasicUI::Window::Window(const std::string& window_name, int width, int heigth, Color color)
	:window_name(window_name)
{
	std::string class_name = window_name + "RBSBasicUI";
	WNDCLASSA wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;				//������ʽ;������;--ˮƽ��ֱ�ı��ػ�REDRAW;
	wndclass.lpfnWndProc = this->WindowProc;
	wndclass.cbClsExtra = 0;									//Ϊ�����Ԥ���Ķ�������ֽ�cb;[=couter byte;]
	wndclass.cbWndExtra = 0;									//Ϊ����Ԥ���Ķ�������ֽ�cb;
	wndclass.hInstance = 0;							//Appʵ�����;--��ǰ����ʵ����ID����;��Windows�ṩ;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);		//Iconͼ����;--��ͨӦ��ͼ��;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);			//Cursor�����;--��ͷ���ָ��;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	//HBRUSH������ˢ���;--ѡ��Ԥ��ı�׼��ˢ--��ɫ;
	wndclass.lpszMenuName = NULL;									//�˵���;--û�в˵�ΪNULL;
	wndclass.lpszClassName = class_name.c_str();
	if (!RegisterClass(&wndclass))
	{
		throw BasicUIException("RegisterClass failed");
	}
	this->hwnd = CreateWindow(class_name.c_str(),
		window_name.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width, heigth,
		NULL, NULL, NULL, NULL);
	this->OnPaintHandler += this->_OnPaintHandler;
	this->OnTimerHandler += this->_OnTimerHandler;
	this->WindowSizeChangedHandler += this->_OnWindowSizeChangedHandler;
	this->MouseMoveHandler += this->_MouseMoveHandler;
	this->OnLeftButtonDownHandler += this->_OnLeftButtonDownHandler;
	RunningWindowsList.insert({ this->hwnd,this });
	this->CreateD2D1Factory();
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
	//����Ƿ��Ѿ�����ͬID�Ķ�ʱ��
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
	//����Ƿ��������ʱ��
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

void RbsLib::Windows::BasicUI::UIElement::OnKeyDown(Window& window, unsigned int vkey, unsigned param)
{
}

#endif

double RbsLib::Windows::BasicUI::GetDistance(const Point& a, const Point& b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}
