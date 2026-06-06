//
//  SqlVersion.hpp
//  Regards.libSqlData
//
//  Created by figuinha jacques on 29/09/2015.
//  Copyright © 2015 figuinha jacques. All rights reserved.
//
#pragma once
#include "SqlExecuteRequest.h"

namespace Regards
{
	namespace Sqlite
	{
		class CSqlResult;

		class CSqlVersion : public CSqlExecuteRequest
		{
		public:
			CSqlVersion(CSqlLib* m_transaction = nullptr, const bool& m_useTransaction = false);
			~CSqlVersion() override;
			bool InsertVersion(const wxString& version);
			bool UpdateVersion(const wxString& version, const wxString& oldValue);
			bool DeleteVersion();
			wxString GetVersion();

		private:
			int TraitementResult(CSqlResult* sqlResult) override;
			int typeResult;
			wxString result;
		};
	}
}
