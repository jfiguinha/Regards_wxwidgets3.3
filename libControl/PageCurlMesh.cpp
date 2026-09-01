#include <header.h>
#include <algorithm>
#include <cmath>
#include "PageCurlMesh.h"

namespace {
    constexpr float PI = 3.14159265358979323846f;

    constexpr float DEG_TO_RAD = PI / 180.0f;

    /*
     * Rayon du cylindre dans l'espace normalisé.
     *
     * Plus la valeur est faible, plus la courbure
     * est serrée.
     */
    constexpr float CYLINDER_RADIUS = 0.18f;

    /*
     * Angle maximal de retournement.
     *
     * PI = 180 degrés.
     */
    constexpr float MAX_CURL_ANGLE = PI;

}  // namespace

CPageCurlMesh::CPageCurlMesh()
    : vao(0),
    vbo(0),
    ebo(0),
    columns(0),
    rows(0),
    renderWidth(0),
    renderHeight(0),
    renderLeft(0),
    renderTop(0),
    initialized(false),
    invertTex(false),
    diagonalAngle(30.0f),
    lastTime(-1.0f) {}

CPageCurlMesh::~CPageCurlMesh() { Release(); }

void CPageCurlMesh::Initialize(int numberOfColumns, int numberOfRows) {
    Release();

    columns = std::max(numberOfColumns, 1);

    rows = std::max(numberOfRows, 1);

    CreateGrid();
    CreateBuffers();

    initialized = true;

    lastTime = -1.0f;
}

void CPageCurlMesh::SetRenderSize(int width, int height, int left, int top) {
    width = std::max(width, 1);

    height = std::max(height, 1);

    const bool sizeChanged = renderWidth != width || renderHeight != height ||
        renderLeft != left || renderTop != top;

    renderWidth = width;
    renderHeight = height;
    renderLeft = left;
    renderTop = top;

    /*
     * Si le mesh existe déjà et que la taille change,
     * il faut régénérer les positions en pixels.
     */
    if (initialized && sizeChanged) {
        lastTime = -1.0f;
    }
}

bool CPageCurlMesh::IsInitialized() const { return initialized; }

void CPageCurlMesh::SetInvertTex(bool invert) {
    if (invertTex == invert) {
        return;
    }

    invertTex = invert;

    /*
     * Les coordonnées de texture changent.
     */
    lastTime = -1.0f;
}

bool CPageCurlMesh::GetInvertTex() const { return invertTex; }

void CPageCurlMesh::SetDiagonalAngle(float angleDegrees) {
    if (diagonalAngle == angleDegrees) {
        return;
    }

    diagonalAngle = angleDegrees;

    lastTime = -1.0f;
}

float CPageCurlMesh::GetDiagonalAngle() const { return diagonalAngle; }

void CPageCurlMesh::CreateGrid() {
    vertices.clear();
    originalVertices.clear();
    indices.clear();

    const size_t vertexCount =
        static_cast<size_t>(columns + 1) * static_cast<size_t>(rows + 1);

    vertices.reserve(vertexCount);
    originalVertices.reserve(vertexCount);

    /*
     * ---------------------------------------------------------
     * Création des sommets.
     *
     * La géométrie est initialement définie dans l'espace :
     *
     * X : 0.0 -> 1.0
     * Y : 0.0 -> 1.0
     * ---------------------------------------------------------
     */
    for (int y = 0; y <= rows; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(rows);

        for (int x = 0; x <= columns; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(columns);

            OriginalVertex original;

            original.x = u;
            original.y = v;

            original.u = u;
            original.v = v;

            originalVertices.push_back(original);

            PageVertex vertex;

            vertex.x = 0.0f;
            vertex.y = 0.0f;
            vertex.z = 0.0f;

            vertex.u = u;
            vertex.v = invertTex ? 1.0f - v : v;

            vertex.curlAmount = 0.0f;

            vertices.push_back(vertex);
        }
    }

    /*
     * ---------------------------------------------------------
     * Création des indices.
     *
     * Chaque cellule contient deux triangles.
     * ---------------------------------------------------------
     */
    const size_t indexCount =
        static_cast<size_t>(columns) * static_cast<size_t>(rows) * 6;

    indices.reserve(indexCount);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < columns; ++x) {
            const unsigned int topLeft =
                static_cast<unsigned int>(y * (columns + 1) + x);

            const unsigned int topRight = topLeft + 1;

            const unsigned int bottomLeft =
                static_cast<unsigned int>((y + 1) * (columns + 1) + x);

            const unsigned int bottomRight = bottomLeft + 1;

            /*
             * Triangle supérieur.
             */
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            /*
             * Triangle inférieur.
             */
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

void CPageCurlMesh::CreateBuffers() {
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    /*
     * ---------------------------------------------------------
     * VBO
     * ---------------------------------------------------------
     *
     * GL_DYNAMIC_DRAW car les positions sont modifiées
     * à chaque frame.
     */
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(PageVertex)),
        vertices.data(), GL_DYNAMIC_DRAW);

    /*
     * ---------------------------------------------------------
     * EBO
     * ---------------------------------------------------------
     *
     * Les indices ne changent jamais.
     */
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    /*
     * ---------------------------------------------------------
     * Attribut 0 : position
     *
     * layout(location = 0)
     * in vec3 position;
     * ---------------------------------------------------------
     */
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PageVertex),
        reinterpret_cast<void*>(offsetof(PageVertex, x)));

    /*
     * ---------------------------------------------------------
     * Attribut 1 : coordonnées de texture
     *
     * layout(location = 1)
     * in vec2 texCoord;
     * ---------------------------------------------------------
     */
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(PageVertex),
        reinterpret_cast<void*>(offsetof(PageVertex, u)));

    /*
     * ---------------------------------------------------------
     * Attribut 2 : intensité de courbure
     *
     * layout(location = 2)
     * in float curlAmount;
     * ---------------------------------------------------------
     */
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        2, 1, GL_FLOAT, GL_FALSE, sizeof(PageVertex),
        reinterpret_cast<void*>(offsetof(PageVertex, curlAmount)));

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CPageCurlMesh::Update(float time) {
    if (!initialized) {
        return;
    }

    /*
     * Le mesh doit avoir une taille valide.
     */
    if (renderWidth <= 0 || renderHeight <= 0) {
        return;
    }

    /*
     * Optimisation :
     *
     * si rien n'a changé, on ne recalcule pas
     * la totalité du mesh.
     */
    if (lastTime == time) {
        return;
    }

    lastTime = time;

    /*
     * ---------------------------------------------------------
     * Progression
     * ---------------------------------------------------------
     *
     * 0.0   -> page plate
     * 100.0 -> page retournée
     */
    const float progress = Clamp(time / 100.0f, 0.0f, 1.0f);

    /*
     * Le bord du curl se déplace de droite à gauche.
     */
    const float curlPosition = 1.0f - progress;

    /*
     * Direction diagonale.
     */
    const float angle = diagonalAngle * DEG_TO_RAD;

    const float cosAngle = std::cos(angle);

    const float sinAngle = std::sin(angle);

    /*
     * ---------------------------------------------------------
     * Mise à jour de tous les sommets
     * ---------------------------------------------------------
     */
    for (size_t i = 0; i < originalVertices.size(); ++i) {
        const OriginalVertex& original = originalVertices[i];

        PageVertex& vertex = vertices[i];

        /*
         * -----------------------------------------------------
         * 1. Translation au centre
         * -----------------------------------------------------
         */
        const float px = original.x - 0.5f;

        const float py = original.y - 0.5f;

        /*
         * -----------------------------------------------------
         * 2. Rotation du repère
         * -----------------------------------------------------
         *
         * On travaille dans un espace tourné afin que
         * le curl soit diagonal.
         */
        float rotatedX = px * cosAngle - py * sinAngle;

        float rotatedY = px * sinAngle + py * cosAngle;

        /*
         * Retour temporaire dans l'espace normalisé.
         */
        rotatedX += 0.5f;
        rotatedY += 0.5f;

        /*
         * -----------------------------------------------------
         * 3. Distance au bord de retournement
         * -----------------------------------------------------
         */
        const float distance = rotatedX - curlPosition;

        float finalX = rotatedX;

        float finalY = rotatedY;

        float finalZ = 0.0f;

        float curlAmount = 0.0f;

        /*
         * -----------------------------------------------------
         * 4. Déformation cylindrique
         * -----------------------------------------------------
         */
        if (distance > 0.0f) {
            float theta = distance / CYLINDER_RADIUS;

            theta = std::min(theta, MAX_CURL_ANGLE);

            /*
             * Cylindre :
             *
             * x = R * sin(theta)
             * z = R * (1 - cos(theta))
             */
            finalX = curlPosition + CYLINDER_RADIUS * std::sin(theta);

            finalZ = CYLINDER_RADIUS * (1.0f - std::cos(theta));

            curlAmount = Clamp(theta / MAX_CURL_ANGLE, 0.0f, 1.0f);
        }

        /*
         * -----------------------------------------------------
         * 5. Rotation inverse
         * -----------------------------------------------------
         */
        float localX = finalX - 0.5f;

        float localY = finalY - 0.5f;

        float normalizedX = localX * cosAngle + localY * sinAngle + 0.5f;

        float normalizedY = -localX * sinAngle + localY * cosAngle + 0.5f;

        /*
         * -----------------------------------------------------
         * 6. Conversion en coordonnées pixels
         * -----------------------------------------------------
         *
         * X et Y sont maintenant compatibles avec
         * RenderQuad(width, height, left, top).
         *
         * Z est également mis à l'échelle afin que
         * la profondeur reste proportionnelle à la taille.
         */
        vertex.x = static_cast<float>(renderLeft) +
            normalizedX * static_cast<float>(renderWidth);

        vertex.y = static_cast<float>(renderTop) +
            normalizedY * static_cast<float>(renderHeight);

        /*
         * La profondeur utilise la plus petite dimension
         * pour conserver une déformation visuellement
         * cohérente.
         */
        vertex.z = finalZ * static_cast<float>(std::min(renderWidth, renderHeight));

        /*
         * -----------------------------------------------------
         * 7. Coordonnées de texture
         * -----------------------------------------------------
         */
        vertex.u = original.u;

        vertex.v = invertTex ? 1.0f - original.v : original.v;

        vertex.curlAmount = curlAmount;
    }

    UploadVertices();
}

void CPageCurlMesh::UploadVertices() {
    if (vbo == 0 || vertices.empty()) {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferSubData(GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(PageVertex)),
        vertices.data());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void CPageCurlMesh::Render() const {
    if (!initialized || vao == 0 || indices.empty()) {
        return;
    }

    glBindVertexArray(vao);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

void CPageCurlMesh::Release() {
    /*
     * Attention :
     *
     * Release() doit être appelé avec un contexte OpenGL
     * valide et courant.
     */

    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);

        ebo = 0;
    }

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);

        vbo = 0;
    }

    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);

        vao = 0;
    }

    vertices.clear();
    originalVertices.clear();
    indices.clear();

    columns = 0;
    rows = 0;

    renderWidth = 0;
    renderHeight = 0;

    renderLeft = 0;
    renderTop = 0;

    initialized = false;

    lastTime = -1.0f;
}

GLuint CPageCurlMesh::GetVAO() const { return vao; }

GLsizei CPageCurlMesh::GetIndexCount() const {
    return static_cast<GLsizei>(indices.size());
}

float CPageCurlMesh::Clamp(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(value, maximum));
}
