#include <header.h>
//
//  SqlGps.cpp
//  Regards.libSqlData
//
//  Created by figuinha jacques on 29/09/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//

#include "SqlGps.h"
#include "SqlResult.h"
#include <SqlParameter.h>
using namespace Regards::Sqlite;

CSqlGps::CSqlGps(CSqlLib* m_transaction, const bool& m_useTransaction)
	: CSqlExecuteRequest(L"RegardsDB")
{
	this->m_transaction = m_transaction;
	this->m_useTransaction = m_useTransaction;
	typeResult = 0;
	photogpsVector = nullptr;
}


CSqlGps::~CSqlGps()
{
}

bool CSqlGps::InsertGps(const wxString& filepath, const wxString& latitude, const wxString& longitude)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(filepath));
	parameter.push_back(std::make_unique<CSqlString>(latitude));
	parameter.push_back(std::make_unique<CSqlString>(longitude));
	return ExecuteSqlWithStatementBool("INSERT INTO PHOTOGPS (FullPath, latitude, longitude) VALUES (?,?,?)", parameter);
}

bool CSqlGps::UpdateGps(const wxString& filepath, const wxString& latitude, const wxString& longitude)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(latitude));
	parameter.push_back(std::make_unique<CSqlString>(longitude));
	parameter.push_back(std::make_unique<CSqlString>(filepath));
	return ExecuteSqlWithStatementBool("UPDATE PHOTOGPS SET latitude = ? , longitude = ? WHERE FullPath = ?", parameter);
}

bool CSqlGps::DeleteGps(const wxString& filepath)
{
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(filepath));
	return ExecuteSqlWithStatementBool("DELETE FROM PHOTOGPS WHERE FullPath = ?", parameter);
}

void CSqlGps::GetGps(PhotoGpsVector* photogpsVector, const wxString& filepath)
{
	typeResult = 0;
	this->photogpsVector = photogpsVector;
	std::vector<std::unique_ptr<CSqlParameter>> parameter;
	parameter.push_back(std::make_unique<CSqlString>(filepath));
	ExecuteSqlWithStatement("SELECT id, FullPath, latitude, longitude FROM PHOTOGPS where FullPath = ?", parameter);
}

int CSqlGps::TraitementResult(CSqlResult* sqlResult)
{
	return FillVector(sqlResult, *photogpsVector);
}
