#include "header.h"
#ifdef __NEW_EXIV2__
#include <ConvertUtility.h>
#include <exiv2/image.hpp>
#include <exiv2/error.hpp>
#include "PictureMetadataExiv_new.h"
#include <libexif/exif-data.h>
#include <regex>
using namespace Regards::exiv2;


CPictureMetadataExiv::CPictureMetadataExiv(const wxString& filename)
{
	//Copy des infos exifs
	isExif = false;

	//Read exif info from source file
	try
	{
		this->filename = filename;
		exif = Exiv2::ImageFactory::open(CConvertUtility::ConvertToStdString(filename).c_str());
		//assert(exif.get() != 0);
		exif->readMetadata();
		isExif = true;
	}
	catch (Exiv2::Error& e)
	{
		std::cout << "Caught Exiv2 exception '" << e << "'\n";
		//return -1;
	}
}

CPictureMetadataExiv::CPictureMetadataExiv(uint8_t* data, const long& size)
{
	//Copy des infos exifs
	isExif = false;

	//Read exif info from source file
	try
	{
		exif = Exiv2::ImageFactory::open(data, size);
		//assert(exif.get() != 0);
		exif->readMetadata();
		isExif = true;
	}
	catch (Exiv2::Error& e)
	{
		std::cout << "Caught Exiv2 exception '" << e << "'\n";
		//return -1;
	}
}


wxString CPictureMetadataExiv::GetCreationDate()
{
	wxString date = "";
	if (isExif)
	{
		try
		{
			Exiv2::ExifData& exifData = exif->exifData();
			if (exifData.empty())
				return "";

			Exiv2::ExifKey orientationKey("Exif.Image.DateTime");
			Exiv2::ExifData::const_iterator md = exifData.findKey(orientationKey);
			if (exifData.end() != md)
			{
				date = md->value().toString();
			}
		}
		catch (...)
		{
		}
	}


	return date;
}

void CPictureMetadataExiv::SetOrientation(const int& orientation)
{
	if (!isExif || exif.get() == nullptr)
		return;

	try
	{
		Exiv2::ExifData& exifData = exif->exifData();
		AddAsciiValue("Exif.Image.Orientation", to_string(orientation), exifData);
		exif->setExifData(exifData);
		exif->writeMetadata();
	}
	catch (...)
	{
	}
}


void CPictureMetadataExiv::SetDateTime(const wxString& dateTime)
{
	if (!isExif || exif.get() == nullptr)
		return;

	try
	{
		Exiv2::ExifData& exifData = exif->exifData();
		AddAsciiValue("Exif.Image.DateTime", dateTime, exifData);
		exif->setExifData(exifData);
		exif->writeMetadata();
	}
	catch (...)
	{
	}
}

void CPictureMetadataExiv::AddAsciiValue(wxString keyName, wxString value, Exiv2::ExifData& exifData)
{
	Exiv2::ExifKey key(CConvertUtility::ConvertToStdString(keyName));

	auto md = exifData.findKey(key);
	if (exifData.end() != md)
	{
		Exiv2::Value::UniquePtr rv = Exiv2::Value::create(Exiv2::asciiString);
		rv->read(CConvertUtility::ConvertToStdString(value));
		md->setValue(rv.get());
		return;
	}

	// Create a ASCII string value (note the use of create)
	Exiv2::Value::UniquePtr v = Exiv2::Value::create(Exiv2::asciiString);
	// Set the value to a string
	v->read(CConvertUtility::ConvertToStdString(value));
	exifData.add(key, v.get());
}

void CPictureMetadataExiv::AddRationalValue(wxString keyName, wxString value, Exiv2::ExifData& exifData)
{
	Exiv2::ExifKey key(CConvertUtility::ConvertToStdString(keyName));

	auto md = exifData.findKey(key);
	if (exifData.end() != md)
	{
		Exiv2::URationalValue::UniquePtr rv = GetGpsRationalValue(value);
		md->setValue(rv.get());
		return;
	}

	Exiv2::URationalValue::UniquePtr rv = GetGpsRationalValue(value);
	exifData.add(key, rv.get());
}

void CPictureMetadataExiv::SetGpsInfos(const wxString& latitudeRef, const wxString& longitudeRef,
	const wxString& latitude, const wxString& longitude)
{
	if (!isExif || exif.get() == nullptr)
		return;

	try
	{
		Exiv2::ExifData& exifData = exif->exifData();

		AddAsciiValue("Exif.GPSInfo.GPSLatitudeRef", latitudeRef, exifData);
		AddRationalValue("Exif.GPSInfo.GPSLatitude", latitude, exifData);
		AddAsciiValue("Exif.GPSInfo.GPSLongitudeRef", longitudeRef, exifData);
		AddRationalValue("Exif.GPSInfo.GPSLongitude", longitude, exifData);

		exif->setExifData(exifData);
		exif->writeMetadata();
	}
	catch (...)
	{
	}
}

CPictureMetadataExiv::~CPictureMetadataExiv()
{}

bool CPictureMetadataExiv::HasExif()
{
	return isExif;
}


// Recupere les donnees Exif brutes du fichier (telles que produites par
// libexif), au format pret a etre ecrit comme segment APP1 (sans le
// header JPEG/Exif "ff d8 ff e1" + taille, qui n'est pas ajoute ici).
//
// Protocole en deux passes, a la charge de l'appelant :
//   1) appeler avec size == 0 : la fonction renvoie dans "size" la taille
//      necessaire (0 si aucune donnee Exif n'a pu etre lue).
//   2) l'appelant alloue un buffer "data" d'au moins "size" octets, puis
//      rappelle la fonction avec ce "size" : la fonction remplit "data".
//
// Pour eviter de relire/re-parser le fichier deux fois (et pour garantir
// que la taille annoncee a la passe 1 correspond exactement aux donnees
// ecrites a la passe 2, meme si le fichier change entre les deux appels),
// le buffer lu par libexif est mis en cache dans "cachedMetadataBuffer"
// des la premiere passe et reutilise par la seconde.
void CPictureMetadataExiv::GetMetadataBuffer(uint8_t*& data, unsigned int& size)
{
	if (size == 0)
	{
		cachedMetadataBuffer.clear();

		ExifData* d = exif_data_new_from_file(filename);
		if (!d)
		{
			//fprintf(stderr, "Could not load data from '%s'!\n", filename);
			size = 0;
			return;
		}

		unsigned char* buf = nullptr;
		unsigned int local = 0;
		exif_data_save_data(d, &buf, &local);
		exif_data_unref(d);

		if (buf != nullptr && local > 0)
		{
			cachedMetadataBuffer.assign(buf, buf + local);
		}

		if (buf != nullptr)
			free(buf);

		size = static_cast<unsigned int>(cachedMetadataBuffer.size());
	}
	else
	{
		// Si la passe 1 n'a pas ete faite (ou a echoue), on relit le fichier
		// directement ici en secours.
		if (cachedMetadataBuffer.empty())
		{
			ExifData* d = exif_data_new_from_file(filename);
			if (!d)
			{
				//fprintf(stderr, "Could not load data from '%s'!\n", filename);
				size = 0;
				return;
			}

			unsigned char* buf = nullptr;
			unsigned int local = 0;
			exif_data_save_data(d, &buf, &local);
			exif_data_unref(d);

			if (buf != nullptr && local > 0)
			{
				cachedMetadataBuffer.assign(buf, buf + local);
			}

			if (buf != nullptr)
				free(buf);
		}

		// Garde-fou : ne jamais ecrire plus que ce que l'appelant a alloue.
		unsigned int toCopy = static_cast<unsigned int>(cachedMetadataBuffer.size());
		if (toCopy > size)
		{
			// Le buffer fourni par l'appelant est trop petit par rapport aux
			// donnees actuelles : on signale la taille reellement necessaire
			// et on n'ecrit rien pour eviter un debordement.
			size = toCopy;
			return;
		}

		if (toCopy > 0)
		{
			memcpy(data, cachedMetadataBuffer.data(), toCopy);
		}

		size = toCopy;

		// Le cache n'est utile que pour relier les deux passes ; on le
		// libere une fois la copie effectuee.
		cachedMetadataBuffer.clear();
		cachedMetadataBuffer.shrink_to_fit();
	}
}

bool CPictureMetadataExiv::CopyMetadata(const wxString& output)
{
	if (isExif)
	{
		try
		{
			Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(CConvertUtility::ConvertToStdString(output));
			if (exif.get())
			{
				bool wroteSomething = false;

				Exiv2::ExifData& exifData = exif->exifData();
				if (!exifData.empty())
				{
					image->setExifData(exifData);
					wroteSomething = true;
				}

				Exiv2::XmpData& xmpData = exif->xmpData();
				if (!xmpData.empty())
				{
					image->setXmpData(xmpData);
					wroteSomething = true;
				}

				Exiv2::IptcData& iptcData = exif->iptcData();
				if (!iptcData.empty())
				{
					image->setIptcData(iptcData);
					wroteSomething = true;
				}

				if (wroteSomething)
				{
					image->writeMetadata();
				}

				return true;
			}
		}
		catch (...)
		{
		}
	}
	return false;
}

bool CPictureMetadataExiv::HasThumbnail()
{
	if (isExif && exif.get() != nullptr)
	{
		try
		{
			Exiv2::ExifData& exifData = exif->exifData();
			if (!exifData.empty())
			{
				Exiv2::ExifThumb thumb(exifData);
				Exiv2::DataBuf data = thumb.copy();
				if (data.size() > 0 && data.data() != nullptr)
					return true;
			}
		}
		catch (...)
		{
		}
	}


	return false;
}

int CPictureMetadataExiv::GetOrientation()
{
	int orientation = -1;
	if (isExif)
	{
		try
		{
			Exiv2::ExifData& exifData = exif->exifData();
			if (exifData.empty())
				return -1;

			Exiv2::ExifKey orientationKey("Exif.Image.Orientation");
			Exiv2::ExifData::const_iterator md = exifData.findKey(orientationKey);
			if (exifData.end() != md)
			{
				wxString value = md->value().toString();
				orientation = atoi(value.c_str());
			}
		}
		catch (...)
		{
		}
	}


	return orientation;
}

Exiv2::URationalValue::UniquePtr CPictureMetadataExiv::GetGpsRationalValue(const wxString& gpsValue)
{
	double dblValue;
	gpsValue.ToDouble(&dblValue);
	Exiv2::URationalValue::UniquePtr rv(new Exiv2::URationalValue);

	//Get Hour
	int result = lround(dblValue);
	rv->value_.push_back(std::make_pair(result, 1));

	//Get Minute
	dblValue = dblValue - result;
	dblValue = 60.0 * dblValue;
	result = lround(dblValue);
	rv->value_.push_back(std::make_pair(result, 1));

	//Get Seconds
	dblValue = dblValue - result;
	dblValue = (3600.0 * dblValue) * 100.0;
	result = lround(dblValue);
	rv->value_.push_back(std::make_pair(result, 100));
	return rv;
}

wxString CPictureMetadataExiv::GetGpsfValue(const wxString& gpsValue)
{
	wxString returnValue = "";
	vector<wxString> latValue;
	int i = 0;

	//Conversion des valeurs des latitudes et des longitudes
	latValue = CConvertUtility::split(gpsValue, ' ');

	float outputValue = 0.0;

	if (latValue.size() == 3)
	{
		for (auto it = latValue.begin(); it != latValue.end(); ++it)
		{
			vector<wxString> intValue = CConvertUtility::split(*it, '/');
			if (intValue.size() != 2)
				return "";

			int valeur = atoi(intValue.at(0));
			int diviseur = atoi(intValue.at(1));
			if (diviseur == 0)
				return "";

			float value = static_cast<float>(valeur) / static_cast<float>(diviseur);
			if (i == 1)
			{
				value = value / 60;
			}
			else if (i == 2)
			{
				value = value / 3600;
			}

			outputValue += value;
			i++;
		}
	}

	return to_string(outputValue);
}


void CPictureMetadataExiv::ReadVideo(bool& hasGps, bool& hasDataTime, wxString& dateTimeInfos, wxString& latitude,
	wxString& longitude)
{
	hasGps = false;
	hasDataTime = false;
	if (isExif)
	{
		try
		{
			Exiv2::XmpData& xmpData = exif->xmpData();
			if (xmpData.empty())
				return;

			bool apple = false;
			wxString exifinfos;
			wxString informations;

			auto end = xmpData.end();
			for (auto md = xmpData.begin(); md != end; ++md)
			{
				informations = md->key();
				exifinfos = toString(*md);

				if (informations == "Xmp.video.MimeType")
				{
					if (exifinfos == "video/quicktime")
						apple = true;
				}
				else if (informations == "Xmp.video.GPSCoordinates")
				{
					// Format XMP attendu : "+DD.DDDD+DDD.DDDD/" (lat lon, signe
					// obligatoire devant chaque composante).
					std::wstring s = exifinfos.ToStdWstring();
					static const std::wregex re(LR"(^([+-][0-9]+(?:\.[0-9]+)?)([+-][0-9]+(?:\.[0-9]+)?))");
					std::wsmatch m;

					if (std::regex_search(s, m, re) && m.size() == 3)
					{
						hasGps = true;
						double flatitude = std::stod(m[1].str());
						double flongitude = std::stod(m[2].str());
						latitude = to_string(flatitude);
						longitude = to_string(flongitude);
					}
				}
				else if (informations.Find("TrackCreateDate") >= 0)
				{
					if (apple)
					{
						int64_t dateTime = atol(exifinfos.c_str());
						if (dateTime > 0)
						{
							dateTimeInfos = GetQuickTimeDate(dateTime);
							hasDataTime = true;
						}
					}
					else
					{
						dateTimeInfos = exifinfos;
						hasDataTime = true;
					}
				}
			}
		}
		catch (...)
		{
		}
	}
}


wxString CPictureMetadataExiv::GetQuickTimeDate(int64_t dateQuicktime)
{
	// Les dates QuickTime sont exprimees en secondes depuis le
	// 1904-01-01 00:00:00 UTC. On les convertit en secondes depuis
	// l'epoch Unix (1970-01-01), puis on formate en UTC.
	static const time_t SecsUntil1970 = 2082844800;

	time_t unixTime = static_cast<time_t>(dateQuicktime) - SecsUntil1970;
	if (unixTime < 0)
		return "";

	struct tm utcTime {};
#ifdef _WIN32
	if (gmtime_s(&utcTime, &unixTime) != 0)
		return "";
#else
	if (gmtime_r(&unixTime, &utcTime) == nullptr)
		return "";
#endif

	char message[32];
	strftime(message, sizeof(message), "%Y-%m-%dT%H:%M:%S", &utcTime);

	return message;
}


void CPictureMetadataExiv::ReadPicture(bool& hasGps, bool& hasDataTime, wxString& dateTimeInfos, wxString& latitude,
	wxString& longitude)
{
	hasGps = false;
	hasDataTime = false;
	if (isExif)
	{
		try
		{
			Exiv2::ExifData& exifData = exif->exifData();
			if (exifData.empty())
				return;

			latitude = "";
			wxString latitudeRef = "";
			longitude = "";
			wxString longitudeRef = "";

			Exiv2::ExifKey gpsTag("Exif.Image.GPSTag");
			Exiv2::ExifKey gpsLatitudeRef("Exif.GPSInfo.GPSLatitudeRef");
			Exiv2::ExifKey gpsLatitude("Exif.GPSInfo.GPSLatitude");
			Exiv2::ExifKey gpsLongitudeRef("Exif.GPSInfo.GPSLongitudeRef");
			Exiv2::ExifKey gpsLongitude("Exif.GPSInfo.GPSLongitude");
			Exiv2::ExifKey dateTime("Exif.Image.DateTime");

			Exiv2::ExifData::const_iterator md = exifData.findKey(dateTime);
			if (exifData.end() != md)
			{
				hasDataTime = true;			
				wxDateTime dt;
				if (dt.ParseFormat(toString(*md), "%Y-%m-%d"))
					dateTimeInfos = toString(*md);
				else
					hasDataTime = false;
			}

			md = exifData.findKey(gpsTag);
			if (exifData.end() != md)
			{
				hasGps = true;
				md = exifData.findKey(gpsLatitudeRef);
				if (exifData.end() != md)
				{
					latitudeRef = md->value().toString();
				}
				md = exifData.findKey(gpsLatitude);
				if (exifData.end() != md)
				{
					latitude = md->value().toString();
				}
				md = exifData.findKey(gpsLongitudeRef);
				if (exifData.end() != md)
				{
					longitudeRef = md->value().toString();
				}
				md = exifData.findKey(gpsLongitude);
				if (exifData.end() != md)
				{
					longitude = md->value().toString();
				}

				if (latitude != "" && longitude != "" && latitudeRef != "" && longitudeRef != "")
				{
					latitude = GetGpsfValue(latitude);
					longitude = GetGpsfValue(longitude);

					if (latitude == "" || longitude == "")
					{
						hasGps = false;
					}
					else
					{
						if (latitudeRef == "S")
							latitude = "-" + latitude;

						if (longitudeRef == "W")
							longitude = "-" + longitude;
					}
				}
				else
					hasGps = false;
			}
			else
				hasGps = false;
		}
		catch (...)
		{
		}
	}
}

tbb::concurrent_vector<CMetadata> CPictureMetadataExiv::ReadExif(Exiv2::ExifData& exifData)
{
	tbb::concurrent_vector<CMetadata> metadataList;

	Exiv2::ExifData::const_iterator end = exifData.end();

	for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i)
	{
		CMetadata metadata;
		metadata.key = i->key();
		metadata.value = toString(*i);

		metadataList.push_back(metadata);
	}
	return metadataList;
}

tbb::concurrent_vector<CMetadata> CPictureMetadataExiv::ReadXmp(Exiv2::XmpData& xmpData)
{
	tbb::concurrent_vector<CMetadata> metadataList;
	wxString exifinfos;
	wxString informations;
	bool apple = false;
	auto end = xmpData.end();
	for (auto md = xmpData.begin(); md != end; ++md)
	{
		informations = md->key();
		exifinfos = toString(*md);

		if (informations == "Xmp.video.MimeType" && exifinfos == "video/quicktime")
		{
			apple = true;
		}
		if (informations.Find("Date") >= 0 && apple)
		{
			int64_t dateTime = atol(exifinfos.c_str());
			if (dateTime > 0)
				exifinfos = GetQuickTimeDate(dateTime);
		}

		CMetadata metadata;
		metadata.key = informations;
		metadata.value = exifinfos;
		metadataList.push_back(metadata);
	}
	return metadataList;
}

tbb::concurrent_vector<CMetadata> CPictureMetadataExiv::ReadIpct(Exiv2::IptcData& ipctData)
{
	tbb::concurrent_vector<CMetadata> metadataList;
	wxString exifinfos;
	wxString informations;
	auto end = ipctData.end();
	for (auto md = ipctData.begin(); md != end; ++md)
	{
		informations = md->key();
		exifinfos = md->value().toString();

		if (md->typeId() == Exiv2::TypeId::unsignedByte)
		{
			Exiv2::Value::UniquePtr value = md->getValue();
			if (value.get())
			{
				std::vector<Exiv2::byte> buffer(value->size());
				long size = value->copy(buffer.data(), Exiv2::ByteOrder::invalidByteOrder);
				if (size > 0)
				{
					exifinfos = wxString::FromUTF8(reinterpret_cast<const char*>(buffer.data()),
						static_cast<size_t>(size));
				}
				else
				{
					exifinfos.clear();
				}
			}
			else
			{
				exifinfos.clear();
			}

			CMetadata metadata;
			metadata.key = informations;
			metadata.value = exifinfos;
			metadataList.push_back(metadata);
		}
	}
	return metadataList;
}

tbb::concurrent_vector<CMetadata> CPictureMetadataExiv::GetMetadata()
{
	tbb::concurrent_vector<CMetadata> metadataList;
	if (isExif && exif.get() != nullptr)
	{
		try
		{
			Exiv2::ExifData& exifData = exif->exifData();
			Exiv2::IptcData& ipctData = exif->iptcData();
			Exiv2::XmpData& xmpData = exif->xmpData();
			if (!exifData.empty())
			{
				metadataList = ReadExif(exifData);
			}
			else if (!ipctData.empty())
			{
				metadataList = ReadIpct(ipctData);
			}
			else if (!xmpData.empty())
			{
				metadataList = ReadXmp(xmpData);
			}
		}
		catch (...)
		{
		}
	}
	return metadataList;
}

wxImage CPictureMetadataExiv::LoadThumbnailFromExif(Exiv2::ExifData* dataIn, wxString& extension,
	int& orientation)
{
	wxImage image;
	if (dataIn != nullptr)
	{
		Exiv2::ExifThumb thumb(*dataIn);
		extension = thumb.extension();
		extension = extension.substr(1, extension.size() - 1);
		Exiv2::DataBuf data = thumb.copy();
		if (data.size() > 0 && data.data() != nullptr)
		{
			Exiv2::ExifKey orientationKey("Exif.Image.Orientation");
			Exiv2::ExifData::const_iterator md = dataIn->findKey(orientationKey);
			if (dataIn->end() != md)
			{
				wxString value = md->value().toString();
				orientation = atoi(value.c_str());
			}

			wxMemoryInputStream cxMemFile(data.data(), data.size());
			image.LoadFile(cxMemFile, wxBITMAP_TYPE_ANY);
		}
	}
	return image;
}

wxImage CPictureMetadataExiv::DecodeThumbnail(wxString& extension, int& orientation)
{
	wxImage bitmap;
	try
	{
		Exiv2::ExifData& exifData = exif->exifData();

		if (!exifData.empty())
		{
			bitmap = LoadThumbnailFromExif(&exifData, extension, orientation);
		}
	}
	catch (...)
	{
	}
	return bitmap;
}
#endif