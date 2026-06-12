#include <header.h>
#include "SqlResult.h"
#include "SqlCatalog.h"
#include <SqlParameter.h>
using namespace Regards::Sqlite;

CSqlCatalog::CSqlCatalog()
	: CSqlExecuteRequest(L"RegardsDB")
{
	numCatalogId = -1;
}


CSqlCatalog::~CSqlCatalog()
{
}


bool CSqlCatalog::InsertCatalog(const wxString& libelle)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(libelle));
	return ExecuteSqlWithStatementBool("INSERT INTO CATALOG (LibelleCatalog) VALUES (?)", parameter);
}

bool CSqlCatalog::UpdateCatalog(const int64_t& numCatalog, const wxString& libelle)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(libelle));
	parameter.push_back(std::make_unique<CSqlInt>(numCatalog));
	return (ExecuteSqlWithStatementBool("UPDATE CATALOG SET LibelleCatalog = ? WHERE NumCatalog = ?", parameter);
}

int64_t CSqlCatalog::GetCatalogId(const wxString& libelle)
{
	type = 1;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(libelle));
	ExecuteSqlWithStatement("SELECT NumCatalog FROM CATALOG WHERE LibelleCatalog = ?", parameter);
	return numCatalogId;
}

wxString CSqlCatalog::GetCatalogLibelle(const int64_t& numCatalog)
{
	type = 2;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numCatalog));
	ExecuteSqlWithStatement("SELECT LibelleCatalog FROM CATALOG WHERE NumCatalog = ?", parameter);
	return libelle;
}

bool CSqlCatalog::DeleteCatalog(const int64_t& numCatalog)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlInt>(numCatalog));
	return ExecuteSqlWithStatementBool("DELETE FROM CATALOG WHERE NumCatalog = ?", parameter);
}

int CSqlCatalog::TraitementResult(CSqlResult* sqlResult)
{
	while (sqlResult->Next())
	{
		switch (type)
		{
		case 1:
			numCatalogId = sqlResult->GetInt("NumCatalog");
			break;
		case 2:
			libelle = sqlResult->GetText("LibelleCatalog");
			break;
		}
	}
};
