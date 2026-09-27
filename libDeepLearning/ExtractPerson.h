#pragma once
class CExtractPerson
{
public:
    CExtractPerson();
    ~CExtractPerson();
    int load();
    cv::Mat extractPerson(const cv::Mat& bgr);
    bool IsLoaded();
private:

    static bool isLoad;
};

