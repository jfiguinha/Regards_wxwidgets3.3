#pragma once
#pragma once
#include <ToolbarWindow.h>
#include <ToolbarInterface.h>
#include <ToolbarButton.h>
using namespace Regards::Window;


namespace Regards::Editor
{
	class CToolbarTools : public CToolbarWindow
	{
	public:
		CToolbarTools(wxWindow* parent, wxWindowID id, const CThemeToolbar& theme, wxWindow* frameToSend, const bool& vertical);
		~CToolbarTools() = default;

	private:

		void Resize() override;
		void EventManager(const int& id) override;

		wxWindow* frameToSend;

		std::unique_ptr<CToolbarButton> selection = nullptr;
		std::unique_ptr<CToolbarButton> selectionCrop = nullptr;
		std::unique_ptr<CToolbarButton> selectMove = nullptr;
		std::unique_ptr<CToolbarButton> drawingTools = nullptr;
		std::unique_ptr<CToolbarButton> geometricTools = nullptr;
		std::unique_ptr<CToolbarButton> textTools = nullptr;
		std::unique_ptr<CToolbarButton> zoomTools = nullptr;
		std::unique_ptr<CToolbarButton> pictureMoveTools = nullptr;
		std::unique_ptr<CToolbarButton> paintbucketTools = nullptr;
		std::unique_ptr<CToolbarButton> gradientTools = nullptr;
	};
}
