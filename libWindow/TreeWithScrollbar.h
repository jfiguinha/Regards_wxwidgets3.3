#pragma once
#include <WindowMain.h>
#include "TitleBarInterface.h"
#include "TitleBar.h"
using namespace std;

namespace Regards::Window
{
	class CTreeWindow;
	class CScrollbarWnd;
	class CTitleBar;

	class CTreeWithScrollbar : public CWindowMain, CTitleBarInterface
	{
	public:
		CTreeWithScrollbar(const wxString& windowName, wxWindow* parent, wxWindowID id,
		                   const CThemeScrollBar& themeScroll, const CThemeTree& theme, const wxString& label = "",
		                   const bool& showTitle = false);
		~CTreeWithScrollbar(void) = default;
		void UpdateScreenRatio() override;

		void ClosePane() override
		{
		};

		void RefreshPane() override
		{
		};
		void SetLabel(const wxString& label) override;

	protected:
		void Resize() override;
		bool showTitle = false;

		
		std::unique_ptr<CTitleBar> titleBar;
		std::unique_ptr<CTreeWindow> treeWindow;
		std::unique_ptr<CScrollbarWnd> scrollWindow;
	};
}
