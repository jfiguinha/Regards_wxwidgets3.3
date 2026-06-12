#pragma once
#include <SqlLib.h>

namespace Regards
{
	namespace Sqlite
	{
		class CSqlResult;

		class CSqlLibExplorer : public CSqlLib
		{
		public:
			CSqlLibExplorer(const bool& readOnly, const wxString& libelleNotGeo, const bool& m_loadInMemory);
			~CSqlLibExplorer() override;

			bool InitDatabase(const wxString& lpFilename) override;
			bool CheckVersion(const wxString& lpFilename) override;

		private:
			wxString libelleNotGeo;
			void LoadAndRotate(const wxString& filePath, const int& rotate);
			bool CreateDatabase(const wxString& databasePath, const bool& m_loadInMemory);
		};
	}
}
