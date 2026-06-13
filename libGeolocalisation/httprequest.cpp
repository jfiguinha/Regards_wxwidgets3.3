#include <header.h>
#include <httprequest.h>
#include <ConvertUtility.h>
#ifdef USECURL
#include <curl/curl.h>
#define SKIP_PEER_VERIFICATION
#define SKIP_HOSTNAME_VERIFICATION
#else
#include <wx/url.h>
#endif

using namespace Regards::Internet;

struct url_data
{
    size_t size;
    char* data;
};

size_t write_data(void* ptr,
    size_t size,
    size_t nmemb,
    struct url_data* data)
{
    const size_t bytesToCopy = size * nmemb;

    if (ptr == nullptr || data == nullptr)
        return 0;

    const size_t newSize = data->size + bytesToCopy;

    // Limite de sécurité : 1 Mo
    constexpr size_t MAX_DOWNLOAD_SIZE = 1024 * 1024;

    if (newSize > MAX_DOWNLOAD_SIZE)
    {
        fprintf(stderr, "Download exceeds maximum allowed size\n");
        return 0;
    }

    char* tmp = static_cast<char*>(
        realloc(data->data, newSize + 1));

    if (tmp == nullptr)
    {
        fprintf(stderr, "Failed to allocate memory\n");
        return 0;
    }

    data->data = tmp;

    memcpy(data->data + data->size,
        ptr,
        bytesToCopy);

    data->size = newSize;
    data->data[data->size] = '\0';

    return bytesToCopy;
}

wxString CHttpRequest::ExecuteRequest(const wxString& url)
{
    wxString xml;

    CURL* curl = curl_easy_init();
    if (curl == nullptr)
    {
        fprintf(stderr, "curl_easy_init failed\n");
        return "";
    }

    url_data data{};
    data.size = 0;
    data.data = static_cast<char*>(malloc(1));

    if (data.data == nullptr)
    {
        curl_easy_cleanup(curl);
        fprintf(stderr, "Memory allocation failed\n");
        return "";
    }

    data.data[0] = '\0';

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        CConvertUtility::ConvertToStdString(url).c_str());

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Timeouts
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // SSL sécurisé
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Callback de réception
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr,
            "curl_easy_perform failed: %s\n",
            curl_easy_strerror(res));

        free(data.data);
        curl_easy_cleanup(curl);

        return "";
    }

    long httpCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &httpCode);

    if (httpCode != 200)
    {
        fprintf(stderr,
            "HTTP Error: %ld\n",
            httpCode);

        free(data.data);
        curl_easy_cleanup(curl);

        return "";
    }

    xml = wxString(data.data, wxConvUTF8);

    free(data.data);
    curl_easy_cleanup(curl);

    return xml;
}
