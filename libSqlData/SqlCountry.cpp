#include <header.h>
#include "SqlCountry.h"
#include "SqlResult.h"
using namespace Regards::Sqlite;

CSqlCountry::CSqlCountry()
	: CSqlExecuteRequest(L"RegardsDB"), m_countryVector(nullptr)
{
}


CSqlCountry::~CSqlCountry()
{
}

/////////////////////////////////////////////////////////////////
//Chargement des informations sur les attributs
/////////////////////////////////////////////////////////////////
bool CSqlCountry::GetCountry(CountryVector* countryVector)
{
	m_countryVector = countryVector;
	return (ExecuteRequest("SELECT NumCountry, CodeCountry, LibelleContinent, LibelleCountry FROM COUNTRY") != -1)
		       ? true
		       : false;
}

int CSqlCountry::TraitementResult(CSqlResult* sqlResult)
{
	return FillVector(sqlResult, *m_countryVector);
};
