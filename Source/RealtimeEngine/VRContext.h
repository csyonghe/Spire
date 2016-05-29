#ifndef VR_CONTEXT_H
#define VR_CONTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "LibGL/OpenGLHardwareRenderer.h"

namespace RealtimeEngine
{
	class VREyeView
	{
	public:
		VectorMath::Matrix4 ViewMatrix;
		float FOV;
		int Width, Height;
	};

	class VRHardwareException : public CoreLib::Exception
	{
	public:
		VRHardwareException() : CoreLib::Exception(L"VRException") {}
		VRHardwareException(CoreLib::String message)
			: CoreLib::Exception(message)
		{}
	};

	class IVRContext : public CoreLib::Object
	{
	public:
		virtual void Initialize() = 0;
		virtual VREyeView GetEyeView(int eye) = 0;
		virtual void SetEyeRenderResult(int eye, GL::Texture2D texColor, GL::Texture2D texDepth) = 0;
		virtual void PresentFrame() = 0;
	};

	IVRContext * CreateVRContext();
	void DeleteVRContext(IVRContext * ctx);
}

#endif