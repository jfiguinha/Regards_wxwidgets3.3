#include <header.h>
#include "SqlPhotoCriteria.h"
#include "SqlCriteria.h"
#include "SqlPhotos.h"
#include <ConvertUtility.h>
#include <SqlTransaction.h>
#include <SqlParameter.h>
using namespace Regards::Sqlite;

CSqlPhotoCriteria::CSqlPhotoCriteria()
	: CSqlExecuteRequest(L"RegardsDB")
{
}


CSqlPhotoCriteria::~CSqlPhotoCriteria()
{
}

bool CSqlPhotoCriteria::InsertPhotoListCriteria(const CListCriteriaPhoto& listPhotoCriteria, bool& isNew,
                                                bool criteriaUpdate, const int& numFolder)
{
	CSqlTransaction sqlTransaction;
	CSqlPhotos sqlPhoto;
	CSqlCriteria sqlCriteria;

	for (auto it = listPhotoCriteria.listCriteria.begin(); it != listPhotoCriteria.listCriteria.end(); ++it)
	{
		CInsertCriteria* insertCriteria = *it;

		//Old Criteria
		int oldCriteriaId = sqlCriteria.GetCriteriaIdByCategorie(listPhotoCriteria.numPhotoId, insertCriteria->type);

		//printf("insertCriteria value : %s \n", CConvertUtility::ConvertToUTF8(insertCriteria->value));

		insertCriteria->id = sqlCriteria.GetOrInsertCriteriaId(listPhotoCriteria.numCatalog, insertCriteria->type,
		                                                       insertCriteria->value, insertCriteria->isNew);

		if (oldCriteriaId == insertCriteria->id)
			continue;

		if (insertCriteria->isNew && isNew == false)
			isNew = true;

		if (!isNew && numFolder != 0)
		{
			int numCriteria = sqlCriteria.GetCriteriaId(insertCriteria->id, numFolder);
			if (numCriteria != insertCriteria->id)
				isNew = true;
		}

		if (oldCriteriaId != -1)
			DeletePhotoCriteria(listPhotoCriteria.numPhotoId, oldCriteriaId);


		InsertPhotoCriteria(listPhotoCriteria.numPhotoId, insertCriteria->id);
	}

	if (criteriaUpdate)
		sqlPhoto.UpdatePhotoCriteria(listPhotoCriteria.numPhotoId);

	sqlTransaction.commit();

	return true;
}

bool CSqlPhotoCriteria::InsertPhotoListCriteria(const CListCriteriaPhoto& listPhotoCriteria, bool& isNew,
                                                bool criteriaUpdate)
{
	return InsertPhotoListCriteria(listPhotoCriteria, isNew, criteriaUpdate, 0);
}

bool CSqlPhotoCriteria::InsertPhotoCriteria(const int64_t& numPhoto, const int64_t& numCriteria)
{
	/*
	std::vector<CSqlParameter*> parameter;
	CSqlInt* sqlInt1 = new CSqlInt();
	sqlInt1->value = numCriteria;
	parameter.push_back(sqlInt1);

	CSqlInt* sqlInt2 = new CSqlInt();
	sqlInt2->value = numPhoto;
	parameter.push_back(sqlInt2);

	bool result = (ExecuteSqlWithStatement("INSERT INTO PHOTOSCRITERIA (NumCriteria, NumPhoto) VALUES (?,?)", parameter) != -1)
	? true
	: false;

	for(CSqlParameter * element : parameter)
	{
		delete element;
	}

	return result;
	*/

	return (ExecuteRequestWithNoResult(
		       "INSERT INTO PHOTOSCRITERIA (NumCriteria, NumPhoto) VALUES (" + to_string(numCriteria) + "," +
		       to_string(numPhoto) + ")") != -1)
		       ? true
		       : false;
	
}

/////////////////////////////////////////////////////////////////////////////
//Suppression de tous les attributs pour les albums
/////////////////////////////////////////////////////////////////////////////
bool CSqlPhotoCriteria::DeleteCriteria(const int64_t& numCriteria)
{
	return (ExecuteRequestWithNoResult("DELETE FROM PHOTOSCRITERIA WHERE NumCriteria = " + to_string(numCriteria)) != -
		       1)
		       ? true
		       : false;
}

bool CSqlPhotoCriteria::DeletePhoto(const int64_t& numPhoto)
{
	return (ExecuteRequestWithNoResult("DELETE FROM PHOTOSCRITERIA WHERE NumPhoto = " + to_string(numPhoto)) != -1)
		       ? true
		       : false;
}

bool CSqlPhotoCriteria::DeletePhotoCriteria(const int64_t& numPhoto, const int64_t& numCriteria)
{
	int value = ExecuteRequestWithNoResult(
		"DELETE FROM PHOTOSCRITERIA WHERE NumCriteria = " + to_string(numCriteria) + " and NumPhoto = " +
		to_string(numPhoto));
	return (value != -1) ? true : false;
}

bool CSqlPhotoCriteria::DeleteFolderCriteria(const int64_t& numFolder)
{
	return (ExecuteRequestWithNoResult(
		       "DELETE FROM PHOTOSCRITERIA WHERE NumPhoto in (select NumPhoto from PHOTOS where NumFolderCatalog = " +
		       to_string(numFolder) + ")") != -1)
		       ? true
		       : false;
}

bool CSqlPhotoCriteria::DeleteCatalogCriteria(const int64_t& numCatalog)
{
	return (ExecuteRequestWithNoResult(
		       "DELETE FROM PHOTOSCRITERIA as PC INNER JOIN PHOTOS as PH ON PC.NumPhoto = PH.NumPhoto INNER JOIN FOLDERCATALOG FC on FC.NumFolderCatalog = PH.NumFolderCatalog WHERE FC.NumCatalog = "
		       + to_string(numCatalog)) != -1)
		       ? true
		       : false;
}


bool CSqlPhotoCriteria::DeletePhotoCriteria()
{
	return ExecuteRequestWithNoResult("DELETE FROM PHOTOSCRITERIA");
}
