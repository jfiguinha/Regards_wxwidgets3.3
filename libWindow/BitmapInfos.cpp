#include "header.h"
#include "BitmapInfos.h"
#include <FileUtility.h>
#include <LibResource.h>
#include <ConvertUtility.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/toplevel.h>
#include <memory>
#include <RegardsConfigParam.h>
#include <ParamInit.h>
#include <SqlPhotos.h>
#include <FileGeolocation.h>
#include <GpsEngine.h>
#include <wx/filename.h>
#include <MetadataExiv2.h>
#include <appcontext.h>
#include <algorithm>
extern AppContext application_context;

#define GPS_TIME 1000

using namespace Regards::Window;
using namespace Regards::Sqlite;
using namespace Regards::Internet;
using namespace Regards::exiv2;
using namespace std;

namespace
{
	constexpr int HEADER_MARGIN = 24; // marge gauche/droite du texte
	constexpr int HEADER_LINE_GAP = 2; // espace entre le nom du fichier et la date
	constexpr int BUTTON_MAX_SIZE = 40; // bouton plein ecran
	constexpr int BUTTON_MARGIN = 16; // marge droite du bouton

	// Raccourcit le texte avec des points de suspension pour qu'il tienne dans maxWidth
	wxString FitText(wxDC* dc, const wxString& text, const CThemeFont& font, const int& maxWidth)
	{
		if (text.IsEmpty() || CWindowMain::GetSizeTexte(dc, text, font).x <= maxWidth)
			return text;

		wxString result = text;
		while (result.Length() > 1)
		{
			result.RemoveLast();
			const wxString candidate = result + L"...";
			if (CWindowMain::GetSizeTexte(dc, candidate, font).x <= maxWidth)
				return candidate;
		}
		return result;
	}
}

struct InfosGps
{
	CWindowMain* window;
	wxString filename;
	wxString gpsInfos;
};

wxDEFINE_EVENT(wxEVENT_GPSINFOSLOCAL, wxCommandEvent);

CBitmapInfos::CBitmapInfos(wxWindow* parent, wxWindowID id, const CThemeBitmapInfos& theme)
	: CWindowMain("CBitmapInfos", parent, id)
{
	CListOfWindow* fileGeolocalisation = CGpsEngine::getInstance();
	gpsInfosUpdate = false;
	bitmapInfosTheme = theme;
	threadGps = nullptr;
	fileGeolocalisation->AddWindow(this);
	listProcessWindow.push_back(this);
	gpsTimer = std::make_unique<wxTimer>(this, wxTIMER_EXIT);
	Connect(wxEVT_PAINT, wxPaintEventHandler(CBitmapInfos::on_paint));
	Connect(wxEVT_MOTION, wxMouseEventHandler(CBitmapInfos::OnHeaderMouseMove));
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CBitmapInfos::OnHeaderMouseLeave));
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CBitmapInfos::OnHeaderLButtonDown));
	Connect(wxTIMER_EXIT, wxEVT_TIMER, wxTimerEventHandler(CBitmapInfos::OnStartGps), nullptr, this);
	Connect(wxEVENT_UPDATEGPSINFOS, wxCommandEventHandler(CBitmapInfos::OnUpdateGpsInfos));
	Connect(wxEVENT_GPSINFOSLOCAL, wxCommandEventHandler(CBitmapInfos::UpdateGpsInfosLocal));
}

bool CBitmapInfos::GetProcessEnd()
{
	// printf("CBitmapInfos::GetProcessEnd() \n");
	if (gpsTimer->IsRunning())
		gpsTimer->Stop();

	if (!(threadGps == nullptr))
		return false;

	return true;
}

void CBitmapInfos::OnStartGps(wxTimerEvent& event)
{
	if (application_context.isGPsAvailable)
	{
		if (!threadGps)
		{
			InfosGps* infos = new InfosGps();
			infos->window = this;
			infos->filename = filename;
			threadGps = std::make_unique<thread>(GetGpsInfos, infos);
		}
		else
		{
			gpsTimer->StartOnce(GPS_TIME);
		}

	}
}


void CBitmapInfos::GetGpsInfos(void* data)
{
	wxString urlServer = "";
	wxString apiKey = "";
	//Géolocalisation
	CRegardsConfigParam* param = CParamInit::getInstance();
	if (param != nullptr)
	{
		urlServer = param->GetGeoLocUrlServer();
		apiKey = param->GetApiKey();
	}

	InfosGps* infosGps = static_cast<InfosGps*>(data);

	if (infosGps != nullptr)
	{
		if (wxFileName::FileExists(infosGps->filename))
		{
			wxString notGeo = CLibResource::LoadStringFromResource("LBLNOTGEO", 1);
			CFileGeolocation fileGeolocalisation(urlServer, apiKey);
			fileGeolocalisation.SetFile(infosGps->filename, notGeo);
			infosGps->gpsInfos = fileGeolocalisation.Geolocalize();
			//infosGps->gpsInfos = fileGeolocalisation.GetGpsInformation();
		}
	}

	wxCommandEvent event(wxEVENT_GPSINFOSLOCAL);
	event.SetClientData(infosGps);
	wxPostEvent(infosGps->window, event);

}

void CBitmapInfos::UpdateGpsInfosLocal(wxCommandEvent& event)
{
	InfosGps* infosGps = (InfosGps*)event.GetClientData();
	if (threadGps && threadGps->joinable())
	{
		threadGps->join();
	}
	threadGps.reset();

	if (infosGps->filename == filename)
	{
		gpsInfos = infosGps->gpsInfos;


		if (gpsInfos == "")
			gpsTimer->StartOnce(GPS_TIME);
		else
			needToRefresh = true;
	}

	delete infosGps;
}

void CBitmapInfos::OnUpdateGpsInfos(wxCommandEvent& event)
{
	wxString urlServer = "";
	wxString apiKey = "";
	//Géolocalisation
	CRegardsConfigParam* param = CParamInit::getInstance();
	if (param != nullptr)
	{
		urlServer = param->GetGeoLocUrlServer();
		apiKey = param->GetApiKey();
	}

	auto filename = static_cast<wxString*>(event.GetClientData());
	int typeData = event.GetInt();
	if (filename != nullptr)
	{
		if (*filename == this->filename)
		{
			wxString notGeo = CLibResource::LoadStringFromResource("LBLNOTGEO", 1);
			CFileGeolocation fileGeolocalisation(urlServer, apiKey);
			fileGeolocalisation.SetFile(*filename, notGeo);
			if (typeData == 1)
			{
				//printf("CBitmapInfos OnTimerGPSUpdate \n");
				gpsInfos = fileGeolocalisation.GetGpsInformation();
			}
			else if (typeData == 2)
			{
				dateInfos = fileGeolocalisation.GetDateTimeInfos();
				SetDateInfos(dateInfos, '.');
			}
			delete filename;
		}
	}

	needToRefresh = true;
}

void CBitmapInfos::SetFilename(const wxString& libelle)
{
	//printf("SetFilename \n");
	if (filename != libelle)
	{
		if (gpsTimer->IsRunning())
			gpsTimer->Stop();

		gpsInfos = "";
		filename = libelle;
		gpsInfosUpdate = false;
		processIdle = true;
		UpdateData();
	}
}


wxString CBitmapInfos::GenerateDefaultTimeStamp()
{
	wxFileName file = wxFileName(filename);
	wxDateTime dt = file.GetModificationTime();
	wxString str = dt.Format(wxT("%Y-%m-%d"), wxDateTime::CET);

	return str;
}

void CBitmapInfos::UpdateData()
{
	bool hasGps = false;
	bool hasDataTime = false;
	wxString latitudeGps;
	wxString longitudeGps;
	wxString dateTimeInfos;

	//printf("UpdateData \n");
	gpsInfos = "";

	CMetadataExiv2 metadata(filename);
	if (metadata.HasExif())
	{
		dateInfos = metadata.GetCreationDate();
		metadata.ReadPicture(hasGps, hasDataTime, dateTimeInfos, latitudeGps, longitudeGps);
	}

	//Recherche dans la base de données des critères sur le fichier
	CSqlPhotos sqlPhotos;
	int insertValue = sqlPhotos.GetCriteriaInsert(filename);
	if (insertValue > 0)
	{
		CriteriaVector criteriaList;
		sqlPhotos.GetPhotoCriteria(&criteriaList, filename);

		for (CCriteria& criteria : criteriaList)
		{
			if (criteria.GetCategorieId() == 1)
			{
				gpsInfos = criteria.GetLibelle();
				if (gpsInfos.find(application_context.special_key) == 0)
				{
					gpsInfos = "";
				}
			}
			else if (criteria.GetCategorieId() == 3)
			{
				dateInfos = criteria.GetLibelle();
			}
		}

		if (dateInfos == "")
		{
			wxString str = GenerateDefaultTimeStamp();
			SetDateInfos(str, '-');
		}
		else
			SetDateInfos(dateInfos, '.');
	}
	else
	{
		if (dateInfos == "")
		{
			wxString str = GenerateDefaultTimeStamp();
			SetDateInfos(str, '-');
		}
		else
			SetDateInfos(dateInfos, '.');

		gpsInfos = "";


	}

	if (hasGps && gpsInfos == "" && application_context.isGPsAvailable)
	{
		gpsTimer->StartOnce(100);
	}

	needToRefresh = true;
}

int CBitmapInfos::Dayofweek(int d, int m, int y)
{
	static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	y -= m < 3;
	return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

void CBitmapInfos::SetDateInfos(const wxString& dataInfos, char seperator)
{
	wxString listMonth = CLibResource::LoadStringFromResource("LBLMONTHNAME", 1);
	wxString listDay = CLibResource::LoadStringFromResource("LBLDAYNAME", 1);
	vector<wxString> MonthName = CConvertUtility::split(listMonth, ',');
	vector<wxString> DayName = CConvertUtility::split(listDay, ',');
	vector<wxString> vDateTime = CConvertUtility::split(dataInfos, seperator);
	if (vDateTime.size() >= 3)
	{
		int year = CConvertUtility::ToInt(vDateTime[0]);
		int month = CConvertUtility::ToInt(vDateTime[1]);
		int day = CConvertUtility::ToInt(vDateTime[2]);

		if (year == 0 || month == 0 || day == 0)
			dateInfos = "Invalid Date Infos";
		else
		{
			int ijour = Dayofweek(day, month, year);
			dateInfos = wxString::Format(
				"%s %d %s, %d",
				DayName.at(ijour),
				day,
				MonthName.at(month - 1),
				year);
		}
	}
}

CBitmapInfos::~CBitmapInfos()
{
	if (gpsTimer)
		gpsTimer->Stop();

	if (threadGps && threadGps->joinable())
		threadGps->join();

	CListOfWindow* fileGeolocalisation = CGpsEngine::getInstance();
	if (fileGeolocalisation)
		fileGeolocalisation->RemoveWindow(this);
}

int CBitmapInfos::GetHeight()
{
	return bitmapInfosTheme.GetHeight();
}

void CBitmapInfos::UpdateScreenRatio()
{
	Resize();
}

void CBitmapInfos::Redraw()
{
	wxClientDC dc(this);
	DrawInformations(&dc);
}

void CBitmapInfos::DrawInformations(wxDC* dc)
{
	double scale_factor = 1.0f;
	wxRect rc = GetWindowRect();
	FillRect(dc, rc, bitmapInfosTheme.colorBack);
	wxString message;
	wxString libelle = CFileUtility::GetFileName(filename);

	// Titre : nom du fichier
	CThemeFont font = bitmapInfosTheme.themeFont;
	font.SetFontSize(bitmapInfosTheme.themeFont.GetFontSize() / scale_factor);

	// Sous-titre : date et geolocalisation, un peu plus petit
	CThemeFont fontInfos = bitmapInfosTheme.themeFont;
	fontInfos.SetFontSize(bitmapInfosTheme.themeFont.GetFontSize() * 0.85 / scale_factor);

	if (gpsInfos != L"")
	{
		wchar_t seperator = '.';
		vector<wxString> listGeo = CConvertUtility::split(gpsInfos, seperator);
		message = dateInfos;
		for (auto it = listGeo.rbegin();
			it != listGeo.rend();
			++it)
		{
			message += "," + *it;
		}
	}
	else
	{
		message = dateInfos;
	}

	// Alignement a gauche, texte tronque s'il depasse
	UpdateFullscreenRect();
	const int rightReserved = fullscreenRect.IsEmpty() ? HEADER_MARGIN : fullscreenRect.width + BUTTON_MARGIN * 2;
	const int maxWidth = std::max(0, GetWindowWidth() - HEADER_MARGIN - rightReserved);
	libelle = FitText(dc, libelle, font, maxWidth);
	message = FitText(dc, message, fontInfos, maxWidth);

	// Bloc de deux lignes centre verticalement (plus de chevauchement ni de texte coupe)
	const wxSize sizeTitle = GetSizeTexte(dc, libelle, font);
	const wxSize sizeInfos = GetSizeTexte(dc, message, fontInfos);
	const bool hasInfos = !message.IsEmpty();
	const int totalHeight = sizeTitle.y + (hasInfos ? HEADER_LINE_GAP + sizeInfos.y : 0);
	const int yStart = std::max(0, (GetWindowHeight() - totalHeight) / 2);

	DrawTexte(dc, libelle, HEADER_MARGIN, yStart, font);
	if (hasInfos)
		DrawTexte(dc, message, HEADER_MARGIN, yStart + sizeTitle.y + HEADER_LINE_GAP, fontInfos);

	DrawFullscreenButton(dc);
}

void CBitmapInfos::SetFullscreenCallback(std::function<void()> callback)
{
	fullscreenCallback = std::move(callback);
}

// Bouton carre, aligne a droite et centre verticalement
void CBitmapInfos::UpdateFullscreenRect()
{
	const int size = std::min(BUTTON_MAX_SIZE, GetWindowHeight() - 8);
	if (size < 20)
	{
		fullscreenRect = wxRect(0, 0, 0, 0);
		return;
	}
	fullscreenRect = wxRect(GetWindowWidth() - size - BUTTON_MARGIN, (GetWindowHeight() - size) / 2, size, size);
}

// Icone "ecran" vectorielle, meme trace que la maquette (grille 24x24)
void CBitmapInfos::DrawFullscreenButton(wxDC* dc)
{
	if (fullscreenRect.IsEmpty())
		return;

	std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromUnknownDC(*dc));
	if (!gc)
		return;

	const wxColour back = bitmapInfosTheme.colorBack;
	const int luminance = (back.Red() * 299 + back.Green() * 587 + back.Blue() * 114) / 1000;
	const bool dark = luminance < 128;

	if (fullscreenHover)
	{
		gc->SetPen(*wxTRANSPARENT_PEN);
		gc->SetBrush(wxBrush(dark ? wxColour(255, 255, 255, 26) : wxColour(0, 0, 0, 26)));
		gc->DrawRoundedRectangle(fullscreenRect.x, fullscreenRect.y, fullscreenRect.width, fullscreenRect.height, 10);
	}

	wxColour iconColour;
	if (dark)
		iconColour = fullscreenHover ? wxColour(255, 255, 255) : wxColour(201, 206, 216);
	else
		iconColour = fullscreenHover ? wxColour(0, 0, 0) : wxColour(70, 76, 88);

	wxPen pen(iconColour, 1.75);
	pen.SetCap(wxCAP_ROUND);
	pen.SetJoin(wxJOIN_ROUND);
	gc->SetPen(pen);
	gc->SetBrush(*wxTRANSPARENT_BRUSH);

	const double iconSize = 22.0;
	const double scale = iconSize / 24.0;
	gc->PushState();
	gc->Translate(fullscreenRect.x + (fullscreenRect.width - iconSize) / 2.0,
		fullscreenRect.y + (fullscreenRect.height - iconSize) / 2.0);
	gc->Scale(scale, scale);
	gc->DrawRoundedRectangle(3, 4, 18, 12, 2);
	gc->StrokeLine(8, 20, 16, 20);
	gc->StrokeLine(12, 16, 12, 20);
	gc->PopState();
}

void CBitmapInfos::OnHeaderMouseMove(wxMouseEvent& event)
{
	UpdateFullscreenRect();
	const bool hover = fullscreenRect.Contains(event.GetPosition());
	if (hover != fullscreenHover)
	{
		fullscreenHover = hover;
		if (hover)
		{
			SetCursor(wxCursor(wxCURSOR_HAND));
			SetToolTip(CLibResource::LoadStringFromResource(L"LBLFULLSCREEN", 1));
		}
		else
		{
			SetCursor(wxCursor(wxCURSOR_ARROW));
			UnsetToolTip();
		}
		needToRefresh = true;
	}
}

void CBitmapInfos::OnHeaderMouseLeave(wxMouseEvent& event)
{
	if (fullscreenHover)
	{
		fullscreenHover = false;
		SetCursor(wxCursor(wxCURSOR_ARROW));
		UnsetToolTip();
		needToRefresh = true;
	}
}

void CBitmapInfos::OnHeaderLButtonDown(wxMouseEvent& event)
{
	UpdateFullscreenRect();
	if (!fullscreenRect.Contains(event.GetPosition()))
	{
		event.Skip();
		return;
	}



	if (fullscreenCallback)
	{
		fullscreenCallback();
	}
	else
	{
		// Repli : bascule la fenetre de premier niveau
		auto top = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
		if (top != nullptr)
			top->ShowFullScreen(!top->IsFullScreen());
	}
}

void CBitmapInfos::on_paint(wxPaintEvent& event)
{
	wxSize size = GetClientSize();
	if (size.x <= 0 || size.y <= 0)
		return;

	wxBufferedPaintDC dc(this);
	DrawInformations(&dc);
}