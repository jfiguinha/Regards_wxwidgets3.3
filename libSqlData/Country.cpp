#include <header.h>
#include "Country.h"
#include <SqlResult.h>

CCountry::CCountry(): numId(0)
{
}


CCountry::~CCountry()
{
}

CCountry CCountry::Read(Regards::Sqlite::CSqlResult* result)
{
	CCountry country;//NumCountry, CodeCountry, LibelleContinent, LibelleCountry
	country.SetId(result->GetInt("NumCountry"));
	country.SetCode(result->GetText("CodeCountry"));
	country.SetContinent(result->GetText("LibelleContinent"));
	country.SetLibelle(result->GetText("LibelleCountry"));
	return country;
}


void CCountry::SetId(const int& numId)
{
	this->numId = numId;
}

int CCountry::GetId()
{
	return numId;
}

void CCountry::SetContinent(const wxString& continent)
{
	this->continent = continent;
}

wxString CCountry::GetContinent()
{
	return continent;
}

void CCountry::SetCode(const wxString& code)
{
	this->code = code;
}

wxString CCountry::GetCode()
{
	return code;
}

void CCountry::SetLibelle(const wxString& libelle)
{
	this->libelle = libelle;
}

wxString CCountry::GetLibelle()
{
	return libelle;
}
