// ==================== LayerElement.h ====================
#pragma once
#include <ImageLoadingFormat.h>

class LayerElement;

enum class LayerBlendMode
{
	Normal = 0,
	Multiply,
	Screen,
	Overlay,
	Darken,
	Lighten,
	Add,
	Subtract
};

namespace Regards
{
    namespace OpenGL
    {
        class GLTexture;
    }
}



class LayerElement
{
public:
    LayerElement() : m_glTexture(nullptr), needTextureUpdate(true)
    {
    };





	~LayerElement()
	{
		if (picture)
			delete picture;

        DeleteOpenGLTexture();
	}

    Regards::OpenGL::GLTexture * GetOpenGLTexture() const { return m_glTexture; }
    void SetOpenGLTexture(Regards::OpenGL::GLTexture * texture);
    void DeleteOpenGLTexture();

	// Constructeur de copie pour cloner proprement un calque
	LayerElement(const LayerElement& other)
	{
		numLayer = other.numLayer;
		isVisible = other.isVisible;
		blendMode = other.blendMode;
		opacity = other.opacity;
		layerName = other.layerName + " _copy"; // Ajoute un suffixe au nom

		if (other.picture)
		{
			// On suppose que CImageLoadingFormat possède un constructeur de copie 
			// ou une méthode pour cloner la matrice OpenCV interne.
			picture = new CImageLoadingFormat(*other.picture);
		}

        // IMPORTANT : On ne copie PAS le pointeur de texture OpenGL sous peine de double-free.
// On force la création d'une nouvelle texture GPU au premier rendu.
        m_glTexture = nullptr;
        needTextureUpdate = true;
	}



	void SetPicture(CImageLoadingFormat* source)
	{
		if (picture)
			delete picture;

		picture = source;

        needTextureUpdate = true;
	}

	CImageLoadingFormat* GetPicture()
	{
		return picture;
	}

	LayerBlendMode blendMode = LayerBlendMode::Normal;
	int numLayer = 0;
	bool isVisible = true;
	int opacity = 100;
	wxString layerName;
    bool needTextureUpdate = true;
private:
	CImageLoadingFormat* picture = nullptr;
    Regards::OpenGL::GLTexture * m_glTexture = nullptr;
};


class CLayerList
{
public:
	CLayerList() = default;
	~CLayerList();

	void push_back(LayerElement* element);
	int size_back() const { return static_cast<int>(m_layers.size()); }
	size_t size() const { return m_layers.size(); }
	bool empty() const { return m_layers.empty(); }

	LayerElement* operator[](size_t index) { return m_layers[index]; }
	const LayerElement* operator[](size_t index) const { return m_layers[index]; }

	auto begin() { return m_layers.begin(); }
	auto end() { return m_layers.end(); }
	auto begin() const { return m_layers.begin(); }
	auto end() const { return m_layers.end(); }

	void SetPicture(CImageLoadingFormat* bitmapIn);
	CImageLoadingFormat* GetPictureToShow();
	int GetWidth() const;
	int GetHeight() const;
	void IsChanged(bool changed);

	void UpdatePictureToShow();

	// --- NOUVELLES FONCTIONS ---
	// Supprime un calque à l'index donné et libère sa mémoire
	bool erase(size_t index);

	// Copie le calque de l'index donné et l'ajoute à la liste
	bool copy(size_t index);

	// --- Nouvelles fonctions de manipulation des calques ---

// Fusionne les calques dont les indices sont fournis dans le vecteur.
// Les calques fusionnés sont supprimés et remplacés par un calque unique contenant le résultat.
	bool FusionLayers(std::vector<int>* listLayer);

	// Monte d'un niveau (index - 1 dans le vecteur, donc un plan plus haut visuellement)
	bool MoveUpLayer(size_t index);

	// Descend d'un niveau (index + 1 dans le vecteur, donc un plan plus bas visuellement)
	bool MoveDownLayer(size_t index);

	void CreateLayer(wxString layerName);

	bool SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity);

    int GetActiveLayer()
    {
        return activeLayer;
    }

private:

	std::vector<LayerElement*> m_layers;
	std::vector<wxPoint> points;
	int selectType;
	CImageLoadingFormat* finalImage = nullptr;
	bool isChanged = false;
	int canvasWidth = 0;
	int canvasHeight = 0;
    int activeLayer = 0;
};



class LayerFonction
{
public:
    static inline float Clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    static inline float BlendChannel(float source, float destination, LayerBlendMode mode)
    {
        switch (mode)
        {
        case LayerBlendMode::Multiply:
            return source * destination;
        case LayerBlendMode::Screen:
            return source + destination - source * destination;
        case LayerBlendMode::Overlay:
            return destination <= 0.5f
                ? 2.0f * source * destination
                : 1.0f - 2.0f * (1.0f - source) * (1.0f - destination);
        case LayerBlendMode::Darken:
            return std::min(source, destination);
        case LayerBlendMode::Lighten:
            return std::max(source, destination);
        case LayerBlendMode::Add:
            return std::min(1.0f, source + destination);
        case LayerBlendMode::Subtract:
            return std::max(0.0f, destination - source);
        case LayerBlendMode::Normal:
        default:
            return source;
        }
    }

    // Normalise les images source en BGRA 8 bits. Les données d'entrée ne sont
    // converties/redimensionnées qu'une seule fois par calque et par composition.
    static cv::Mat PrepareLayerImage(const cv::Mat& input, int width, int height)
    {
        if (input.empty() || width <= 0 || height <= 0)
            return {};

        cv::Mat eightBit;
        if (input.depth() == CV_8U)
            eightBit = input;
        else
            input.convertTo(eightBit, CV_MAKETYPE(CV_8U, input.channels()));

        cv::Mat bgra;
        if (eightBit.channels() == 4)
            bgra = eightBit;
        else if (eightBit.channels() == 3)
            cv::cvtColor(eightBit, bgra, cv::COLOR_BGR2BGRA);
        else if (eightBit.channels() == 1)
            cv::cvtColor(eightBit, bgra, cv::COLOR_GRAY2BGRA);
        else
            return {};

        if (bgra.cols != width || bgra.rows != height)
        {
            cv::Mat resized;
            const int interpolation =
                (bgra.cols > width || bgra.rows > height) ? cv::INTER_AREA : cv::INTER_LINEAR;
            cv::resize(bgra, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
            return resized;
        }
        return bgra;
    }

    // Composition source-over avec alpha droit (straight alpha), et mode de
    // fusion appliqué aux couleurs. Aucun split/merge ni Mat flottant temporaire.
    static void CompositeLayer(cv::Mat& destination, const cv::Mat& source,
        float layerOpacity, LayerBlendMode mode)
    {
        if (destination.empty() || source.empty() || layerOpacity <= 0.0f)
            return;

        CV_Assert(destination.type() == CV_8UC4 && source.type() == CV_8UC4);
        const int width = destination.cols;

        for (int y = 0; y < destination.rows; ++y)
        {
            auto* dst = destination.ptr<cv::Vec4b>(y);
            const auto* src = source.ptr<cv::Vec4b>(y);

            for (int x = 0; x < width; ++x)
            {
                const float sourceAlpha = (src[x][3] / 255.0f) * layerOpacity;
                if (sourceAlpha <= 0.0f)
                    continue;

                const float destinationAlpha = dst[x][3] / 255.0f;
                const float outputAlpha = sourceAlpha + destinationAlpha * (1.0f - sourceAlpha);

                for (int c = 0; c < 3; ++c)
                {
                    const float sourceColor = src[x][c] / 255.0f;
                    const float destinationColor = dst[x][c] / 255.0f;
                    const float blendedColor = BlendChannel(sourceColor, destinationColor, mode);

                    // Formule W3C de composition avec mode de fusion :
                    // Co = (1-as)*ad*Cd + (1-ad)*as*Cs + as*ad*B(Cs,Cd)
                    const float premultiplied =
                        (1.0f - sourceAlpha) * destinationAlpha * destinationColor +
                        (1.0f - destinationAlpha) * sourceAlpha * sourceColor +
                        sourceAlpha * destinationAlpha * blendedColor;

                    const float outputColor = outputAlpha > 0.0f
                        ? premultiplied / outputAlpha
                        : 0.0f;
                    dst[x][c] = cv::saturate_cast<uchar>(Clamp01(outputColor) * 255.0f);
                }

                dst[x][3] = cv::saturate_cast<uchar>(Clamp01(outputAlpha) * 255.0f);
            }
        }
    }

    static bool CompositeOneLayer(cv::Mat& canvas, LayerElement* layer,
        int width, int height)
    {
        if (!layer || !layer->isVisible || !layer->GetPicture() ||
            !layer->GetPicture()->IsOk())
            return false;

        const float opacity = Clamp01(layer->opacity / 100.0f);
        if (opacity <= 0.0f)
            return false;

        cv::Mat source = PrepareLayerImage(layer->GetPicture()->GetMatImage(), width, height);
        if (source.empty())
            return false;

        CompositeLayer(canvas, source, opacity, layer->blendMode);
        return true;
    }
};