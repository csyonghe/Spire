/***********************************************************************

CoreLib - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/
#include "LibGL.h"

/***********************************************************************
NVCOMMANDLIST.CPP
***********************************************************************/
/*-----------------------------------------------------------------------
Copyright (c) 2014, NVIDIA. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Neither the name of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/


PFNGLCREATESTATESNVPROC __nvcCreateStatesNV;
PFNGLDELETESTATESNVPROC __nvcDeleteStatesNV;
PFNGLISSTATENVPROC __nvcIsStateNV;
PFNGLSTATECAPTURENVPROC __nvcStateCaptureNV;
PFNGLDRAWCOMMANDSNVPROC __nvcDrawCommandsNV;
PFNGLDRAWCOMMANDSADDRESSNVPROC __nvcDrawCommandsAddressNV;
PFNGLDRAWCOMMANDSSTATESNVPROC __nvcDrawCommandsStatesNV;
PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC __nvcDrawCommandsStatesAddressNV;
PFNGLCREATECOMMANDLISTSNVPROC __nvcCreateCommandListsNV;
PFNGLDELETECOMMANDLISTSNVPROC __nvcDeleteCommandListsNV;
PFNGLISCOMMANDLISTNVPROC __nvcIsCommandListNV;
PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC __nvcListDrawCommandsStatesClientNV;
PFNGLCOMMANDLISTSEGMENTSNVPROC __nvcCommandListSegmentsNV;
PFNGLCOMPILECOMMANDLISTNVPROC __nvcCompileCommandListNV;
PFNGLCALLCOMMANDLISTNVPROC __nvcCallCommandListNV;
PFNGLGETCOMMANDHEADERNVPROC __nvcGetCommandHeaderNV;
PFNGLGETSTAGEINDEXNVPROC __nvcGetStageIndexNV;


static int initedNVcommandList = 0;

int init_NV_command_list(GetProcFunc fnGetProc)
{
	if (initedNVcommandList) return __nvcCreateStatesNV != ((void*)0);

	__nvcCreateStatesNV = (PFNGLCREATESTATESNVPROC)fnGetProc("glCreateStatesNV");
	__nvcDeleteStatesNV = (PFNGLDELETESTATESNVPROC)fnGetProc("glDeleteStatesNV");
	__nvcIsStateNV = (PFNGLISSTATENVPROC)fnGetProc("glIsStateNV");
	__nvcStateCaptureNV = (PFNGLSTATECAPTURENVPROC)fnGetProc("glStateCaptureNV");
	__nvcDrawCommandsNV = (PFNGLDRAWCOMMANDSNVPROC)fnGetProc("glDrawCommandsNV");
	__nvcDrawCommandsAddressNV = (PFNGLDRAWCOMMANDSADDRESSNVPROC)fnGetProc("glDrawCommandsAddressNV");
	__nvcDrawCommandsStatesNV = (PFNGLDRAWCOMMANDSSTATESNVPROC)fnGetProc("glDrawCommandsStatesNV");
	__nvcDrawCommandsStatesAddressNV = (PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC)fnGetProc("glDrawCommandsStatesAddressNV");
	__nvcCreateCommandListsNV = (PFNGLCREATECOMMANDLISTSNVPROC)fnGetProc("glCreateCommandListsNV");
	__nvcDeleteCommandListsNV = (PFNGLDELETECOMMANDLISTSNVPROC)fnGetProc("glDeleteCommandListsNV");
	__nvcIsCommandListNV = (PFNGLISCOMMANDLISTNVPROC)fnGetProc("glIsCommandListNV");
	__nvcListDrawCommandsStatesClientNV = (PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC)fnGetProc("glListDrawCommandsStatesClientNV");
	__nvcCommandListSegmentsNV = (PFNGLCOMMANDLISTSEGMENTSNVPROC)fnGetProc("glCommandListSegmentsNV");
	__nvcCompileCommandListNV = (PFNGLCOMPILECOMMANDLISTNVPROC)fnGetProc("glCompileCommandListNV");
	__nvcCallCommandListNV = (PFNGLCALLCOMMANDLISTNVPROC)fnGetProc("glCallCommandListNV");
	__nvcGetCommandHeaderNV = (PFNGLGETCOMMANDHEADERNVPROC)fnGetProc("glGetCommandHeaderNV");
	__nvcGetStageIndexNV = (PFNGLGETSTAGEINDEXNVPROC)fnGetProc("glGetStageIndexNV");

	initedNVcommandList = 1;

	return __nvcCreateStatesNV != ((void*)0);
}



/***********************************************************************
OPENGLHARDWARERENDERER.CPP
***********************************************************************/

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

/***********************************************************************
UISYSTEM_WINGL.CPP
***********************************************************************/
#include <wingdi.h>

#pragma comment(lib,"imm32.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif

using namespace CoreLib;
using namespace VectorMath;

namespace GraphicsUI
{
	SHIFTSTATE GetCurrentShiftState()
	{
		SHIFTSTATE Shift = 0;
		if (GetAsyncKeyState(VK_SHIFT))
			Shift = Shift | SS_SHIFT;
		if (GetAsyncKeyState(VK_CONTROL))
			Shift = Shift | SS_CONTROL;
		if (GetAsyncKeyState(VK_MENU))
			Shift = Shift | SS_ALT;
		return Shift;
	}

	void TranslateMouseMessage(UIMouseEventArgs &Data, WPARAM wParam, LPARAM lParam)
	{
		Data.Shift = GetCurrentShiftState();
		bool L, M, R, S, C;
		L = (wParam&MK_LBUTTON) != 0;
		M = (wParam&MK_MBUTTON) != 0;
		R = (wParam&MK_RBUTTON) != 0;
		S = (wParam & MK_SHIFT) != 0;
		C = (wParam & MK_CONTROL) != 0;
		Data.Delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (L)
		{
			Data.Shift = Data.Shift | SS_BUTTONLEFT;
		}
		else {
			if (M) {
				Data.Shift = Data.Shift | SS_BUTTONMIDDLE;
			}
			else {
				if (R) {
					Data.Shift = Data.Shift | SS_BUTTONRIGHT;
				}
			}
		}
		if (S)
			Data.Shift = Data.Shift | SS_SHIFT;
		if (C)
			Data.Shift = Data.Shift | SS_CONTROL;
		Data.X = GET_X_LPARAM(lParam);
		Data.Y = GET_Y_LPARAM(lParam);
	}

	void WinGLSystemInterface::TickTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e)
	{
		entry->DoTick();
	}

	void WinGLSystemInterface::HoverTimerTick(Object *, CoreLib::WinForm::EventArgs e)
	{
		entry->DoMouseHover();
	}

	void WinGLSystemInterface::SetEntry(UIEntry * pEntry)
	{
		this->entry = pEntry;
		tmrHover.Interval = Global::HoverTimeThreshold;
		tmrHover.OnTick.Bind(this, &WinGLSystemInterface::HoverTimerTick);
	}

	int WinGLSystemInterface::HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam)
	{
		int rs = -1;
		unsigned short Key;
		UIMouseEventArgs Data;

		switch (message)
		{
		case WM_CHAR:
		{
			Key = (unsigned short)(DWORD)wParam;
			entry->DoKeyPress(Key, GetCurrentShiftState());
			break;
		}
		case WM_KEYUP:
		{
			Key = (unsigned short)(DWORD)wParam;
			entry->DoKeyUp(Key, GetCurrentShiftState());
			break;
		}
		case WM_KEYDOWN:
		{
			Key = (unsigned short)(DWORD)wParam;
			entry->DoKeyDown(Key, GetCurrentShiftState());
			break;
		}
		case WM_SYSKEYDOWN:
		{
			Key = (unsigned short)(DWORD)wParam;
			if ((lParam&(1 << 29)))
			{
				entry->DoKeyDown(Key, SS_ALT);
			}
			else
				entry->DoKeyDown(Key, 0);
			if (Key != VK_F4)
				rs = 0;
			break;
		}
		case WM_SYSCHAR:
		{
			rs = 0;
			break;
		}
		case WM_SYSKEYUP:
		{
			Key = (unsigned short)(DWORD)wParam;
			if ((lParam & (1 << 29)))
			{
				entry->DoKeyUp(Key, SS_ALT);
			}
			else
				entry->DoKeyUp(Key, 0);
			rs = 0;
			break;
		}
		case WM_MOUSEMOVE:
		{
			tmrHover.StartTimer();
			TranslateMouseMessage(Data, wParam, lParam);
			entry->DoMouseMove(Data.X, Data.Y);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			tmrHover.StartTimer();
			TranslateMouseMessage(Data, wParam, lParam);
			entry->DoMouseDown(Data.X, Data.Y, Data.Shift);
			SetCapture(hWnd);
			break;
		}
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
		{
			tmrHover.StopTimer();
			ReleaseCapture();
			TranslateMouseMessage(Data, wParam, lParam);
			if (message == WM_RBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONRIGHT;
			else if (message == WM_LBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONLEFT;
			else if (message == WM_MBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONMIDDLE;
			entry->DoMouseUp(Data.X, Data.Y, Data.Shift);
			break;
		}
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
			entry->DoDblClick();
		}
		break;
		case WM_MOUSEWHEEL:
		{
			UIMouseEventArgs e;
			TranslateMouseMessage(e, wParam, lParam);
			entry->DoMouseWheel(e.Delta);
		}
		break;
		case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			entry->SetWidth(rect.right - rect.left);
			entry->SetHeight(rect.bottom - rect.top);
		}
		break;
		case WM_PAINT:
		{
			//Draw(0,0);
		}
		break;
		case WM_ERASEBKGND:
		{
		}
		break;
		case WM_NCMBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
		{
			tmrHover.StopTimer();
			entry->DoClosePopup();
		}
		break;
		break;
		case WM_IME_SETCONTEXT:
			lParam = 0;
			break;
		case WM_INPUTLANGCHANGE://改变输入法
		{
			HKL hKL = GetKeyboardLayout(0);
			if (ImmIsIME(hKL))//判断是否使用输入法
			{
				wchar_t inputName[128] = { 0 };
				HIMC hIMC = ImmGetContext(hWnd);
				ImmEscape(hKL, hIMC, IME_ESC_IME_NAME, inputName);
				ImmReleaseContext(hWnd, hIMC);
				entry->ImeMessageHandler.DoImeChangeName(String(inputName));
			}
			else
			{
				entry->ImeMessageHandler.DoImeChangeName(String(L""));
			}
		}
		rs = 0;
		break;
		case WM_IME_NOTIFY://选字列表修改
		{
			rs = 0;
			switch (wParam)
			{
			case IMN_OPENCANDIDATE://打开选字列表
				entry->ImeMessageHandler.DoImeOpenCandidate();
				break;
			case IMN_CHANGECANDIDATE://修改选字列表
				{
					HIMC hIMC = ImmGetContext(hWnd);
					unsigned int listSize = ImmGetCandidateList(hIMC, 0, NULL, 0);
					if (listSize)
					{
						//开辟缓冲区存放选字列表信息
						List<unsigned char> buffer;
						buffer.SetSize(listSize);
						CANDIDATELIST * list = (CANDIDATELIST*)buffer.Buffer();
						ImmGetCandidateList(hIMC, 0, list, listSize);
						int count = 0;
						if (list->dwCount >= list->dwSelection)
						{
							count = Math::Min(list->dwCount - list->dwSelection, list->dwPageSize);
							List<String> candList;
							for (int i = 0; i < count; i++)
							{
								candList.Add(String((wchar_t*)(buffer.Buffer() + list->dwOffset[list->dwSelection + i])));
							}
							entry->ImeMessageHandler.DoImeChangeCandidate(candList);
						}
					}
					ImmReleaseContext(hWnd, hIMC);
				}
				break;
			case IMN_CLOSECANDIDATE://关闭选字列表
				entry->ImeMessageHandler.DoImeCloseCandidate();
				break;
			}
		}
		break;
		case WM_IME_COMPOSITION:
		{
			HIMC hIMC = ImmGetContext(hWnd);
			if (lParam&GCS_COMPSTR)//获得输入栏的文字
			{
				wchar_t EditString[201];
				unsigned int StrSize = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, EditString, sizeof(EditString) - sizeof(char));
				EditString[StrSize / sizeof(wchar_t)] = 0;
				entry->ImeMessageHandler.DoImeCompositeString(String(EditString));
			}
			if (lParam&GCS_RESULTSTR)
			{
				wchar_t ResultStr[201];
				unsigned int StrSize = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, ResultStr, sizeof(ResultStr) - sizeof(TCHAR));
				ResultStr[StrSize / sizeof(wchar_t)] = 0;
				entry->ImeMessageHandler.StringInputed(String(ResultStr));
			}
			ImmReleaseContext(hWnd, hIMC);
			rs = 0;
		}
		break;
		case WM_IME_STARTCOMPOSITION://把WM_IME_STARTCOMPOSITION视为已处理以便消除Windows自己打开的输入框
			entry->ImeMessageHandler.DoImeStart();
			rs = 1;
			break;
		case WM_IME_ENDCOMPOSITION:
			entry->ImeMessageHandler.DoImeEnd();
			rs = 0;
			break;
		}
		return rs;
	}

	class Font
	{
	public:
		CoreLib::String FontName;
		int Size;
		bool Bold, Underline, Italic, StrikeOut;
		Font()
		{
			NONCLIENTMETRICS NonClientMetrics;
			NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(NonClientMetrics.iPaddedBorderWidth);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
			FontName = NonClientMetrics.lfMessageFont.lfFaceName;
			Size = 9;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String& sname, int ssize)
		{
			FontName = sname;
			Size = ssize;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String & sname, int ssize, bool sBold, bool sItalic, bool sUnderline)
		{
			FontName = sname;
			Size = ssize;
			Bold = sBold;
			Underline = sUnderline;
			Italic = sItalic;
			StrikeOut = false;
		}
	};

	class Canvas
	{
	private:
		HFONT hdFont;
		HBRUSH hdBrush;
	public:
		HDC Handle;
		Canvas(HDC DC)
		{
			Handle = DC;
			hdBrush = CreateSolidBrush(RGB(255, 255, 255));
			SelectObject(Handle, hdBrush);
		}
		~Canvas()
		{
			DeleteObject(hdFont);
			DeleteObject(hdBrush);
		}
		void ChangeFont(Font newFont)
		{
			LOGFONT font;
			font.lfCharSet = DEFAULT_CHARSET;
			font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			font.lfEscapement = 0;
			wcscpy_s(font.lfFaceName, 32, newFont.FontName.Buffer());
			font.lfHeight = -MulDiv(newFont.Size, GetDeviceCaps(Handle, LOGPIXELSY), 72);
			font.lfItalic = newFont.Italic;
			font.lfOrientation = 0;
			font.lfOutPrecision = OUT_DEVICE_PRECIS;
			font.lfPitchAndFamily = DEFAULT_PITCH || FF_DONTCARE;
			font.lfQuality = DEFAULT_QUALITY;
			font.lfStrikeOut = newFont.StrikeOut;
			font.lfUnderline = newFont.Underline;
			font.lfWeight = (newFont.Bold ? FW_BOLD : FW_NORMAL);
			font.lfWidth = 0;
			hdFont = CreateFontIndirect(&font);
			HGDIOBJ OldFont = SelectObject(Handle, hdFont);
			DeleteObject(OldFont);
		}
		void DrawText(const CoreLib::String & text, int X, int Y)
		{
			(::TextOut(Handle, X, Y, text.Buffer(), text.Length()));
		}
		int DrawText(const CoreLib::String & text, int X, int Y, int W)
		{
			RECT R;
			R.left = X;
			R.top = Y;
			R.right = X + W;
			R.bottom = 1024;
			return ::DrawText(Handle, text.Buffer(), text.Length(), &R, DT_WORDBREAK | DT_NOPREFIX);
		}
		TextSize GetTextSize(const CoreLib::String& Text)
		{
			SIZE sText;
			TextSize result;
			sText.cx = 0; sText.cy = 0;
			GetTextExtentPoint32(Handle, Text.Buffer(), Text.Length(), &sText);
			result.x = sText.cx;  result.y = sText.cy;
			return result;
		}
		void Clear(int w, int h)
		{
			RECT Rect;
			Rect.bottom = h;
			Rect.right = w;
			Rect.left = 0;  Rect.top = 0;
			FillRect(Handle, &Rect, hdBrush);
		}
	};

	class DIBImage
	{
	private:
		void CreateBMP(int Width, int Height)
		{
			BITMAPINFO bitInfo;
			void * imgptr = NULL;
			bitHandle = NULL;
			bitInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitInfo.bmiHeader.biWidth = Width;
			bitInfo.bmiHeader.biHeight = -Height;
			bitInfo.bmiHeader.biPlanes = 1;
			bitInfo.bmiHeader.biBitCount = 24;
			bitInfo.bmiHeader.biCompression = BI_RGB;
			bitInfo.bmiHeader.biSizeImage = 0;
			bitInfo.bmiHeader.biXPelsPerMeter = 0;
			bitInfo.bmiHeader.biYPelsPerMeter = 0;
			bitInfo.bmiHeader.biClrUsed = 0;
			bitInfo.bmiHeader.biClrImportant = 0;
			bitInfo.bmiColors[0].rgbBlue = 255;
			bitInfo.bmiColors[0].rgbGreen = 255;
			bitInfo.bmiColors[0].rgbRed = 255;
			bitInfo.bmiColors[0].rgbReserved = 255;
			bitHandle = CreateDIBSection(0, &bitInfo, DIB_RGB_COLORS, &imgptr, NULL, 0);
			HGDIOBJ OldBmp = SelectObject(Handle, bitHandle);
			DeleteObject(OldBmp);
			if (ScanLine) delete[] ScanLine;
			if (Height)
				ScanLine = new unsigned char *[Height];
			else
				ScanLine = 0;
			int rowWidth = Width*bitInfo.bmiHeader.biBitCount / 8; //Width*3
			while (rowWidth % 4) rowWidth++;
			for (int i = 0; i < Height; i++)
			{
				ScanLine[i] = (unsigned char *)(imgptr)+rowWidth*i;
			}
			canvas->Clear(Width, Height);
		}
	public:
		HDC Handle;
		HBITMAP bitHandle;
		Canvas * canvas;
		unsigned char** ScanLine;
		DIBImage()
		{
			ScanLine = NULL;
			Handle = CreateCompatibleDC(NULL);
			canvas = new Canvas(Handle);
			canvas->ChangeFont(Font(L"Segoe UI", 10));

		}
		~DIBImage()
		{
			delete canvas;
			if (ScanLine)
				delete[] ScanLine;
			DeleteDC(Handle);
			DeleteObject(bitHandle);
		}

		void SetSize(int Width, int Height)
		{
			CreateBMP(Width, Height);
		}

	};

	class GLUIRenderer
	{
	private:
		GL::HardwareRenderer * glContext;
		int screenWidth, screenHeight;
		Matrix4 orthoMatrix;
		const wchar_t * textureVSSrc = LR"(
				#version 440
				layout(location = 0) in vec2 vert_pos;
				layout(location = 1) in vec2 vert_uv;	
				layout(location = 0) uniform mat4 orthoMatrix;
				layout(location = 1) uniform vec2 translation;

				out vec2 pos;
				out vec2 uv;
				void main()
				{
					pos = vert_pos + translation;
					gl_Position = orthoMatrix * vec4(pos, 0.0, 1.0);
					uv = vert_uv;
				}
			)";
		const char * textureFSSrc = R"(
				#version 440
				layout(location = 2) uniform sampler2D texAlbedo;
				layout(location = 3) uniform vec4 clipBounds;
				
				in vec2 pos;
				in vec2 uv;
				layout(location = 0) out vec4 color;
				void main()
				{
					if (pos.x < clipBounds.x) discard;
					if (pos.y < clipBounds.y) discard;
					if (pos.x > clipBounds.z) discard;
					if (pos.y > clipBounds.w) discard;
 					color = texture(texAlbedo, uv);
				}
			)";
		const char * solidColorFSSrc = R"(
				#version 440
				layout(location = 2) uniform vec4 solidColor;
				layout(location = 3) uniform vec4 clipBounds;
				in vec2 pos;
				layout(location = 0) out vec4 color;
				void main()
				{
					if (pos.x < clipBounds.x) discard;
					if (pos.y < clipBounds.y) discard;
					if (pos.x > clipBounds.z) discard;
					if (pos.y > clipBounds.w) discard;
					color = solidColor;
				}
			)";
		const char * textFSSrc = R"(
				#version 440
				layout(location = 2) uniform sampler2D texAlbedo;
				layout(location = 3) uniform vec4 clipBounds;
				layout(location = 4) uniform vec4 fontColor;
				in vec2 pos;
				in vec2 uv;
				layout(location = 0) out vec4 color;
				void main()
				{
					if (pos.x < clipBounds.x) discard;
					if (pos.y < clipBounds.y) discard;
					if (pos.x > clipBounds.z) discard;
					if (pos.y > clipBounds.w) discard;
					float alpha = texture(texAlbedo, uv).x;
					color = fontColor;
					color.w *= alpha;
				}
			)";
		const char * shadowFsSrc = R"(
			#version 440
			layout(location = 1) uniform vec2 translation;
			layout(location = 2) uniform vec2 origin;
			layout(location = 3) uniform vec2 size;
			layout(location = 4) uniform vec4 shadowColor;
			layout(location = 5) uniform float shadowSize;
			layout(location = 6) uniform vec4 rectBounds;
			layout(location = 7) uniform vec4 clipBounds;
			in vec2 pos;
			layout(location = 0) out vec4 color;
			// This approximates the error function, needed for the gaussian integral
			vec4 erf(vec4 x)
			{
				vec4 s = sign(x), a = abs(x);
				x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
				x *= x;
				return s - s / (x * x);
			}
			// Return the mask for the shadow of a box from lower to upper
			float boxShadow(vec2 lower, vec2 upper, vec2 point, float sigma)
			{
				vec4 query = vec4(point - lower, point - upper);
				vec4 integral = 0.5 + 0.5 * erf(query * (sqrt(0.5) / sigma));
				return (integral.z - integral.x) * (integral.w - integral.y);
			}

			void main()
			{
				if (pos.x > rectBounds.x && pos.x <rectBounds.z && pos.y > rectBounds.y && pos.y < rectBounds.w)
					discard;
				if (pos.x < clipBounds.x) discard;
				if (pos.y < clipBounds.y) discard;
				if (pos.x > clipBounds.z) discard;
				if (pos.y > clipBounds.w) discard;
				float shadow = boxShadow(origin, origin+size, pos - translation, shadowSize);
				color = vec4(shadowColor.xyz, shadowColor.w*shadow);
			}
			)";
	private:
		GL::Shader vs, textureFs, textFs, solidColorFs, shadowFs;
		GL::Program textureProgram, textProgram, solidColorProgram, shadowProgram;
		GL::BufferObject vertexBuffer;
		GL::VertexArray posUvVertexArray, posVertexArray;
		GL::TextureSampler linearSampler;
		Vec2 translation;
		Vec4 clipRect;
	public:
		GLUIRenderer(GL::HardwareRenderer * hw)
		{
			translation = Vec2::Create(0.0f, 0.0f);
			clipRect = Vec4::Create(0.0f, 0.0f, 1e20f, 1e20f);
			glContext = hw;
			vs = glContext->CreateShader(GL::ShaderType::VertexShader, textureVSSrc);
			textureFs = glContext->CreateShader(GL::ShaderType::FragmentShader, textureFSSrc);
			textFs = glContext->CreateShader(GL::ShaderType::FragmentShader, textFSSrc);
			solidColorFs = glContext->CreateShader(GL::ShaderType::FragmentShader, solidColorFSSrc);
			shadowFs = glContext->CreateShader(GL::ShaderType::FragmentShader, shadowFsSrc);
			textureProgram = glContext->CreateProgram(vs, textureFs);
			textureProgram.Link();

			textProgram = glContext->CreateProgram(vs, textFs);
			textProgram.Link();

			solidColorProgram = glContext->CreateProgram(vs, solidColorFs);
			solidColorProgram.Link();

			shadowProgram = glContext->CreateProgram(vs, shadowFs);
			shadowProgram.Link();

			vertexBuffer = glContext->CreateBuffer(GL::BufferUsage::ArrayBuffer);
			vertexBuffer.SetData(nullptr, sizeof(float) * 16);

			posUvVertexArray = glContext->CreateVertexArray();
			posVertexArray = glContext->CreateVertexArray();

			List<GL::VertexAttributeDesc> attribs;
			attribs.Add(GL::VertexAttributeDesc(GL::DataType::Float2, 0, 0, 0));
			posVertexArray.SetVertex(vertexBuffer, attribs.GetArrayView(), 8);

			attribs.Add(GL::VertexAttributeDesc(GL::DataType::Float2, 0, 8, 1));
			posUvVertexArray.SetVertex(vertexBuffer, attribs.GetArrayView(), 16);

			linearSampler = glContext->CreateTextureSampler();
			linearSampler.SetFilter(GL::TextureFilter::Linear);
		}
		~GLUIRenderer()
		{
			glContext->DestroyProgram(textProgram);
			glContext->DestroyProgram(textureProgram);
			glContext->DestroyProgram(solidColorProgram);
			glContext->DestroyShader(vs);
			glContext->DestroyShader(textureFs);
			glContext->DestroyShader(textFs);
			glContext->DestroyShader(solidColorFs);
			glContext->DestroyVertexArray(posUvVertexArray);
			glContext->DestroyVertexArray(posVertexArray);

			glContext->DestroyBuffer(vertexBuffer);
			glContext->DestroyTextureSampler(linearSampler);

		}
		void SetScreenResolution(int w, int h)
		{
			screenWidth = w;
			screenHeight = h;
			Matrix4::CreateOrthoMatrix(orthoMatrix, 0.0f, (float)screenWidth, 0.0f, (float)screenHeight, 1.0f, -1.0f);
		}
		void BeginUIDrawing()
		{
			glContext->SetBlendMode(GL::BlendMode::AlphaBlend);
			glContext->SetZTestMode(GL::BlendOperator::Disabled);
			glContext->SetViewport(0, 0, screenWidth, screenHeight);
		}
		void EndUIDrawing()
		{
			glContext->SetBlendMode(GL::BlendMode::Replace);
			glContext->BindVertexArray(GL::VertexArray());
		}
		void DrawLine(const Vec4 & color, float x0, float y0, float x1, float y1)
		{
			Vec2 points[2];
			points[0] = Vec2::Create(x0 + 0.5f, y0 + 0.5f);
			points[1] = Vec2::Create(x1 + 0.5f, y1 + 0.5f);
			vertexBuffer.SetData(points, sizeof(float) * 4);
			solidColorProgram.Use();
			solidColorProgram.SetUniform(0, orthoMatrix);
			solidColorProgram.SetUniform(1, translation);
			solidColorProgram.SetUniform(2, color);
			solidColorProgram.SetUniform(3, clipRect);
			glContext->BindVertexArray(posVertexArray);
			glContext->DrawArray(GL::PrimitiveType::Lines, 0, 2);
		}
		void DrawSolidPolygon(const Vec4 & color, CoreLib::ArrayView<Vec2> points)
		{
			vertexBuffer.SetData(points.Buffer(), sizeof(float) * 2 * points.Count());
			solidColorProgram.Use();
			solidColorProgram.SetUniform(0, orthoMatrix);
			solidColorProgram.SetUniform(1, translation);
			solidColorProgram.SetUniform(2, color);
			solidColorProgram.SetUniform(3, clipRect);

			glContext->BindVertexArray(posVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, points.Count());
		}

		void DrawSolidQuad(const Vec4 & color, float x, float y, float x1, float y1)
		{
			Vec2 vertexData[4];
			vertexData[0] = Vec2::Create((float)x, (float)y);
			vertexData[1] = Vec2::Create((float)x, (float)y1);
			vertexData[2] = Vec2::Create((float)x1, (float)y1);
			vertexData[3] = Vec2::Create((float)x1, (float)y);
			vertexBuffer.SetData(vertexData, sizeof(float) * 8);
			solidColorProgram.Use();
			solidColorProgram.SetUniform(0, orthoMatrix);
			solidColorProgram.SetUniform(1, translation);
			solidColorProgram.SetUniform(2, color);
			solidColorProgram.SetUniform(3, clipRect);

			glContext->BindVertexArray(posVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, 4);
		}
		void DrawTextureQuad(GL::Texture2D texture, float x, float y, float x1, float y1)
		{
			Vec4 vertexData[4];
			vertexData[0] = Vec4::Create(x, y, 0.0f, 0.0f);
			vertexData[1] = Vec4::Create(x, y1, 0.0f, -1.0f);
			vertexData[2] = Vec4::Create(x1, y1, 1.0f, -1.0f);
			vertexData[3] = Vec4::Create(x1, y, 1.0f, 0.0f);
			vertexBuffer.SetData(vertexData, sizeof(float) * 16);

			textureProgram.Use();
			textureProgram.SetUniform(0, orthoMatrix);
			textureProgram.SetUniform(1, translation);
			textureProgram.SetUniform(2, 0);
			textureProgram.SetUniform(3, clipRect);

			glContext->UseTexture(0, texture, linearSampler);
			glContext->BindVertexArray(posUvVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, 4);
		}
		void DrawTextQuad(GL::Texture2D texture, const Vec4 & fontColor, float x, float y, float x1, float y1)
		{
			Vec4 vertexData[4];
			vertexData[0] = Vec4::Create((float)x, (float)y, 0.0f, 0.0f);
			vertexData[1] = Vec4::Create((float)x, (float)y1, 0.0f, 1.0f);
			vertexData[2] = Vec4::Create((float)x1, (float)y1, 1.0f, 1.0f);
			vertexData[3] = Vec4::Create((float)x1, (float)y, 1.0f, 0.0f);
			vertexBuffer.SetData(vertexData, sizeof(float) * 16);

			textProgram.Use();
			textProgram.SetUniform(0, orthoMatrix);
			textProgram.SetUniform(1, translation);
			textProgram.SetUniform(2, 0);
			textProgram.SetUniform(3, clipRect);
			textProgram.SetUniform(4, fontColor);
			glContext->UseTexture(0, texture, linearSampler);
			glContext->BindVertexArray(posUvVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, 4);
		}
		void DrawRectangleShadow(const Vec4 & color, float x, float y, float w, float h, float offsetX, float offsetY, float shadowSize)
		{
			Vec2 vertexData[4];
			vertexData[0] = Vec2::Create((float)x + offsetX - shadowSize * 1.5f, (float)y + offsetY - shadowSize * 1.5f);
			vertexData[1] = Vec2::Create((float)x + offsetX - shadowSize * 1.5f, (float)(y + h + offsetY) + shadowSize * 3.0f);
			vertexData[2] = Vec2::Create((float)(x + w + offsetX + shadowSize * 3.0f), (float)(y + h + offsetY) + shadowSize * 3.0f);
			vertexData[3] = Vec2::Create((float)(x + w + offsetX + shadowSize * 3.0f), (float)y + offsetY - shadowSize * 1.5f);
			vertexBuffer.SetData(vertexData, sizeof(float) * 8);
			shadowProgram.Use();
			shadowProgram.SetUniform(0, orthoMatrix);
			shadowProgram.SetUniform(1, translation);
			shadowProgram.SetUniform(2, Vec2::Create(x + offsetX, y + offsetY));
			shadowProgram.SetUniform(3, Vec2::Create(w, h));
			shadowProgram.SetUniform(4, color);
			shadowProgram.SetUniform(5, shadowSize * 0.5f);
			shadowProgram.SetUniform(6, Vec4::Create(x, y, x + w, y + h));
			shadowProgram.SetUniform(7, clipRect);
			glContext->BindVertexArray(posVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, 4);
		}
		void SetClipRect(float x, float y, float x1, float y1)
		{
			clipRect.x = x;
			clipRect.y = y;
			clipRect.z = x1;
			clipRect.w = y1;
		}
	};

	class UIImage : public IImage
	{
	public:
		WinGLSystemInterface * context = nullptr;
		GL::Texture2D texture;
		int w, h;
		UIImage(WinGLSystemInterface* ctx, const CoreLib::Imaging::Bitmap & bmp)
		{
			context = ctx;
			texture = context->glContext->CreateTexture2D();
			texture.SetData(bmp.GetIsTransparent() ? GL::StorageFormat::RGBA_I8 : GL::StorageFormat::RGB_I8, bmp.GetWidth(), bmp.GetHeight(), 1,
				bmp.GetIsTransparent() ? GL::DataType::Byte4 : GL::DataType::Byte, bmp.GetPixels());
			w = bmp.GetWidth();
			h = bmp.GetHeight();
		}
		~UIImage()
		{
			context->glContext->DestroyTexture(texture);
		}
		virtual int GetHeight() override
		{
			return h;
		}
		virtual int GetWidth() override
		{
			return w;
		}
	};

	IImage * WinGLSystemInterface::CreateImageObject(const CoreLib::Imaging::Bitmap & bmp)
	{
		return new UIImage(this, bmp);
	}

	Vec4 WinGLSystemInterface::ColorToVec(GraphicsUI::Color c)
	{
		return Vec4::Create(c.R / 255.0f, c.G / 255.0f, c.B / 255.0f, c.A / 255.0f);
	}

	void WinGLSystemInterface::SetClipboardText(const String & text)
	{
		if (OpenClipboard(NULL))
		{
			EmptyClipboard();
			HGLOBAL hBlock = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (text.Length() + 1));
			if (hBlock)
			{
				WCHAR *pwszText = (WCHAR*)GlobalLock(hBlock);
				if (pwszText)
				{
					CopyMemory(pwszText, text.Buffer(), text.Length() * sizeof(WCHAR));
					pwszText[text.Length()] = L'\0';  // Terminate it
					GlobalUnlock(hBlock);
				}
				SetClipboardData(CF_UNICODETEXT, hBlock);
			}
			CloseClipboard();
			if (hBlock)
				GlobalFree(hBlock);
		}
	}

	String WinGLSystemInterface::GetClipboardText()
	{
		String txt;
		if (OpenClipboard(NULL))
		{
			HANDLE handle = GetClipboardData(CF_UNICODETEXT);
			if (handle)
			{
				// Convert the ANSI string to Unicode, then
				// insert to our buffer.
				WCHAR *pwszText = (WCHAR*)GlobalLock(handle);
				if (pwszText)
				{
					// Copy all characters up to null.
					txt = pwszText;
					wchar_t rtn[2] = { 13,0 };
					int fid = txt.IndexOf(String(rtn));
					if (fid != -1)
						txt = txt.SubString(0, fid);
					GlobalUnlock(handle);
				}
			}
			CloseClipboard();
		}
		return txt;
	}

	IFont * WinGLSystemInterface::LoadDefaultFont(DefaultFontType dt)
	{
		switch (dt)
		{
		case DefaultFontType::Content:
			return defaultFont.Ptr();
		case DefaultFontType::Title:
			return titleFont.Ptr();
		default:
			return symbolFont.Ptr();
		}
	}

	void WinGLSystemInterface::SetResolution(int w, int h)
	{
		uiRenderer->SetScreenResolution(w, h);
	}

	void WinGLSystemInterface::ExecuteDrawCommands(CoreLib::List<DrawCommand>& commands)
	{
		uiRenderer->BeginUIDrawing();
		int ptr = 0;
		while (ptr < commands.Count())
		{
			auto & cmd = commands[ptr];
			switch (cmd.Name)
			{
			case DrawCommandName::ClipQuad:
				uiRenderer->SetClipRect(cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::Line:
				uiRenderer->DrawLine(ColorToVec(cmd.SolidColorParams.color), cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::SolidQuad:
				uiRenderer->DrawSolidQuad(ColorToVec(cmd.SolidColorParams.color), cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::TextQuad:
				uiRenderer->DrawTextQuad(((BakedText*)cmd.TextParams.text)->texture, ColorToVec(cmd.TextParams.color), cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::ShadowQuad:
				uiRenderer->DrawRectangleShadow(ColorToVec(cmd.ShadowParams.color), (float)cmd.ShadowParams.x, (float)cmd.ShadowParams.y, (float)cmd.ShadowParams.w,
					(float)cmd.ShadowParams.h, (float)cmd.ShadowParams.offsetX, (float)cmd.ShadowParams.offsetY, cmd.ShadowParams.shadowSize);
				break;
			case DrawCommandName::TextureQuad:
				uiRenderer->DrawTextureQuad(((UIImage*)cmd.TextParams.text)->texture, cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::Triangle:
			{
				Array<Vec2, 3> verts;
				verts.Add(Vec2::Create(cmd.x0, cmd.y0));
				verts.Add(Vec2::Create(cmd.x1, cmd.y1));
				verts.Add(Vec2::Create(cmd.TriangleParams.x2, cmd.TriangleParams.y2));
				uiRenderer->DrawSolidPolygon(ColorToVec(cmd.TriangleParams.color), verts.GetArrayView());
				break;
			}
			case DrawCommandName::Ellipse:
			{
				Array<Vec2, 24> verts;
				int edges = 20;
				float dTheta = Math::Pi * 2.0f / edges;
				float theta = 0.0f;
				float dotX = (cmd.x0 + cmd.x1) * 0.5f;
				float dotY = (cmd.y0 + cmd.y1) * 0.5f;
				float radX = (cmd.x1 - cmd.x0) * 0.5f;
				float radY = (cmd.y1 - cmd.y0) * 0.5f;
				for (int i = 0; i < edges; i++)
				{
					verts.Add(Vec2::Create(dotX + radX * cos(theta), dotY - radY * sin(theta)));
					theta += dTheta;
				}
				uiRenderer->DrawSolidPolygon(ColorToVec(cmd.SolidColorParams.color), verts.GetArrayView());
			}
			}
			ptr++;
		}
		uiRenderer->EndUIDrawing();
	}

	void WinGLSystemInterface::SwitchCursor(CursorType c)
	{
		LPTSTR cursorName;
		switch (c)
		{
		case CursorType::Arrow:
			cursorName = IDC_ARROW;
			break;
		case CursorType::IBeam:
			cursorName = IDC_IBEAM;
			break;
		case CursorType::Cross:
			cursorName = IDC_CROSS;
			break;
		case CursorType::Wait:
			cursorName = IDC_WAIT;
			break;
		case CursorType::SizeAll:
			cursorName = IDC_SIZEALL;
			break;
		case CursorType::SizeNS:
			cursorName = IDC_SIZENS;
			break;
		case CursorType::SizeWE:
			cursorName = IDC_SIZEWE;
			break;
		case CursorType::SizeNWSE:
			cursorName = IDC_SIZENWSE;
			break;
		case CursorType::SizeNESW:
			cursorName = IDC_SIZENESW;
			break;
		default:
			cursorName = IDC_ARROW;
		}
		SetCursor(LoadCursor(0, cursorName));
	}

	WinGLSystemInterface::WinGLSystemInterface(GL::HardwareRenderer * ctx)
	{
		glContext = ctx;
		uiRenderer = new GLUIRenderer(ctx);
		defaultFont = new WinGLFont(ctx, Font(L"Segoe UI", 13));
		titleFont = new WinGLFont(ctx, Font(L"Segoe UI", 13, true, false, false));
		symbolFont = new WinGLFont(ctx, Font(L"Webdings", 13));
		tmrHover.Interval = Global::HoverTimeThreshold;
		tmrHover.OnTick.Bind(this, &WinGLSystemInterface::HoverTimerTick);
		tmrTick.Interval = 50;
		tmrTick.OnTick.Bind(this, &WinGLSystemInterface::TickTimerTick);
		tmrTick.StartTimer();
	}

	WinGLSystemInterface::~WinGLSystemInterface()
	{
		tmrTick.StopTimer();
		delete uiRenderer;
	}

	IFont * WinGLSystemInterface::CreateFontObject(const Font & f)
	{
		return new WinGLFont(glContext, f);
	}

	Rect WinGLFont::MeasureString(const CoreLib::String & text, int /*width*/)
	{
		Rect rs;
		auto size = rasterizer.GetTextSize(text);
		rs.x = rs.y = 0;
		rs.w = size.x;
		rs.h = size.y;
		return rs;
	}

	IBakedText * WinGLFont::BakeString(const CoreLib::String & text, int width)
	{
		BakedText * result = new BakedText();
		auto imageData = rasterizer.RasterizeText(text, width);
		result->glContext = this->glContext;
		result->Width = imageData.Size.x;
		result->Height = imageData.Size.y;
		result->texture = glContext->CreateTexture2D();
		result->texture.SetData(GL::StorageFormat::Int8, result->Width, result->Height, 1, GL::DataType::Byte4, imageData.Image.Buffer());
		return result;
	}

	TextRasterizer::TextRasterizer()
	{
		Bit = new DIBImage();
	}

	TextRasterizer::~TextRasterizer()
	{
		delete Bit;
	}

	void TextRasterizer::SetFont(const Font & Font) // Set the font style of this label
	{
		Bit->canvas->ChangeFont(Font);
	}

	TextRasterizationResult TextRasterizer::RasterizeText(const CoreLib::String & text, int w) // Set the text that is going to be displayed.
	{
		int TextWidth, TextHeight;
		List<unsigned char> pic;
		TextSize size;
		if (w == 0)
		{
			size = Bit->canvas->GetTextSize(text);
			TextWidth = size.x;
			TextHeight = size.y;
			Bit->SetSize(TextWidth, TextHeight);
			Bit->canvas->Clear(TextWidth, TextHeight);
			Bit->canvas->DrawText(text, 0, 0, 10240);
		}
		else
		{
			size.x = w;
			size.y = Bit->canvas->DrawText(text, 0, 0, w);
			TextWidth = size.x;
			TextHeight = size.y;
			Bit->SetSize(TextWidth, TextHeight);
			Bit->canvas->Clear(TextWidth, TextHeight);
			Bit->canvas->DrawText(text, 0, 0, w);
		}
		pic.SetSize(TextWidth*TextHeight * 4);
		int LineWidth = TextWidth;
		for (int i = 0; i < TextHeight; i++)
		{
			for (int j = 0; j < TextWidth; j++)
			{
				auto val = 255 - (Bit->ScanLine[i][j * 3 + 2] + Bit->ScanLine[i][j * 3 + 1] + Bit->ScanLine[i][j * 3]) / 3;
				pic[(i*LineWidth + j) * 4] = pic[(i*LineWidth + j) * 4 + 1] = pic[(i*LineWidth + j) * 4 + 2] = pic[(i*LineWidth + j) * 4 + 3] = (unsigned char)val;
			}
		}
		TextRasterizationResult result;
		result.Image = _Move(pic);
		result.Size.x = TextWidth;
		result.Size.y = TextHeight;
		return result;
	}

	inline TextSize TextRasterizer::GetTextSize(const CoreLib::String & text)
	{
		return Bit->canvas->GetTextSize(text);
	}

}

#ifdef RCVR_UNICODE
#define UNICODE
#endif

