// ReSharper disable All
#include <header.h>
// stdafx.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets qui sont utilisés fréquemment,
// et sont rarement modifiés
//
#include "RenderOpenGL.h"
#include <OpenCLContext.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <FileUtility.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#include <utility.h>
#include <ParamInit.h>
#include <RegardsConfigParam.h>
#include <GLCharacter.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __APPLE__
#define USE_GLUT
#endif

class CFreeTypeFace
{
public:
	CFreeTypeFace() {};
	FT_Face face;
};

#include <appcontext.h>
extern AppContext application_context;

using namespace Regards::OpenGL;
using namespace Regards::OpenCL;

static inline void FillTexCoords(GLfloat* tex,
	bool inverted,
	bool flipH,
	bool flipV)
{
	float left = flipH ? 1.0f : 0.0f;
	float right = flipH ? 0.0f : 1.0f;
	float top = flipV ? 1.0f : 0.0f;
	float bottom = flipV ? 0.0f : 1.0f;

	if (inverted)
		std::swap(top, bottom);

	// Organisation optimisée pour GL_TRIANGLE_STRIP (Z-pattern)
	tex[0] = left;  tex[1] = top;
	tex[2] = right; tex[3] = top;
	tex[4] = left;  tex[5] = bottom;
	tex[6] = right; tex[7] = bottom;
}


CRenderOpenGL::CRenderOpenGL(wxGLCanvas* canvas)
	: wxGLContext(canvas), base(0), myGLVersion(0), mouseUpdate(nullptr)
{
	width = 0;
	height = 0;
	openCLContext = std::make_unique<COpenCLContext>();
}


bool CRenderOpenGL::IsInit()
{
	return isInit;
}

GLTexture* CRenderOpenGL::GetTextureDisplay()
{
	return textureDisplay.get();
}

bool CRenderOpenGL::GetOpenGLInterop()
{
	return application_context.openclOpenGLInterop;
}


// ... (CRenderOpenGL::CRenderOpenGL, IsInit, GetTextureDisplay, GetOpenGLInterop restent inchangés) ...

void CRenderOpenGL::Init(wxGLCanvas* canvas)
{
	if (isInit || canvas == nullptr)
		return;

	SetCurrent(*canvas);

	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

	printf("OpenGL: %s\n", version);
	printf("GLSL: %s\n", glslVersion);

	application_context.isOpenCLInitialized = false;
	application_context.openclOpenGLInterop = false;

	CRegardsConfigParam* regardsParam = CParamInit::getInstance();

	if (regardsParam != nullptr && openCLContext != nullptr)
	{
		openCLContext->AssociateToVulkan();
		const bool openCLAvailable = cv::ocl::haveOpenCL() && regardsParam->GetIsOpenCLSupport();

		if (openCLAvailable)
		{
			const bool wantOpenGLOpenCLInterop = regardsParam->GetIsOpenCLOpenGLInteropSupport();
			if (wantOpenGLOpenCLInterop)
			{
				try
				{
					openCLContext->initializeContextFromGL();
					application_context.isOpenCLInitialized = true;
					application_context.openclOpenGLInterop = true;
				}
				catch (const cv::Exception& e)
				{
					std::cout << "OpenCL/OpenGL interop failed: " << e.what() << std::endl;
					try
					{
						openCLContext->CreateDefaultOpenCLContext();
						application_context.isOpenCLInitialized = true;
					}
					catch (const cv::Exception& fallbackException)
					{
						std::cout << "OpenCL fallback failed: " << fallbackException.what() << std::endl;
					}
				}
			}
			else
			{
				try
				{
					openCLContext->CreateDefaultOpenCLContext();
					application_context.isOpenCLInitialized = true;
				}
				catch (const cv::Exception& e)
				{
					std::cout << "OpenCL initialization failed: " << e.what() << std::endl;
				}
			}
		}

		regardsParam->SetIsOpenCLSupport(application_context.isOpenCLInitialized);
		regardsParam->SetIsOpenCLOpenGLInteropSupport(application_context.openclOpenGLInterop);
	}

	myGLVersion = 3.3f; // Forcé ou parsé pour valider le Core Profile
	isInit = true;
	textureDisplay = std::make_unique<GLTexture>();

#ifndef USE_GLUT
	LoadFont("Antonio-Bold.ttf");
#endif

	InitTextBuffers();
}

void CRenderOpenGL::PrintSubtitle(int x, int y, double scale_factor, wxString text)
{
	float font_height = 15;
    void * font_choose = GLUT_BITMAP_TIMES_ROMAN_24;
	float font_width = glutBitmapWidth(font_choose, 'x');
    int xPos = 0;

	std::vector<wxString> list = CConvertUtility::split(text, '\\');
	if (list.size() > 0)
	{
		wxString line = list[0];
        xPos = x - ((font_width * line.size()) / 2);
		glWindowPos2i(xPos, y);
		//get the length of the string to display
		int len = static_cast<int>(line.Length());

		//glScalef(scale_factor,scale_factor,scale_factor); 
        int xPosition = 0;
		//loop to display character by character
		for (auto i = 0; i < len; i++)
		{
			wxUniChar c = line[i];
			char letter;
			c.GetAsChar(&letter);
			glutBitmapCharacter(font_choose, c);
            xPosition += font_width;
		}

		for (int i = 1;i < list.size();i++)
		{
			wxUniChar c = list[i][0];
			if (c == 'N')
			{
				//New Line
				wxString line = list[i];
				glWindowPos2i(x - ((font_width * line.size()) / 2), y - font_height * 2);
				//get the length of the string to display
				int len = static_cast<int>(line.Length());

				//glScalef(scale_factor,scale_factor,scale_factor); 

				//loop to display character by character
				for (auto i = 1; i < len; i++)
				{
					wxUniChar c = line[i];
					char letter;
					c.GetAsChar(&letter);
					glutBitmapCharacter(font_choose, c);
				}
			}
            else
            {
				wxString line = list[i];
				glWindowPos2i(xPos + xPosition + font_width, y - font_height * 2);
				//get the length of the string to display
				int len = static_cast<int>(line.Length());

				//glScalef(scale_factor,scale_factor,scale_factor); 

				//loop to display character by character
				for (auto i = 1; i < len; i++)
				{
					wxUniChar c = line[i];
					char letter;
					c.GetAsChar(&letter);
					glutBitmapCharacter(font_choose, c);
				}
            }
		}
	}
}



void CRenderOpenGL::UpdateProjectionMatrix() {
	int renderWidth = width;
	int renderHeight = height;

	if (renderWidth <= 0) renderWidth = 1;

	if (renderHeight <= 0) renderHeight = 1;

	const float left = 0.0f;
	const float right = static_cast<float>(renderWidth);

	const float bottom = 0.0f;
	const float top = static_cast<float>(renderHeight);

	const float nearValue = -1.0f;
	const float farValue = 1.0f;

	projectionMatrix[0] = 2.0f / (right - left);

	projectionMatrix[1] = 0.0f;
	projectionMatrix[2] = 0.0f;
	projectionMatrix[3] = 0.0f;

	projectionMatrix[4] = 0.0f;

	projectionMatrix[5] = 2.0f / (top - bottom);

	projectionMatrix[6] = 0.0f;
	projectionMatrix[7] = 0.0f;

	projectionMatrix[8] = 0.0f;
	projectionMatrix[9] = 0.0f;

	projectionMatrix[10] = -2.0f / (farValue - nearValue);

	projectionMatrix[11] = 0.0f;

	projectionMatrix[12] = -(right + left) / (right - left);

	projectionMatrix[13] = -(top + bottom) / (top - bottom);

	projectionMatrix[14] = -(farValue + nearValue) / (farValue - nearValue);

	projectionMatrix[15] = 1.0f;
}

COpenGLShader * CRenderOpenGL::FindShader(const wxString& shaderName,
                                      GLenum shaderType,
                                      const wxString& vertexName)
{
    auto it = shaderMap.find(shaderName);

    if (it != shaderMap.end())
        return it->second.get();

    auto shader = std::make_unique<COpenGLShader>();

	shader->m_pShader = std::make_unique<GLSLShader>();
	shader->m_pShader->CreateProgram(vertexName, GL_VERTEX_SHADER);
	shader->m_pShader->CreateProgram(shaderName, shaderType);

	//COpenGLShader * result = shader->m_pShader.get();

    shaderMap[shaderName] = std::move(shader);

    return  shaderMap[shaderName].get();
}

CRenderOpenGL::~CRenderOpenGL()
{
	if (textVAO != 0) glDeleteVertexArrays(1, &textVAO);
	if (textVBO != 0) glDeleteBuffers(1, &textVBO);
	if (textEBO != 0) glDeleteBuffers(1, &textEBO);
}

wxGLContext* CRenderOpenGL::GetGLContext()
{
	return this;
}

void CRenderOpenGL::Print(int x, int y, double scale_factor, const char* text)
{

#ifdef USE_GLUT	

    float font_height = 15;
    
    if(scale_factor > 1.0f)
        font_height = font_height * 2;

    //glPushMatrix();
	//glRasterPos2f(x, height - font_height);
    //glLoadIdentity();
	glWindowPos2i(x, height - font_height);
    
    
    //glColor4f(0.5, 0.8f, 0.2f, 1.0f);   
	//get the length of the string to display
	int len = static_cast<int>(strlen(text));
        
    //glScalef(scale_factor,scale_factor,scale_factor); 

	//loop to display character by character
	for (auto i = 0; i < len; i++)
	{
        if(scale_factor > 1.0f)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
        else
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
	}
    
    //glPopMatrix();
    //glColor4f(1,1,1,1);

#else

	RenderText(text, x, height - (heightFont * 0.3 * scale_factor), 0.3f * scale_factor, vec3f(0.5, 0.8f, 0.2f));

#endif
}

float CRenderOpenGL::CalculateTextWidth(const wxString& text, float scale)
{
	float totalWidth = 0.0f;
	for (size_t i = 0; i < text.size(); ++i)
	{
		auto it = Characters.find(text[i]);
		if (it != Characters.end())
		{
			totalWidth += (it->second.Advance >> 6) * scale;
		}
	}
	return totalWidth;
}

void CRenderOpenGL::PrintSubtitle(int x, int y, double scale_factor, float red, float green, float blue, wxString text)
{
#ifdef USE_GLUT	
	// (Conserver votre code GLUT d'origine si nécessaire, inchangé ici)
	float font_height = 15;
	void* font_choose = GLUT_BITMAP_TIMES_ROMAN_24;
	float font_width = glutBitmapWidth(font_choose, 'x');
	int xPos = 0;
	glColor3f(red, green, blue);
	std::vector<wxString> list = CConvertUtility::split(text, '\\');
	if (list.size() > 0)
	{
		wxString line = list[0];
		xPos = x - ((font_width * line.size()) / 2);
		glWindowPos2i(xPos, y);
		int len = static_cast<int>(line.Length());
		int xPosition = 0;
		for (auto i = 0; i < len; i++)
		{
			glutBitmapCharacter(font_choose, line[i]);
			xPosition += font_width;
		}

		for (int i = 1; i < list.size(); i++)
		{
			if (list[i].empty()) continue;
			wxUniChar c = list[i][0];
			if (c == 'N' || c == 'n')
			{
				wxString line = list[i];
				glWindowPos2i(x - ((font_width * line.size()) / 2), y - font_height * 2);
				int len = static_cast<int>(line.Length());
				for (auto j = 1; j < len; j++)
				{
					glutBitmapCharacter(font_choose, line[j]);
				}
			}
			else
			{
				wxString line = list[i];
				glWindowPos2i(xPos + xPosition + font_width, y - font_height * 2);
				int len = static_cast<int>(line.Length());
				for (auto j = 1; j < len; j++)
				{
					glutBitmapCharacter(font_choose, line[j]);
				}
			}
		}
	}
#else   
	// ═══════════════════════════════════════════════════════════════════════
	// NOUVEAU PIPELINE - OPENGL 3.3 CORE (Ajustement automatique de la hauteur)
	// ═══════════════════════════════════════════════════════════════════════
	std::vector<wxString> list = CConvertUtility::split(text, '\\');
	if (list.empty())
		return;

	const float fRed = red / 255.0f;
	const float fGreen = green / 255.0f;
	const float fBlue = blue / 255.0f;
	const vec3f textColor(fRed, fGreen, fBlue);

	const float scale = static_cast<float>(scale_factor);
	// Hauteur d'une ligne avec son espacement (interligne)
	const float lineHeight = (heightFont > 0 ? static_cast<float>(heightFont) : 24.0f) * scale * 1.5f;

	// ─── ÉTAPE 1 : COMPTER LE NOMBRE RÉEL DE LIGNES VERTICALES ───
	int totalLines = 1; // La première ligne (index 0) existe toujours
	for (size_t i = 1; i < list.size(); i++)
	{
		if (!list[i].empty() && (list[i][0] == 'N' || list[i][0] == 'n'))
		{
			totalLines++;
		}
	}

	// ─── ÉTAPE 2 : COMPENSER LA HAUTEUR GLOBALE ───
	// Si nous avons 3 lignes, nous voulons que le milieu du bloc (ou sa base) 
	// reste stable. On remonte le point de départ vertical 'currentY'.
	// (totalLines - 1) * lineHeight / 2.0f permet de centrer verticalement le bloc autour de 'y'.
	float currentY = static_cast<float>(y) + ((totalLines - 1) * lineHeight / 2.0f);


	// ─── ÉTAPE 3 : RENDU GÉOMÉTRIQUE ───
	// Traitement et rendu de la toute première ligne (Index 0)
	wxString firstLine = list[0];
	float firstLineWidth = CalculateTextWidth(firstLine, scale);
	float xPos = static_cast<float>(x) - (firstLineWidth / 2.0f);

	RenderText(firstLine, xPos, currentY, scale, textColor);

	float xPositionAccumulated = firstLineWidth;

	// Analyse des blocs suivants (ceux qui étaient précédés d'un '\')
	for (size_t i = 1; i < list.size(); i++)
	{
		if (list[i].empty())
			continue;

		wxUniChar c = list[i][0];

		if (c == 'N' || c == 'n')
		{
			// Cas 1 : Vrai saut de ligne (\N ou \n)
			wxString cleanLine = list[i].SubString(1, list[i].size() - 1);

			// On descend verticalement d'une ligne
			currentY -= lineHeight;

			float currentLineWidth = CalculateTextWidth(cleanLine, scale);
			float newXPos = static_cast<float>(x) - (currentLineWidth / 2.0f);

			RenderText(cleanLine, newXPos, currentY, scale, textColor);

			xPos = newXPos;
			xPositionAccumulated = currentLineWidth;
		}
		else
		{
			// Cas 2 : Caractère spécial / Balise de formatage (Reste sur la même ligne)
			wxString cleanLine = list[i].SubString(1, list[i].size() - 1);

			float spaceWidth = CalculateTextWidth(L" ", scale);
			float inlineXPos = xPos + xPositionAccumulated + spaceWidth;

			RenderText(cleanLine, inlineXPos, currentY, scale, textColor);

			xPositionAccumulated += spaceWidth + CalculateTextWidth(cleanLine, scale);
		}
	}
#endif

}

void CRenderOpenGL::RenderQuadInternal(float width, float height, int left, int top, bool inverted, bool flipH, bool flipV)
{
	// Définition des sommets pour un GL_TRIANGLE_STRIP (Triangle 1: Haut-Gauche, Haut-Droite, Bas-Gauche; Triangle 2: Bas-Droite)
	const GLfloat vertices[8] =
	{
		static_cast<GLfloat>(left),         static_cast<GLfloat>(top),
		static_cast<GLfloat>(left + width), static_cast<GLfloat>(top),
		static_cast<GLfloat>(left),         static_cast<GLfloat>(top + height),
		static_cast<GLfloat>(left + width), static_cast<GLfloat>(top + height)
	};

	GLfloat texCoords[8];
	FillTexCoords(texCoords, inverted, flipH, flipV);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

	// Utilisation de GL_TRIANGLE_STRIP à la place de GL_QUADS (Interdit en Core Profile)
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

GLvoid CRenderOpenGL::ReSizeGLScene(GLsizei width, GLsizei height)
{
	if (height == 0) height = 1;

	glViewport(0, 0, width, height);

	// NETTOYAGE OpenGL 3.3 : Plus de glMatrixMode(GL_PROJECTION) ou de gluPerspective.
	// C'est votre Shader qui utilise la matrice générée par UpdateProjectionMatrix()
	UpdateProjectionMatrix();
}

bool CRenderOpenGL::SetData(Regards::Picture::CPictureArray& bitmap, const bool& deleteOldData)
{
	return textureDisplay->SetData(bitmap, openCLContext.get(), deleteOldData);
}


GLTexture* CRenderOpenGL::GetDisplayTexture(const int& width, const int& height)
{
	return textureDisplay.get();
}

int CRenderOpenGL::GetWidth()
{
	return width;
}

int CRenderOpenGL::GetHeight()
{
	return height;
}

bool CRenderOpenGL::CreateScreenRender(const int& width, const int& height, const CRgbaquad& color)
{
	const bool sizeChanged = (this->width != width || this->height != height);

	if (sizeChanged)
	{
		this->width = width;
		this->height = height;
		ReSizeGLScene(width, height);

		// NETTOYAGE OpenGL 3.3 : Suppression des glMatrixMode/gluOrtho2D obsolètes
	}

	glClearColor(
		color.GetFRed() / 255.0f,
		color.GetFGreen() / 255.0f,
		color.GetFBlue() / 255.0f,
		color.GetFAlpha() / 255.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return sizeChanged;
}

void CRenderOpenGL::RenderQuad(GLTexture* texture, int left, int top, bool inverted)
{
    if (texture == nullptr)
        return;

    RenderQuadInternal(
        static_cast<float>(texture->GetWidth()),
        static_cast<float>(texture->GetHeight()),
        left,
        top,
        inverted,
        false,
        false);

	
}


void CRenderOpenGL::RenderQuad(int width, int height, int left, int top, bool inverted)
{
    if (textureDisplay == nullptr)
        return;

    RenderQuadInternal(
        static_cast<float>(textureDisplay->GetWidth()),
        static_cast<float>(textureDisplay->GetHeight()),
        left,
        top,
        inverted,
        false,
        false);


	
}



void CRenderOpenGL::RenderQuad(GLTexture* texture, const int& width, const int& height, const bool& flipH,
                               const bool& flipV, int left, int top, bool inverted)
{
    if (texture == nullptr)
        return;

    RenderQuadInternal(
        static_cast<float>(texture->GetWidth()),
        static_cast<float>(texture->GetHeight()),
        left,
        top,
        inverted,
        flipH,
        flipV);

	
}


void CRenderOpenGL::RenderQuad(GLTexture* texture, const bool& flipH, const bool& flipV, int left, int top,
                               bool inverted)
{
    if (texture == nullptr)
        return;

    RenderQuadInternal(
        static_cast<float>(texture->GetWidth()),
        static_cast<float>(texture->GetHeight()),
        left,
        top,
        inverted,
        flipH,
        flipV);

	
}

void CRenderOpenGL::RenderToScreen(IMouseUpdate* mousUpdate, CEffectParameter* effectParameter, const int& left,
	const int& top, const bool& inverted)
{

	textureDisplay->Enable();


	// 1. On cherche et on active le shader par défaut pour le Core Profile
	COpenGLShader* defaultShader = FindShader(L"IDR_GLSL_TEXTURE");
	if (defaultShader != nullptr)
	{
		defaultShader->EnableShader(projectionMatrix); // On injecte la matrice de projection

		// 2. On lie la texture à l'uniform "textureScreen" du shader
		defaultShader->m_pShader->SetTexture("textureScreen", textureDisplay->GetTextureID(), 0);
	}

	// 3. On dessine le rectangle
	RenderQuad(textureDisplay.get(), left, top, inverted);

	// 4. On désactive le shader
	if (defaultShader != nullptr)
	{
		defaultShader->DisableShader();
	}
	

	textureDisplay->Disable();
}

GLTexture* CRenderOpenGL::GetGLTexture()
{
	return textureDisplay.get();
}

void CRenderOpenGL::LoadCharacter(unsigned char c, CFreeTypeFace& face)
{
	// Load character glyph 
	if (FT_Load_Char(face.face, c, FT_LOAD_RENDER))
	{
		std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
		return;
	}
	// generate texture

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED,
		face.face->glyph->bitmap.width,
		face.face->glyph->bitmap.rows,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		face.face->glyph->bitmap.buffer
	);
	// set texture options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLTexture* glTexture = new GLTexture(texture, face.face->glyph->bitmap.width, face.face->glyph->bitmap.rows);

	// now store character for later use
	Character character = {
		glTexture,
		vec2d(face.face->glyph->bitmap.width, face.face->glyph->bitmap.rows),
		vec2d(face.face->glyph->bitmap_left, face.face->glyph->bitmap_top),
		static_cast<unsigned int>(face.face->glyph->advance.x)
	};
	Characters.insert(std::pair<char, Character>(c, character));

	widthFont = face.face->glyph->bitmap.width;
	heightFont = face.face->glyph->bitmap.rows;
}

int CRenderOpenGL::LoadFont(const wxString & fontName)
{
    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

	// find path to font
    wxString font_name = CFileUtility::GetResourcesFolderFontPathWithExt(fontName);
    //std::string font_name = FileSystem::getPath("resources/fonts/Antonio-Bold.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }
	
	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 32; c < 127; c++)
        {
			CFreeTypeFace freetypeFace;
			freetypeFace.face = face;
			LoadCharacter(c, freetypeFace);
        }

		// load first 128 characters of ASCII set
		for (unsigned char c = 192; c < 255; c++)
		{
			CFreeTypeFace freetypeFace;
			freetypeFace.face = face;
			LoadCharacter(c, freetypeFace);
		}
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    
	return 0;
}

void CRenderOpenGL::RenderQuad(GLTexture* texture, float left, float top, float scale, bool inverted)
{
   if (texture == nullptr)
        return;

    RenderQuadInternal(
        texture->GetWidth() * scale,
        texture->GetHeight() * scale,
        static_cast<int>(left),
        static_cast<int>(top),
        inverted,
        false,
        false);

	   
}

void CRenderOpenGL::RenderCharacter(GLSLShader* m_pShader, GLTexture* glTexture, const float & left, const float & top, const float & scale, const vec3f & color)
{
	if (m_pShader != nullptr)
	{
		if (!m_pShader->SetTexture("text", glTexture->GetTextureID(),0))
		{
			printf("SetTexture textureScreen failed \n ");
		}
		if (!m_pShader->SetVec3Param("textColor", color))
		{
			printf("SetParam intensity failed \n ");
		}
	}
	RenderQuad(glTexture, left, top, scale, true);
}

void CRenderOpenGL::InitTextBuffers()
{
	if (textVAO != 0) return;

	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glGenBuffers(1, &textEBO);

	glBindVertexArray(textVAO);

	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	// On pré-alloue l'espace pour 4 sommets (1 Quad) de manière fixe
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(TextVertex), nullptr, GL_DYNAMIC_DRAW);

	// Attribut 0 : Position (x, y) -> 2 floats
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)0);

	// Attribut 1 : Coordonnées de texture (u, v) -> 2 floats
	// Le décalage est de 2 * sizeof(float) car les coordonnées viennent après x et y
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), (void*)(2 * sizeof(float)));

	// Remplissage unique et définitif de l'EBO (les indices d'un quad ne changent jamais)
	GLuint indices[6] = { 0, 2, 1, 1, 2, 3 };
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void CRenderOpenGL::RenderText(wxString text, float x, float y, float scale, vec3f color)
{
	if (text.size() <= 0)
		return;

	if (textVAO == 0)
		InitTextBuffers();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	COpenGLShader* m_pShader = FindShader(L"IDR_GLSL_COLOR");
	if (m_pShader == nullptr || !m_pShader->EnableShader(projectionMatrix))
	{
		glDisable(GL_BLEND);
		return;
	}

	m_pShader->m_pShader->SetVec3Param("textColor", color);

	glBindVertexArray(textVAO);

	wxString::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		const Character& ch = Characters[*c];
		if (ch.glTexture == nullptr)
			continue;

		// Liaison de la texture spécifique au caractère
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ch.glTexture->GetTextureID());
		m_pShader->m_pShader->SetIntegerParam("text", 0);

		// Calcul des coordonnées spatiales
		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;

		// Tableau brut sur la pile : aucune allocation dynamique, aucun risque de crash mémoire
		TextVertex vertices[4] = {
			{ xpos,     ypos + h, 0.0f, 0.0f }, // Haut Gauche
			{ xpos + w, ypos + h, 1.0f, 0.0f }, // Haut Droite
			{ xpos,     ypos,     0.0f, 1.0f }, // Bas Gauche
			{ xpos + w, ypos,     1.0f, 1.0f }  // Bas Droite
		};

		// Mise à jour de la mémoire du VBO sur le GPU
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		// Rendu du Quad (les indices sont déjà pré-chargés de manière stable dans l'EBO)
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		x += (ch.Advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_pShader->DisableShader();
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
}