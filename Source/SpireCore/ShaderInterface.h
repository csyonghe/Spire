#ifndef SHADER_INTERFACE_H
#define SHADER_INTERFACE_H

class ITexture2D
{
public:
	virtual void GetValue(float * value, float u, float v, float dudx, float dudy, float dvdx, float dvdy) = 0;
};

class ITexture3D
{
public:
	virtual void GetValue(float * value, float u, float v, float w, float lod) = 0;
};


class IShader
{
public:
	virtual void SetUniformInput(const char * name, void * value) = 0;
	virtual void SetInput(const char * name, void * buffer) = 0;
	virtual void SetInputSize(int n) = 0;
	virtual bool GetOutput(const char * name, void * buffer, int & bufferSize) = 0;
	virtual void Run() = 0;
};

#endif