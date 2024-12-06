#pragma once
#include "../BaseType.h"
#ifdef WIN32
#include "BasicUI.h"
#include <vector>
#include <list>
namespace RbsLib::Windows::Graph
{
	class Plot
	{
	private:
		std::list<std::vector<double>> x_list;
		std::list<std::vector<double>> y_list;
		std::list<D2D1::ColorF> color_list;
		std::list<std::vector<double>> scatter_x_list;
		std::list<std::vector<double>> scatter_y_list;
		std::list<D2D1::ColorF> scatter_color_list;
		class PlotElement :public BasicUI::UIElement
		{
		private:
			std::list<std::vector<double>> x_list;
			std::list<std::vector<double>> y_list;
			std::list<D2D1::ColorF> color_list;
			std::list<std::vector<double>> scatter_x_list;
			std::list<std::vector<double>> scatter_y_list;
			std::list<D2D1::ColorF> scatter_color_list;
		public:
			PlotElement(const std::list<std::vector<double>>& x_list, const std::list<std::vector<double>>& y_list, const std::list<D2D1::ColorF>& color_list, const std::list<std::vector<double>>& scatter_x_list, const std::list<std::vector<double>>& scatter_y_list, const std::list<D2D1::ColorF>& scatter_color_list);
			void Draw(RbsLib::Windows::BasicUI::Window& window) override;
		};
	public:
		void AddPlot(const std::vector<double>& x, const std::vector<double>& y, D2D1::ColorF);
		void AddScatter(const std::vector<double>& x, const std::vector<double>& y, D2D1::ColorF);
		void Show(const std::string& name);
	};
}

#endif
