#pragma once
namespace Regards
{
	namespace Editor
	{
		class CMainTheme;

		class CMainThemeInit
		{
		public:
			static CMainTheme* getInstance();
			static void SaveTheme();

		private:

			static void Initialize();
			static std::unique_ptr<CMainTheme> _singleton;
			static wxString documentPath;
		};
	}
}
