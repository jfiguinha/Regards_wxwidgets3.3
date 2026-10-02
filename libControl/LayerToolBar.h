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
		void PostEvent(wxEventType type);

	
		std::unique_ptr<CToolbarButton> deleteButton = nullptr;
		std::unique_ptr<CToolbarButton> copy = nullptr;
		std::unique_ptr<CToolbarButton> plus = nullptr;
	};
}
