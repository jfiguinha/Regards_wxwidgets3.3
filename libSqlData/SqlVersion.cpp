#include <header.h>
//
//  SqlVersion.cpp
//  Regards.libSqlData
//
//  Created by figuinha jacques on 29/09/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#include "SqlVersion.h"
#include "SqlResult.h"
#include <SqlParameter.h>
#include <ConvertUtility.h>
using namespace Regards::Sqlite;

CSqlVersion::CSqlVersion(CSqlLib* m_transaction, const bool& m_useTransaction)
	: CSqlExecuteRequest(L"RegardsDB")
{
	this->m_transaction = m_transaction;
	this->m_useTransaction = m_useTransaction;
	typeResult = 0;
	result = "";
}


CSqlVersion::~CSqlVersion()
{
}

bool CSqlVersion::InsertVersion(const wxString& version)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(version));
	return ExecuteSqlWithStatementBool("INSERT INTO VERSION (libelle) VALUES (?)", parameter);
}

bool CSqlVersion::UpdateVersion(const wxString& version, const wxString& oldValue)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(version));
	parameter.push_back(std::make_unique<CSqlString>(oldValue));
	return ExecuteSqlWithStatementBool("UPDATE VERSION SET libelle = ? WHERE libelle = ?", parameter);
}

bool CSqlVersion::DeleteVersion()
{
	return (ExecuteRequestWithNoResult("DELETE FROM VERSION") != -1) ? true : false;
}

wxString CSqlVersion::GetVersion()
{
	typeResult = 1;
	ExecuteRequest("SELECT libelle FROM VERSION");
	printf("Version : %s \n", CConvertUtility::ConvertToStdString(result));
	return result;
}


int CSqlVersion::TraitementResult(CSqlResult* sqlResult)
{
	result = "";
	while (sqlResult->Next())
	{
		switch (typeResult)
		{
		case 1:
			result = sqlResult->GetText("libelle");
			break;
		}

		if (!result.empty())
			break;
	}
	return 1;
}
