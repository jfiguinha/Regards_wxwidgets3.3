#pragma once
#include <ToolbarWindow.h>
using namespace Regards::Window;

class CRegardsBitmap;

namespace Regards::Viewer
{
	class CToolbar : public CToolbarWindow
	{
	public:
		CToolbar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical);
		~CToolbar() override;
		void SetUpdateVisible(const bool& isVisible);

	private:
		void EventManager(const int& id) override;

		
		std::unique_ptr<CToolbarButton> imageNewVersion = nullptr;
		std::unique_ptr<CToolbarButton> scanner = nullptr;
		std::unique_ptr<CToolbarButton> print = nullptr;
		std::unique_ptr<CToolbarButton> editor = nullptr;
		std::unique_ptr<CToolbarButton> export_button = nullptr;
		std::unique_ptr<CToolbarButton> export_diaporama_button = nullptr;
		std::unique_ptr<CToolbarButton> imageFirst = nullptr;
	};
}
