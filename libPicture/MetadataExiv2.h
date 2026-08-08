#pragma once
;
class CMetadata;

namespace Regards
{
	namespace exiv2
	{
		class CPictureMetadataExiv;

		class CMetadataExiv2
		{
		public:
			CMetadataExiv2(const wxString& filename);
			~CMetadataExiv2();
			std::vector<uint8_t> GetMetadataBuffer();
			bool HasExif();
			bool HasThumbnail();
			int GetOrientation();
			void SetOrientation(const int& orientation);
			std::vector<CMetadata> GetMetadata();
			bool CopyMetadata(const wxString& output);
			wxImage DecodeThumbnail(wxString& extension, int& orientation);
			void SetDateTime(const wxString& dateTime);
			wxString GetCreationDate();
			void SetGpsInfos(const wxString& latitudeRef, const wxString& longitudeRef, const wxString& latitude,
			                 const wxString& longitude);
			//void ReadVideo(bool & hasGps, bool & hasDataTime, wxString & dateTimeInfos, wxString & latitude, wxString & longitude);
			void ReadPicture(bool& hasGps, bool& hasDataTime, wxString& dateTimeInfos, wxString& latitude,
			                 wxString& longitude);

		private:
			CPictureMetadataExiv* metaExiv;
			wxString filename;

		};
	}
}
