#pragma once

#include <cstddef>
#include <vector>

class CPageCurlMesh {
public:
    struct PageVertex {
        float x;
        float y;
        float z;

        float u;
        float v;

        // 0.0 = partie plate
        // 1.0 = partie complètement enroulée
        float curlAmount;
    };

public:
    CPageCurlMesh();
    ~CPageCurlMesh();

    CPageCurlMesh(const CPageCurlMesh&) = delete;
    CPageCurlMesh& operator=(const CPageCurlMesh&) = delete;

    // Crée la grille.
    //
    // Exemple :
    // Initialize(64, 64)
    //
    // = 4225 sommets et 24576 indices.
    void Initialize(int columns = 64, int rows = 64);

    // Définit la zone de rendu en coordonnées pixels.
    //
    // Le mesh final sera généré entre :
    //
    // x = left               -> left + width
    // y = top                -> top + height
    //
    void SetRenderSize(int width, int height, int left = 0, int top = 0);

    // Met à jour la déformation.
    //
    // time :
    //   0.0   -> page plate
    // 100.0   -> page complètement retournée
    //
    void Update(float time);

    // Rend le VAO/EBO.
    void Render() const;

    // Libération des ressources OpenGL.
    void Release();

    bool IsInitialized() const;

    void SetInvertTex(bool invert);
    bool GetInvertTex() const;

    // Direction diagonale du retournement.
    //
    // Valeur par défaut : 30 degrés.
    void SetDiagonalAngle(float angleDegrees);

    float GetDiagonalAngle() const;

    GLuint GetVAO() const;
    GLsizei GetIndexCount() const;

private:
    struct OriginalVertex {
        // Coordonnées normalisées de la grille.
        float x;
        float y;

        // Coordonnées de texture.
        float u;
        float v;
    };

private:
    void CreateGrid();
    void CreateBuffers();
    void UploadVertices();

    static float Clamp(float value, float minimum, float maximum);

private:
    std::vector<PageVertex> vertices;
    std::vector<OriginalVertex> originalVertices;
    std::vector<unsigned int> indices;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    int columns;
    int rows;

    // Dimensions de sortie en pixels.
    int renderWidth;
    int renderHeight;

    // Position de sortie en pixels.
    int renderLeft;
    int renderTop;

    bool initialized;
    bool invertTex;

    float diagonalAngle;

    // Dernière valeur utilisée.
    float lastTime;
};
