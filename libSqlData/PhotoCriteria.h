#pragma once

class CPhotoCriteria
{
public:
    CPhotoCriteria() : numPhotoId(0), numCriteriaId(0) {}
    ~CPhotoCriteria() = default;

    void SetPhotoId(int numId) { numPhotoId = numId; }
    int GetPhotoId() const { return numPhotoId; }

    void SetCriteriaId(int numId) { numCriteriaId = numId; }
    int GetCriteriaId() const { return numCriteriaId; }

private:
    int numPhotoId;
    int numCriteriaId;
};

using PhotoCriteriaVector = std::vector<CPhotoCriteria>;
