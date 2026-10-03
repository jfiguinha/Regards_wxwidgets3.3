#pragma once
#include <ToolbarWindow.h>
#include <ToolbarInterface.h>
#include <ToolbarTexte.h>
using namespace Regards::Window;


namespace Regards::Viewer
{
	class CToolbarPhoto : public CToolbarWindow
	{
	public:
		CToolbarPhoto(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, CToolbarInterface* toolbarInterface,
		              const bool& vertical);
		~CToolbarPhoto() = default;
		void SetFolderPush();
		void SetCriteriaPush();
#if wxUSE_DRAWINGTOOLS
		void SetDrawingPush();
#endif
		void ShowBitmapToolbar(const bool& showBitmapToolbar);
	private:
		void Resize() override;
		void EventManager(const int& id) override;

		CToolbarInterface* toolbarInterface;
		std::unique_ptr<CToolbarTexte> folder;
#if wxUSE_DRAWINGTOOLS
		std::unique_ptr<CToolbarTexte> drawing;
#endif
		std::unique_ptr<CToolbarTexte> criteria;
	};
}
