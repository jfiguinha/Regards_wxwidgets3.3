#include "header.h"
//
//  CalendarSelect.cpp
//  Regards.libExplorer
//
//  Created by figuinha jacques on 01/10/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#include "CalendarSelect.h"
#include "CalendarDialogBox.h"
#include <ConvertUtility.h>
using namespace Regards::Window;


bool CCalendarSelect::SelectNewDate(wxWindow* window, const wxString& dataInfos)
{
    CCalendarDialogBox calendarBox(window);
    wxDateTime dt;

    // 1. Tronquer la chaîne si elle dépasse 10 caractères
    wxString cleanedData = dataInfos;
    if (cleanedData.Length() > 10)
    {
        cleanedData = cleanedData.Left(10);
    }

    // 2. Vérifications de format et construction de la date
    if (cleanedData.Length() == 10)
    {
        // Séparateur dynamique basé sur le 5ème caractère (ex: YYYY-MM-DD ou YYYY/MM/DD)
        wxChar separator = cleanedData[4];
        vector<wxString> vDateTime = CConvertUtility::split(cleanedData, separator);

        // On vérifie que le split a bien retourné les 3 éléments attendus (Année, Mois, Jour)
        if (vDateTime.size() >= 3)
        {
            int year = atoi(vDateTime[0]);
            int month = atoi(vDateTime[1]) - 1; // wxDateTime::Month commence à 0 (Janvier)
            int day = atoi(vDateTime[2]);

            // Validation des plages de valeurs avant affectation
            if (month >= wxDateTime::Jan && month <= wxDateTime::Dec && day >= 1 && day <= 31 && year > 0)
            {
                dt.Set(day, static_cast<wxDateTime::Month>(month), year);
            }
        }
    }

    // 3. Si la date extraite n'est pas valide, on peut optionnellement définir la date du jour par défaut
    if (!dt.IsValid())
    {
        dt = wxDateTime::Today();
    }

    calendarBox.SetDate(dt);

    if (calendarBox.ShowModal() == wxID_OK)
    {
        SetSelectDate(calendarBox.GetDate());
        return true;
    }

    return false;
}

void CCalendarSelect::SetSelectDate(const wxDateTime& dateSelect)
{
	dt = dateSelect;
	updateDate = dt.Format("%Y.%m.%d");
}

bool CCalendarSelect::SelectNewDate(wxWindow* window, const wxDateTime& dateSelect)
{
	CCalendarDialogBox calendarBox(window);
	calendarBox.SetDate(dateSelect);

	if (calendarBox.ShowModal() == wxID_OK)
	{
		SetSelectDate(calendarBox.GetDate());
		return true;
	}


	return false;
}

wxDateTime CCalendarSelect::GetSelectDate()
{
	return dt;
}

wxString CCalendarSelect::GetSelectStringDate()
{
	return updateDate;
}
