#pragma once
#include <SqlEngine.h>
#include <SqlLibExplorer.h>
#include <LibResource.h>
#include <wx/filename.h>
using namespace Regards::Sqlite;

namespace Regards::Sqlite
{
	class CSqlInit
	{
	public:

		static bool InitializeSQLServerDatabase(const wxString& folder, const bool& load_inmemory)
		{
			wxString libelleNotGeo = CLibResource::LoadStringFromResource("LBLNOTGEO", 1);
			auto libExplorer = new CSqlLibExplorer(false, libelleNotGeo, load_inmemory);
			wxFileName file = wxFileName(folder, "regards.db");
			return CSqlEngine::Initialize(file.GetFullPath(), L"RegardsDB", libExplorer);
		}
		static void KillSqlEngine()
		{
			CSqlEngine::kill(L"RegardsDB");
		}
	};
}
