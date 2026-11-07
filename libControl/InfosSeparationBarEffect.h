#pragma once
#include <InfosSeparationBar.h>
using namespace Regards::Window;

namespace Regards::Control
{
	class CInfosSeparationBarEffect : public CInfosSeparationBar
	{
	public:
		CInfosSeparationBarEffect(const CThemeInfosSeparationBar& theme, float dpiScale)
			: CInfosSeparationBar(theme, dpiScale)
		{
		};

		~CInfosSeparationBarEffect(void) override
		{
		};

		void AddPhotoToList(const int& numElement);
	};
}
