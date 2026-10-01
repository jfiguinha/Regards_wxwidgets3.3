// Dessin.h: interface for the CDessin class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CEffectParameter;

namespace Regards::FiltreEffet
{
	class CDraw
	{
	public:
		void SetRatio(const float& m_fValue);
		void SetMaxPosition(const wxRect& m_rcPicture);
		CDraw();
		virtual ~CDraw();

		wxColour WithOpacity(const wxColour& colour, unsigned char opacity)
		{
			return wxColour(
				colour.Red(),
				colour.Green(),
				colour.Blue(),
				opacity);
		}

		cv::Scalar ConvertScalarToWxColour(const cv::Scalar& scalar_color, int alpha)
		{
			// Extraction des canaux OpenCV (Indices standard : 0 = Blue, 1 = Green, 2 = Red, 3 = Alpha)
			int blue = static_cast<int>(scalar_color[0]);
			int green = static_cast<int>(scalar_color[1]);
			int red = static_cast<int>(scalar_color[2]);

			return cv::Scalar(red, green, blue, alpha);
		}


		virtual bool RefreshAfterKeyDown()
		{
			return false;
		}

		virtual bool RefreshAfterUpdateParameter()
		{
			return false;
		}

		wxColour WithOpacity(const cv::Scalar& colour)
		{
			return wxColour(
				static_cast<unsigned char>(colour[2]),
				static_cast<unsigned char>(colour[1]),
				static_cast<unsigned char>(colour[0]),
				static_cast<unsigned char>(colour[3]));
		}

		static void DrawARectangle(wxDC* deviceContext, const wxRect& rc, const wxColour& rgb);
		static void DessinerRectangleVide(wxDC* deviceContext, const int32_t& iTaille, const wxRect& rc,
		                                  const wxColour& rgb);
		void DessinerCarre(wxDC* deviceContext, const int32_t& iLargeur, const int32_t& iHauteur, const int32_t& iMarge,
		                   const int32_t& iPosX = 0, const int32_t& iPosY = 0,
		                   const wxColour& rgb = wxColour(0, 0, 0, 0));
		static void DessinerDashRectangle(wxDC* deviceContext, const int32_t& iTaille, const wxRect& rc,
		                                  const wxColour& rgbFirst, const wxColour& rgbSecond);
		static void DessinerDotDashRectangle(wxDC* deviceContext, const int32_t& iTaille, const wxRect& rc,
		                                     const wxColour& rgbFirst, const wxColour& rgbSecond);

		virtual void Dessiner(wxDC* deviceContext, const long& m_lHScroll, const long& m_lVScroll, const float& ratio)
		{
		};

		virtual void Dessiner(wxDC* deviceContext, const long& m_lHScroll, const long& m_lVScroll, const float& ratio,
		                      const wxColour& rgb)
		{
		};

		virtual void Dessiner(wxDC* deviceContext, const long& m_lHScroll, const long& m_lVScroll, const float& ratio,
		                      const wxColour& rgb, const wxColour& rgbFirst, const wxColour& rgbSecond,
		                      const int32_t& style)
		{
		};

		virtual void Selection(const int32_t& xNewSize, const int32_t& yNewSize, const long& m_lHScroll,
		                       const long& m_lVScroll, const float& ratio)
		{
		};

		virtual void MouseMove(const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll,
		                       const float& ratio)
		{
		};

		virtual void MouseMove(wxDC* deviceContext, const long& m_lx, const long& m_ly, const long& m_lHScroll,
		                       const long& m_lVScroll, const float& ratio, const wxColour& rgb)
		{
		};


		virtual void KeyDown(const int32_t& keyCode) {};

		virtual void MouseMove(wxDC* deviceContext, const long& m_lx, const long& m_ly, const long& m_lHScroll,
		                       const long& m_lVScroll, const float& ratio, const wxColour& rgb, const wxColour& rgbBack)
		{
		};

		virtual void InitPoint(CEffectParameter* effect, const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll,
		                       const float& ratio)
		{
		};

		virtual void InitPoint(const long& m_lx, const long& m_ly, const long& m_lHScroll, const long& m_lVScroll,
		                       const float& ratio, const wxColour& rgb)
		{
		};


		virtual void SetTransparenceValue(wxImage& drawingImage)
		{

		};

		virtual void DessinerSurMat(cv::Mat& matrix, const long& hScroll, const long& vScroll, const float& ratio)
		{

		};

		cv::Scalar GetColorWithTransparancy(wxColour color, int opacity)
		{
			// OpenCV utilise le format BGR(A) : 
			// color[0] = Blue, color[1] = Green, color[2] = Red
			// On écrase le 4ème canal (indice 3) avec la valeur d'opacité courante du trait
			return cv::Scalar(color.Red(), color.Green(), color.Blue(), static_cast<double>(opacity));
		};

		virtual void MouseUp()
		{};

		virtual void MouseDown(CEffectParameter* effect)
		{};

		virtual void GetPos(wxRect& rc)
		{
			rc = m_rcAffichage;
		};

		virtual void GetPoint(wxPoint& pt)
		{
			pt = this->pt;
		};

		virtual void GetScreenPoint(wxPoint& pt)
		{
			pt = this->pt;
		};

		virtual void SetScaleFactor(const double& factor)
		{
			this->factor = factor;
		}

		virtual double GetScaleFactor()
		{
			return factor;
		}

		virtual void SetCursor()
		{
			wxSetCursor(wxCursor(wxCURSOR_ARROW));
		}

		float XDrawingPosition(const float& m_lx, const long& m_lHScroll, const float& ratio);
		float YDrawingPosition(const float& m_ly, const long& m_lVScroll, const float& ratio);
		float XRealPosition(const float& m_lx, const long& m_lHScroll, const float& ratio);
		float YRealPosition(const float& m_ly, const long& m_lVScroll, const float& ratio);

	protected:
		wxRect m_rcAffichage;
		float m_fRatioValue;
		wxPoint pt;
		double factor = 1.0;
		bool VerifierValiditerPoint(const wxPoint& pt);

	};
}
