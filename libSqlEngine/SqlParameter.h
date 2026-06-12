#pragma once
#define TYPE_INT 1
#define TYPE_STRING 2
namespace Regards
{
	namespace Sqlite
	{
		class CSqlParameter
		{
		public:
			int type = 0;
		};


		class CSqlInt :public CSqlParameter
		{
		public:
			int type = 1;
			int value = 0;
		};

		class CSqlString :public CSqlParameter
		{
		public:
			int type = 1;
			wxString value = "";
		};
	}
}