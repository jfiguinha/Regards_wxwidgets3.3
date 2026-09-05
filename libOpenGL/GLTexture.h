#pragma once
#ifdef __WXGTK__
#include <epoxy/gl.h>
#include <epoxy/glx.h>
#include <CL/cl_gl.h>
#endif

#include <PictureArray.h>

class CTextureGLPriv;
class CTextureCudaPriv;

#ifdef __APPLE__

namespace cv
{
	namespace ogl
	{
        class Texture2D;
    }
}

namespace Regards
{
	namespace OpenCL
	{
		class COpenCLContext;
	}

	namespace OpenGL
	{
		// Handle texture related opertions in this class.
		class GLTexture
		{
		public:

			GLTexture(void);
            GLTexture(const int& textureId, const int &width, const int &height);
			~GLTexture(void);

			void SetFilterType(GLint FilterType_i, GLint FilterValue_i);
			void Delete();
			void DeleteInteropTexture();
			void Enable();
			bool SetData(Regards::Picture::CPictureArray& bitmap, Regards::OpenCL::COpenCLContext* openCLContext, const bool& deleteOldData = false);

			void Disable()
			{
				//glDisable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			int GetTextureID()
			{
				return m_nTextureID;
			}

			int GetWidth();
			int GetHeight();


		protected:

			void checkErrors(std::string desc);
			GLuint m_nTextureID;

            bool SetTextureData(Regards::Picture::CPictureArray& bitmap);
			//bool SetTextureDataCPU(Regards::Picture::CPictureArray& bitmap);

			std::unique_ptr<CTextureGLPriv> pimpl_ = nullptr;
			
			//std::unique_ptr<cv::ogl::Texture2D> tex = nullptr;

			int width;
			int height;
			GLenum format;
			bool pboSupported = false;
            GLenum dataformat;
			GLuint pboIds[1];
			
		};
	}
}



#else


namespace cv
{
	namespace ogl
	{
		class Texture2D;
	}
}

namespace Regards
{
	namespace OpenCL
	{
		class COpenCLContext;
	}

	namespace OpenGL
	{
		class GLTexture
		{
		public:

			GLTexture(void);
			GLTexture(const int& textureId, const int& width, const int& height);
			~GLTexture(void);

			void SetFilterType(GLint FilterType_i, GLint FilterValue_i);
			void Delete();
			void DeleteInteropTexture();
			void Enable();
			bool SetData(Regards::Picture::CPictureArray& bitmap, Regards::OpenCL::COpenCLContext* openCLContext, const bool& deleteOldData = false);

			void Disable()
			{
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			// Retourne désormais le bon type conforme à OpenGL (GLuint au lieu de int)
			GLuint GetTextureID()
			{
				return m_nTextureID;
			}

			int GetWidth();
			int GetHeight();


		protected:

			void checkErrors(std::string desc);
			GLuint m_nTextureID;

			bool SetTextureData(Regards::Picture::CPictureArray& bitmap);

			std::unique_ptr<CTextureGLPriv> pimpl_ = nullptr;

			int width;
			int height;
			GLenum format;
			bool pboSupported = false;
			GLenum dataformat;
			GLuint pboIds[1];

		};
	}
}

#endif