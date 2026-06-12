#include <header.h>
//
//  PhotoGps.cpp
//  Regards.libDataStructure
//
//  Created by figuinha jacques on 29/09/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#include "PhotoGps.h"
#include <SqlResult.h>

CPhotoGps CPhotoGps::Read(Regards::Sqlite::CSqlResult* result)
{
	CPhotoGps gps;
	gps.SetId(result->GetInt("id"));
	gps.SetPath(result->GetText("FullPath"));
	gps.SetLatitude(result->GetText("latitude"));
	gps.SetLongitude(result->GetText("longitude"));
	return gps;
}

void CPhotoGps::SetId(const int& numId)
{
	this->numId = numId;
}

int CPhotoGps::GetId()
{
	return numId;
}

void CPhotoGps::SetPath(const wxString& path)
{
	this->path = path;
}

wxString CPhotoGps::GetPath()
{
	return path;
}

void CPhotoGps::SetLatitude(const wxString& latitude)
{
	this->latitude = latitude;
}

wxString CPhotoGps::GetLatitude()
{
	return latitude;
}

double CPhotoGps::GetLatitudeNumber()
{
	double dlat = 0.0;
	latitude.ToDouble(&dlat);
	return dlat;
}

void CPhotoGps::SetLongitude(const wxString& longitude)
{
	this->longitude = longitude;
}

wxString CPhotoGps::GetLongitude()
{
	return longitude;
}

double CPhotoGps::GetLongitudeNumber()
{
	double dlong = 0.0;
	longitude.ToDouble(&dlong);
	return dlong;
}
