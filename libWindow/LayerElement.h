// ==================== LayerElement.h ====================
#pragma once
#include <ImageLoadingFormat.h>

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

class LayerElement
{
public:
	LayerElement() = default;
	~LayerElement()
	{
		if (picture)
			delete picture;
	}

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
	}

	void SetPicture(CImageLoadingFormat* source)
	{
		if (picture)
			delete picture;

		picture = source;
	}

	CImageLoadingFormat* GetPicture()
	{
		return picture;
	}

	LayerBlendMode blendMode = LayerBlendMode::Normal;
	int numLayer;
	bool isVisible;
	int opacity;
	wxString layerName;

private:
	CImageLoadingFormat* picture = nullptr;
};


class CLayerList
{
public:
	CLayerList() = default;
	~CLayerList();

	void push_back(LayerElement* element);
	int size_back() const { return m_layers.size(); }
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
	int GetWidth();
	int GetHeight();
	void IsChanged(bool isChanged);

	void UpdatePictureToShow();

	// --- NOUVELLES FONCTIONS ---
	// Supprime un calque à l'index donné et libère sa mémoire
	bool erase(size_t index);

	// Copie le calque de l'index donné et l'ajoute à la liste
	bool copy(size_t index);

	// --- Nouvelles fonctions de manipulation des calques ---

// Fusionne les calques dont les indices sont fournis dans le vecteur.
// Les calques fusionnés sont supprimés et remplacés par un calque unique contenant le résultat.
	bool FusionLayers(std::vector<int> * listLayer);

	// Monte d'un niveau (index - 1 dans le vecteur, donc un plan plus haut visuellement)
	bool MoveUpLayer(size_t index);

	// Descend d'un niveau (index + 1 dans le vecteur, donc un plan plus bas visuellement)
	bool MoveDownLayer(size_t index);

	void CreateLayer(wxString layerName);

	bool SetPropertiesLayer(int numLayer, wxString layerName, LayerBlendMode blendMode, int opacity);

private:

	cv::Mat CreateCheckerboardBackground(int width, int height, int sizeSquare = 16);

	std::vector<LayerElement*> m_layers;
	std::vector<wxPoint> points;
	int selectType;
	CImageLoadingFormat* finalImage = nullptr;
	bool isChanged = false;
	int canvasWidth = 0;
	int canvasHeight = 0;
};
