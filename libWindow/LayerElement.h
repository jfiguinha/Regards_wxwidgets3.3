#pragma once
#include <ImageLoadingFormat.h>

class LayerElement
{
public:

	LayerElement() = default;
	~LayerElement()
	{
		if (picture)
			delete picture;
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

    // --- Fonctions simulant le comportement d'un std::vector ---

    // Ajouter un élément à la liste
	void push_back(LayerElement* element);

    // Obtenir le nombre de calques
    int size_back() const { return m_layers.size(); }
    size_t size() const { return m_layers.size(); }
    bool empty() const { return m_layers.empty(); }

    // Accès aux éléments (opérateur entre crochets)
    LayerElement * operator[](size_t index) { return m_layers[index]; }
    const LayerElement * operator[](size_t index) const { return m_layers[index]; }

    // Itérateurs pour permettre l'utilisation des boucles "for (auto& layer : list)"
    auto begin() { return m_layers.begin(); }
    auto end() { return m_layers.end(); }
    auto begin() const { return m_layers.begin(); }
    auto end() const { return m_layers.end(); }

    // --- Vos fonctions personnalisées (Logique métier) ---
	void SetCanvasSize(const int &width, const int &height);
	void SetPicture(CImageLoadingFormat* bitmapIn);

	CImageLoadingFormat* GetPictureToShow();

	int GetWidth();
	int GetHeight();

private:
    std::vector<LayerElement *> m_layers;


	std::vector<wxPoint> points;
	int selectType;
	CImageLoadingFormat* finalImage = nullptr;
	bool isChanged = false;
	int canvasWidth = 0;
	int canvasHeight = 0;
};