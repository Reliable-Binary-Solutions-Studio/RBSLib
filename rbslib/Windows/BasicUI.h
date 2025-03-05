#pragma once
#include "../BaseType.h"
#ifdef WIN32
#include <string>
#include <Windows.h>
#include <exception>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <list>
#include "../Function.h"
#include <memory>
namespace RbsLib::Windows::BasicUI
{
	class Window;
	class UIElement;
	struct Point
	{
		int x;
		int y;
	};
	struct BoxSize
	{
		int width;
		int height;
	};
	class BasicUIException :public std::exception
	{
		std::string str;
	public:
		const char* what(void)const noexcept override;
		BasicUIException(const std::string& str);
	};
	enum class Color
	{
		NONE = 0,
		WHITE = 1,
		BLACK = 2
	};
	class Window/*若要为窗口过程添加Handle，请继承Window并在构造函数中添加Handle*/
	{
	private:
		HWND hwnd;
		std::list<UINT_PTR> timer_list;
		std::list<std::shared_ptr<UIElement>> ui_element_list;
		RECT BindedRect;
		RbsLib::Function::Function<void(Window&)> main_thread_tasks;
		
		Window(HWND hwnd)noexcept;

		static auto CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;

		void CreateD2D1Factory(void);
		void CreateD2D1RenderTarget(void);
		void ReleaseD2D1RenderTarget(void);
		void ReleaseD2D1Factory(void);
		bool IsD2D1RenderTargetInitialized(void) const noexcept;

		std::uint32_t GenerateUIElementID(void);

		//默认的消息处理函数
		static void _OnWindowCreateHandler(Window& window);
		static void _OnPaintHandler(Window& window);
		static void _OnTimerHandler(Window& window, UINT_PTR id);
		static void _OnWindowSizeChangedHandler(Window& window);
		static void _MouseMoveHandler(Window& window, int x, int y, int key_status);
		static void _OnLeftButtonDownHandler(Window& window, int x, int y, int key_status);
		static void _OnKeyDownHandler(Window& window, unsigned int vkey, unsigned int param);
		static void _OnLeftButtonUpHandler(Window& window, int x, int y, int key_status);
		static void _OnRightButtonDownHandler(Window& window, int x, int y, int key_status);
		//消息循环处理函数
		void OnWindowCreate(void);
		void PaintWindow(void);
		void OnWindowSizeChanged(void);
		void OnTimer(UINT_PTR id);
		void MouseMove(int x, int y,int Key_status);
		void OnLeftButtonDown(int x, int y, int key_status);
		void OnKeyDown(unsigned int vkey, unsigned int param);
		void OnLeftButtonUp(int x, int y, int key_status);//鼠标左键弹起
		void OnRightButtonDown(int x, int y, int key_status);//鼠标右键按下
		
		
	public:
		ID2D1Factory* d2d1_factory = nullptr;
		ID2D1HwndRenderTarget* d2d1_render_target = nullptr;
		std::string window_name;

		//回调列表
		RbsLib::Function::Function<void(Window&)> WindowSizeChangedHandler;
		RbsLib::Function::Function<void(Window&)> OnPaintHandler;
		RbsLib::Function::Function<void(Window&, UINT_PTR)> OnTimerHandler;
		RbsLib::Function::Function<void(Window&, int x, int y, int key_status)> MouseMoveHandler;
		RbsLib::Function::Function<void(Window&,int x,int y,int key_status)> OnLeftButtonDownHandler;
		RbsLib::Function::Function<void(Window&, unsigned int, unsigned int)> OnKeyDownHandler;
		RbsLib::Function::Function<void(Window&, int x, int y, int key_status)> LeftButtonUpHandler;
		RbsLib::Function::Function<void(Window&, int x, int y, int key_status)> RightButtonDownHandler;

		Window(const std::string& window_name, int width, int heigth, Color color,std::uint64_t style = WS_OVERLAPPEDWINDOW);
		Window(const Window&) = delete;
		~Window();
		void Show();
		HWND GetHWND(void) const noexcept;
		void SetWindowSize(int width, int height);
		void SetTimer(UINT_PTR id, UINT elapse);
		void KillTimer(UINT_PTR id);
		void ClearTimer(void);
		std::uint32_t AddUIElement(std::shared_ptr<UIElement> ui_element);
		std::shared_ptr<UIElement> GetUIElement(std::uint32_t id);
		void RemoveUIElement(std::uint32_t id);
		void Draw(void);//立即无效化当前呈现目标
		const RECT& GetD2D1RenderTargetRect(void) const;
		RECT GetRect(void) const;
		void ResetD2D1RenderTarget(void);
		void WindowSize(int x,int y,int width, int height);
		RECT WindowSize()const;
		HWND GetHWND(void);

		void AddMainThreadTask(const std::function<void(Window&)>& task);
	};
	class UIElement
	{
	protected:
		
	public:
		int id = 0;
		Window* window = nullptr;
		virtual void OnUIElementAdd(Window& window);
		virtual void OnUIElementRemove(Window& window);
		virtual void Draw(Window& window);
		virtual void OnTimer(Window& window,UINT_PTR timer_id);
		virtual void OnWindowSizeChanged(Window& window);
		virtual void MouseMove(Window& window, int x, int y, int key_status);
		virtual void OnLeftButtonDown(Window& window, int x, int y, int key_status);
		virtual void OnLeftButtonUp(Window& window, int x, int y, int key_status);
		virtual void OnRightButtonDown(Window& window, int x, int y, int key_status);
		virtual void OnKeyDown(Window& window, unsigned int vkey, unsigned int param);
		virtual void OnWindowCreate(Window& window);
	};
	class EventArgs
	{
	public:
		Window& window;
		UIElement& sender;
	};
	class Events {};
	using EventHandle = RbsLib::Function::Function<void(EventArgs&)>;
	class UIRoot :public UIElement
	{
	private:
		D2D1::ColorF color;
		ID2D1SolidColorBrush* brush;
	public:
		UIRoot(const D2D1::ColorF& color);
		void OnWindowCreate(Window& window) override;
		~UIRoot();
		void Draw(Window& window) override;
	};

	class Button :public UIElement
	{
	private:
		Point point;
		BoxSize box_size;
		std::wstring text;
		IDWriteFactory* dwrite_factory;
		IDWriteTextFormat* text_format;
		ID2D1SolidColorBrush* background_brush,*text_brush;
		bool color_lock = false;
		bool is_lbutton_down_in_button = false;
		bool is_last_mouse_in_button = false;
		D2D1::ColorF button_color,hover_color=D2D1::ColorF::DarkGray,click_color = D2D1::ColorF::Gray, text_color;
		void CreateTextFormat(ID2D1Factory* factory,const wchar_t* font_family,const wchar_t* font_name,
			int width,int height,
			float font_size);
		void OnUIElementAdd(Window& window) override;
		void MouseMove(Window& window, int x, int y, int key_status) override;
		void OnLeftButtonDown(Window& window, int x, int y, int key_status) override;
		void OnLeftButtonUp(Window& window, int x, int y, int key_status) override;
		void OnRightButtonDown(Window& window, int x, int y, int key_status) override;
	public:
		Button(const Point& point, const BoxSize& box_size, const std::wstring& text,const D2D1::ColorF& button_color = D2D1::ColorF::LightGray, const D2D1::ColorF&text_color = D2D1::ColorF::Black);
		~Button();
		void Draw(Window&window) override;
		void Text(const std::wstring& text);
		const std::wstring& Text(void) const;
		void Move(const Point& point);
		void Size(const BoxSize& box_size);
		void ButtonColor(const D2D1::ColorF& color);
		D2D1::ColorF ButtonColor(void) const;
		const BoxSize& Size(void) const;
		class ButtonEvents :public Events
		{
		public:
			EventHandle OnClick;
			EventHandle OnRightButtonDown;
		} events;
		
	};
	class Label :public UIElement
	{
	private:
		Point point;
		BoxSize box_size;
		std::wstring text;
		IDWriteFactory* dwrite_factory;
		IDWriteTextFormat* text_format;
		ID2D1SolidColorBrush* text_brush;
		D2D1::ColorF text_color;
		void CreateTextFormat(ID2D1Factory* factory, const wchar_t* font_family, const wchar_t* font_name,
			int width, int height,
			float font_size);
		void OnUIElementAdd(Window& window) override;
		void OnUIElementRemove(Window& window) override;
		void Draw(Window& window) override;
	public:
		Label(const Point& point, const BoxSize& box_size, const std::wstring& text, const D2D1::ColorF& text_color = D2D1::ColorF::Black);
		~Label();
		void Text(const std::wstring& text);
		const std::wstring& Text(void) const;
		void Move(const Point& point);
		void Size(const BoxSize& box_size);
		void TextColor(const D2D1::ColorF& color);
		D2D1::ColorF TextColor(void) const;
		const BoxSize& Size(void) const;
	};
	double GetDistance(const Point& a, const Point& b);
}
#endif