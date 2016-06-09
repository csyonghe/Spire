#include "glew/glew.h"
#include "glew/wglew.h"
#include <Windows.h>
#include "../WinForm/Debug.h"
#include "OpenGLHardwareRenderer.h"
#include "../Imaging/Bitmap.h"

namespace GL
{
	using CoreLib::Diagnostics::Debug;
	using namespace CoreLib::Basic;

	FrameBuffer FrameBuffer::DefaultFrameBuffer;

	void __stdcall GL_DebugCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum /*severity*/, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			Debug::Write(L"[GL Error] ");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			Debug::Write(L"[GL Performance] ");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			Debug::Write(L"[GL Portability] ");
			break;
		default:
			return;
		}
		Debug::WriteLine(String(message));
		if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		{
			printf("%s\n", message);
 			Debug::WriteLine(L"--------");
		}
	}
	HardwareRenderer * CreateHardwareRenderer()
	{
		return new HardwareRenderer();
	}

	enum CommandName : int
	{
		TERMINATE_SEQUENCE_COMMAND_NV                      = 0x0000,
		NOP_COMMAND_NV                                     = 0x0001,
		DRAW_ELEMENTS_COMMAND_NV                           = 0x0002,
		DRAW_ARRAYS_COMMAND_NV                             = 0x0003,
		DRAW_ELEMENTS_STRIP_COMMAND_NV                     = 0x0004,
		DRAW_ARRAYS_STRIP_COMMAND_NV                       = 0x0005,
		DRAW_ELEMENTS_INSTANCED_COMMAND_NV                 = 0x0006,
		DRAW_ARRAYS_INSTANCED_COMMAND_NV                   = 0x0007,
		ELEMENT_ADDRESS_COMMAND_NV                         = 0x0008,
		ATTRIBUTE_ADDRESS_COMMAND_NV                       = 0x0009,
		UNIFORM_ADDRESS_COMMAND_NV                         = 0x000a,
		BLEND_COLOR_COMMAND_NV                             = 0x000b,
		STENCIL_REF_COMMAND_NV                             = 0x000c,
		LINE_WIDTH_COMMAND_NV                              = 0x000d,
		POLYGON_OFFSET_COMMAND_NV                          = 0x000e,
		ALPHA_REF_COMMAND_NV                               = 0x000f,
		VIEWPORT_COMMAND_NV                                = 0x0010,
		SCISSOR_COMMAND_NV                                 = 0x0011,
		FRONT_FACE_COMMAND_NV                              = 0x0012,
	};

	template<typename T>
	void InsertToken(List<unsigned char> & buffer, const T & cmd)
	{
		buffer.AddRange((unsigned char *)&cmd, sizeof(cmd));
	}

	void CommandBuffer::TerminateSequence()
	{
		TerminateSequenceCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(TERMINATE_SEQUENCE_COMMAND_NV, sizeof(cmd));
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::NoOp()
	{
		NOPCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(NOP_COMMAND_NV, sizeof(cmd));
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::DrawElements(GLuint firstIndex, GLuint baseVertex, GLuint count)
	{
		DrawElementsCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(DRAW_ELEMENTS_COMMAND_NV, sizeof(cmd));
		cmd.baseVertex = baseVertex;
		cmd.count = count;
		cmd.firstIndex = firstIndex;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::DrawArrays(GLuint first, GLuint count)
	{
		DrawArraysCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(DRAW_ARRAYS_COMMAND_NV, sizeof(cmd));
		cmd.count = count;
		cmd.first = first;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::ElementAddress(GLuint64 addr, GLuint typeSizeInByte)
	{
		ElementAddressCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(ELEMENT_ADDRESS_COMMAND_NV, sizeof(cmd));
		cmd.typeSizeInByte = typeSizeInByte;
		cmd.addressHi = addr >> 32;
		cmd.addressLo = addr & 0xFFFFFFFF;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::AttributeAddress(GLuint index, GLuint64 addr)
	{
		AttributeAddressCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(ATTRIBUTE_ADDRESS_COMMAND_NV, sizeof(cmd));
		cmd.index = index;
		cmd.addressHi = addr >> 32;
		cmd.addressLo = addr & 0xFFFFFFFF;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::UniformAddress(GLushort index, ShaderType stage, GLuint64 addr)
	{
		UniformAddressCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(UNIFORM_ADDRESS_COMMAND_NV, sizeof(cmd));
		cmd.index = index;
		switch (stage)
		{
		case ShaderType::VertexShader:
			cmd.stage = glGetStageIndexNV(GL_VERTEX_SHADER);
			break;
		case ShaderType::FragmentShader:
			cmd.stage = glGetStageIndexNV(GL_FRAGMENT_SHADER);
			break;
		default:
			throw NotImplementedException();
		}
		cmd.addressHi = addr >> 32;
		cmd.addressLo = addr & 0xFFFFFFFF;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::PolygonOffset(float scale, float bias)
	{
		PolygonOffsetCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(POLYGON_OFFSET_COMMAND_NV, sizeof(cmd));
		cmd.bias = bias;
		cmd.scale = scale;
		InsertToken(buffer, cmd);

	}
	void CommandBuffer::FrontFace(int frontFace)
	{
		FrontFaceCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(FRONT_FACE_COMMAND_NV, sizeof(cmd));
		cmd.frontFace = frontFace;
		InsertToken(buffer, cmd);
	}
	void CommandBuffer::Viewport(int x, int y, int w, int h)
	{
		ViewportCommandNV cmd;
		cmd.header = glGetCommandHeaderNV(VIEWPORT_COMMAND_NV, sizeof(cmd));
		cmd.x = x;
		cmd.y = y;
		cmd.width = w;
		cmd.height = h;
		InsertToken(buffer, cmd);
	}
	void Texture2D::DebugDump(String fileName)
	{
		List<float> data;
		int w, h;
		GetSize(w, h);
		data.SetSize(w*h * 4);
		GetData(0, DataType::Float4, data.Buffer(), data.Count());
		CoreLib::Imaging::ImageRef img;
		img.Pixels = (Vec4*)data.Buffer();
		img.Width = w;
		img.Height = h;
		img.SaveAsBmpFile(fileName);
	}
}