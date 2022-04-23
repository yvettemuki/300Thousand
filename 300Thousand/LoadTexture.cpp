#include "LoadTexture.h"
#include "FreeImage.h"


GLuint LoadTexture(const std::string& fname)
{
   FIBITMAP* tempImg = FreeImage_Load(FreeImage_GetFileType(fname.c_str(), 0), fname.c_str());
   FIBITMAP* img = FreeImage_ConvertTo32Bits(tempImg);
   FreeImage_Unload(tempImg);

   return createTexture(img, GL_RGBA, 32);
}

GLfloat* convertFreeImageRGBAFToFloatArray(FIBITMAP* img) {
	GLuint height = FreeImage_GetHeight(img);
	GLuint width = FreeImage_GetWidth(img);
	GLfloat* floatImg = new GLfloat[height * width * 4];

	int currIndex = 0;
	for (int y = 0; y < height; y++) {
		FIRGBAF* bits = (FIRGBAF*)FreeImage_GetScanLine(img, y);
		for (int x = 0; x < width; x++) {
			floatImg[currIndex++] = bits[x].blue;
			floatImg[currIndex++] = bits[x].green;
			floatImg[currIndex++] = bits[x].red;
			floatImg[currIndex++] = bits[x].alpha;
		}
	}
	
	return floatImg;
}

GLuint createTexture(FIBITMAP* img, GLint format, int bits, bool flipOrigin)
{
	GLuint tex_id;

	GLuint w = FreeImage_GetWidth(img);
	GLuint h = FreeImage_GetHeight(img);
	GLuint scanW = FreeImage_GetPitch(img);

	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	if (bits <= 32)
	{
		
		GLubyte* byteImg = new GLubyte[h * scanW];
		FreeImage_ConvertToRawBits(byteImg, img, scanW, bits, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, flipOrigin ? TRUE : FALSE);//true to flip origin
		//FreeImage_Unload(img);

		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, byteImg);
		delete byteImg;
	}
	else if (bits == 128) 
	{
		GLfloat * floatImg = convertFreeImageRGBAFToFloatArray(img);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_BGRA, GL_FLOAT, floatImg);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (flipOrigin) {
		glGenerateMipmap(GL_TEXTURE_2D);
	
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
	}
	else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	

	return tex_id;
}