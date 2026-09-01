#include <header.h>
#include "PageCurlFilter.h"
#include <BitmapDisplay.h>
#include <GLTexture.h>
#include <ImageLoadingFormat.h>
#include <RGBAQuad.h>

#include "effect_id.h"

using namespace Regards::Filter;
using namespace Regards::OpenGL;

CPageCurlFilter::CPageCurlFilter()
{
    initTexture = true;
    meshInitialized = false;
    meshWidth = 0;
    meshHeight = 0;
}

CPageCurlFilter::~CPageCurlFilter() { pageCurlMesh.Release(); }

void CPageCurlFilter::RenderTexture(CRenderBitmapOpenGL* renderOpenGL,
    const float& time, const float& invert,
    const int& width, const int& height,
    const int& left, const int& top) {
    if (renderOpenGL == nullptr || pictureFirst == nullptr ||
        pictureNext == nullptr) {
        return;
    }

    /*
     * ---------------------------------------------------------
     * 1. Initialisation du mesh
     * ---------------------------------------------------------
     *
     * Une grille 64 x 64 donne :
     *
     * (64 + 1) * (64 + 1) = 4225 sommets
     *
     * C'est largement suffisant pour une transition fluide.
     *
     * Le mesh est recréé uniquement lorsque nécessaire.
     */
    if (!meshInitialized || meshWidth != width || meshHeight != height) {
        pageCurlMesh.Release();

        pageCurlMesh.Initialize(64, 64);

        meshInitialized = true;
        meshWidth = width;
        meshHeight = height;
    }

    /*
     * ---------------------------------------------------------
     * 2. Configuration des textures
     * ---------------------------------------------------------
     */

    glActiveTexture(GL_TEXTURE0);
    pictureFirst->Enable();

    glActiveTexture(GL_TEXTURE1);
    pictureNext->Enable();

    /*
     * ---------------------------------------------------------
     * 3. Dessin de l'image suivante en arrière-plan
     * ---------------------------------------------------------
     *
     * Avec l'ancien shader :
     *
     * targetTex était calculée directement dans le fragment
     * shader.
     *
     * Maintenant, nous avons une vraie géométrie 3D.
     * Nous dessinons donc pictureNext avant la page.
     */

    glDisable(GL_DEPTH_TEST);

    renderOpenGL->GetRenderOpengl()->RenderQuad(pictureNext.get(), width, height,
        false, false, left, top, false);

    /*
     * ---------------------------------------------------------
     * 4. Mise à jour de la géométrie
     * ---------------------------------------------------------
     *
     * time attendu :
     *
     * 0   -> page plate
     * 50  -> page à moitié retournée
     * 100 -> page retournée
     */
    pageCurlMesh.SetInvertTex(invert != 0.0f);

    pageCurlMesh.SetRenderSize(
        width,
        height,
        left,
        top);

    pageCurlMesh.Update(time);

    /*
     * ---------------------------------------------------------
     * 5. Activation du shader du PageCurl
     * ---------------------------------------------------------
     *
     * Le shader ne fait plus la déformation.
     *
     * Il fait uniquement :
     *
     * - affichage de sourceTex
     * - gestion recto / verso
     * - ombrage du verso
     */
    COpenGLShader* shader = renderOpenGL->FindShader(L"IDR_GLSL_PAGECURL_MESH", GL_FRAGMENT_SHADER, L"IDR_GLSL_VERTEX_MESH");

    if (shader == nullptr) {
        return;
    }

    shader->EnableShader(renderOpenGL->GetProjectionMatrix());

    /*
     * ---------------------------------------------------------
     * 6. Liaison de la texture source
     * ---------------------------------------------------------
     *
     * Le nouveau fragment shader utilise uniquement sourceTex.
     *
     * targetTex n'est plus nécessaire puisque pictureNext a
     * déjà été dessinée derrière le mesh.
     */
    glActiveTexture(GL_TEXTURE0);
    pictureFirst->Enable();

    if (!shader->m_pShader->SetTexture("sourceTex", pictureFirst->GetTextureID(),
        0)) {
        printf("SetTexture sourceTex failed\n");
    }

    /*
     * ---------------------------------------------------------
     * 7. Blending
     * ---------------------------------------------------------
     */
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /*
     * ---------------------------------------------------------
     * 8. Rendu du mesh
     * ---------------------------------------------------------
     *
     * Important :
     *
     * On désactive le culling car nous voulons afficher
     * le recto ET le verso de la page.
     *
     * Le fragment shader utilise gl_FrontFacing pour savoir
     * quelle face est actuellement visible.
     */
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);

    if (cullFaceEnabled) {
        glDisable(GL_CULL_FACE);
    }

    pageCurlMesh.Render();

    /*
     * ---------------------------------------------------------
     * 9. Restauration du CULL_FACE
     * ---------------------------------------------------------
     */
    if (cullFaceEnabled) {
        glEnable(GL_CULL_FACE);
    }

    /*
     * ---------------------------------------------------------
     * 10. Désactivation du shader
     * ---------------------------------------------------------
     */
    shader->DisableShader();

    /*
     * ---------------------------------------------------------
     * 11. Nettoyage
     * ---------------------------------------------------------
     */
    glDisable(GL_BLEND);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int CPageCurlFilter::GetTypeFilter() { return IDM_AFTEREFFECT_PAGECURL; }

void CPageCurlFilter::SetTransitionBitmap(const bool& start,
    IBitmapDisplay* bmpViewer,
    CImageLoadingFormat* bmpSecond) {
    initTexture = true;

    meshInitialized = false;

    bmpViewer->StartTransitionEffect(bmpSecond, false);
}

bool CPageCurlFilter::RenderTexture(CImageLoadingFormat* nextPicture,
    CImageLoadingFormat* source,
    IBitmapDisplay* bmpViewer,
    CRenderBitmapOpenGL* renderOpenGL,
    const float& scale_factor,
    const int& etape) {
    if (etape > 0 && etape < 110) {
        GenerateTexture(nextPicture, source, bmpViewer);

        int widthOutput = static_cast<int>(bmpViewer->GetWidth() * scale_factor);

        int heightOutput = static_cast<int>(bmpViewer->GetHeight() * scale_factor);

        RenderTexture(renderOpenGL, static_cast<float>(etape), false, widthOutput,
            heightOutput, 0, 0);

        return true;
   }

   return false;
}

void CPageCurlFilter::GenerateTexture(CImageLoadingFormat* nextPicture,
    CImageLoadingFormat* source,
    IBitmapDisplay* bmpViewer) {
    bool init = false;

    std::unique_ptr<CImageLoadingFormat> bitmapOut;

    std::unique_ptr<CImageLoadingFormat> bitmapFirst =
        std::make_unique<CImageLoadingFormat>();

    /*
     * ---------------------------------------------------------
     * Image suivante
     * ---------------------------------------------------------
     */
    if (initTexture || (pictureFirst->GetWidth() != bmpViewer->GetWidth() &&
        pictureFirst->GetHeight() != bmpViewer->GetHeight())) {
        init = true;
        initTexture = false;
    }

    if (init) {
        CRgbaquad colorBack = bmpViewer->GetBackColor();

        auto mat = cv::Mat(bmpViewer->GetHeight(), bmpViewer->GetWidth(), CV_8UC4,
            cv::Scalar(colorBack.GetBlue(), colorBack.GetGreen(),
                colorBack.GetRed(), 255));

        bitmapFirst->SetPicture(mat);

        bitmapOut.reset(GenerateInterpolationBitmapTexture(nextPicture, bmpViewer));

        if (bitmapOut != nullptr) {
            bitmapFirst->InsertBitmap(bitmapOut.get(), out.x, out.y);
        }

        mat = bitmapFirst->GetMatrix().getMat();

        cv::flip(mat, mat, 0);

        Regards::Picture::CPictureArray pictureArray =
            Regards::Picture::CPictureArray(mat);

        pictureNext->SetData(pictureArray, nullptr);
    }

    /*
     * ---------------------------------------------------------
     * Image actuelle
     * ---------------------------------------------------------
     */
    if (init) {
        CRgbaquad colorBack = bmpViewer->GetBackColor();

        auto mat = cv::Mat(bmpViewer->GetHeight(), bmpViewer->GetWidth(), CV_8UC4,
            cv::Scalar(colorBack.GetBlue(), colorBack.GetGreen(),
                colorBack.GetRed(), 255));

        bitmapFirst->SetPicture(mat);

        bitmapOut.reset(GenerateInterpolationBitmapTexture(source, bmpViewer));

        if (bitmapOut != nullptr) {
            bitmapFirst->InsertBitmap(bitmapOut.get(), out.x, out.y);

            bitmapFirst->Flip();
        }

        pictureFirst->SetData(bitmapFirst->GetMatrix(), nullptr);
    }
}

GLTexture* CPageCurlFilter::GetTexture(const int& numTexture) {
    if (numTexture == 0) {
        return pictureFirst.get();
    }

    return pictureNext.get();
}
