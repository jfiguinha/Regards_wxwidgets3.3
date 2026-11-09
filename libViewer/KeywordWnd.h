#pragma once
#include <TreeControl.h>
#include <TreeWithScrollbar.h>
#include <wx/textctrl.h>
#include <wx/weakref.h>
using namespace Regards::Window;

#define WM_ADDCRITERIA 1
#define WM_REFRESHCRITERIA 2

class CThemeTree;

namespace Regards
{
	namespace Window
	{
		class CTreeElementControlInterface;
		class CTreeDataCategory;
		class CWindowMain;
	}

	namespace Viewer
	{
		class CMainParam;

		class CKeywordWnd : public CTreeControl
		{
		public:
			CKeywordWnd(CWindowMain* windowMain, CThemeTree* theme, CTreeElementControlInterface* interfaceControl);
			~CKeywordWnd();
			void Init(const wxString& filename);
			void UpdateScreenRatio() override;

			void SlidePosChange(CTreeElement* treeElement, const int& position, CTreeElementValue* value,
				const wxString& key) override
			{};

		protected:
			void Resize();
			void ClickOnElement(CPositionElement* element, wxWindow* window, const int& x, const int& y,
				const int& posLargeur, const int& posHauteur) override;

		private:
			//Draw Tree (style moderne : fond uni, filets - voir CTreeControl::EnableModernStyle)
			void OnModernRelayout() override;
			void RenderTree();
			void RenderChildTree(tree<CTreeData*>::sibling_iterator& parent);

			CPositionElement* RenderDelete(
				CTreeDataCategory* data,
				int& xPos,
				int& yPos,
				bool visible);

			void InitKeyWordCategorie(tree<CTreeData*>::iterator parent, int numParent);
			void NotifyCriteriaChange(wxWindow* window);
			void AddKeyword(wxWindow* window);

			//Edition du libellé d'un critère (clic sur le libellé)
			void BeginEditKeyword(CPositionElement* element, wxWindow* window, int scrollX, int scrollY);
			void CommitEdit();
			void CancelEdit();
			void CloseEditor();
			void OnEditEnter(wxCommandEvent& event);
			void OnEditKillFocus(wxFocusEvent& event);
			void OnEditKeyDown(wxKeyEvent& event);

			//Gestion des check box
			bool GetCheckState(const wxString& exifKey, const wxString& key) const;
			bool GetTriangleState(const wxString& exifKey, const wxString& key) const;
			void StoreCheckState(const wxString& exifKey, const wxString& key, bool state);

			//Variable
			int numCatalog = 0;
			int idElement = 0;
			int yPos = 0;
			wxString stateValue = "";
			wxString stateTriangleValue = "";
			CWindowMain* windowMain = nullptr;
			int numPhotoId = 0;

			//Edition inline du libellé
			wxWeakRef<wxTextCtrl> editCtrl;
			int editedCategorie = -1;
			wxString editedKey = "";
		};
	}
}