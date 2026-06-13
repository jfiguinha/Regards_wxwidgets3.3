#pragma once

namespace Regards::Internet
{


	class CHttpRequest
	{
	public:
		static wxString ExecuteRequest(const wxString& url);
	};
}