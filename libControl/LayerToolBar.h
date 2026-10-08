#pragma once
#include <ToolbarWindow.h>
using namespace Regards::Window;

namespace Regards::Control
{
	class CListPicture;

	class CLayerToolBar : public CToolbarWindow
	{
	public:
		CLayerToolBar(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, const bool& vertical);
		~CLayerToolBar() = default;


	private:
		void EventManager(const int& id) override;
	
		std::unique_ptr<CToolbarButton> newlayer = nullptr;
		std::unique_ptr<CToolbarButton> deleteLayer = nullptr;
		std::unique_ptr<CToolbarButton> copyLayer = nullptr;
		std::unique_ptr<CToolbarButton> fusionLayer = nullptr;
		std::unique_ptr<CToolbarButton> moveupLayer = nullptr;
		std::unique_ptr<CToolbarButton> movedownLayer = nullptr;
		std::unique_ptr<CToolbarButton> propertiesLayer = nullptr;
	};
}
