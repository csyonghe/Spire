#include "UISystem_WinGL.h"
#include <Windows.h>
#include <wingdi.h>
#include "../VectorMath.h"
#include "OpenGLHardwareRenderer.h"

#pragma comment(lib,"imm32.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif

using namespace CoreLib;
using namespace VectorMath;

const int TextBufferSize = 4 * 1024 * 1024;
const int TextPixelBits = 2;
const int Log2TextPixelsPerByte = Math::Log2Floor(8 / TextPixelBits);

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

	struct UberVertex
	{
		float x, y, u, v;
		int inputIndex;
	};

	struct TextUniformFields
	{
		int TextWidth, TextHeight;
		int StartPointer;
		int NotUsed;
	};

	struct ShadowUniformFields
	{
		unsigned short ShadowOriginX, ShadowOriginY, ShadowWidth, ShadowHeight;
		int ShadowSize;
		int NotUsed;
	};

	struct UniformField
	{
		unsigned short ClipRectX, ClipRectY, ClipRectX1, ClipRectY1;
		int ShaderType;
		Color InputColor;
		union
		{
			TextUniformFields TextParams;
			ShadowUniformFields ShadowParams;
		};
	};

	class GLUIRenderer
	{
	private:
		GL::HardwareRenderer * glContext;
		int screenWidth, screenHeight;
		Matrix4 orthoMatrix;
		const wchar_t * uberVsSrc = LR"(
				#version 440
				layout(location = 0) in vec2 vert_pos;
				layout(location = 1) in vec2 vert_uv;	
				layout(location = 2) in int vert_primId;
				layout(location = 0) uniform mat4 orthoMatrix;
				out vec2 pos;
				out vec2 uv;
				flat out int primId;
				void main()
				{
					pos = vert_pos;
					gl_Position = orthoMatrix * vec4(pos, 0.0, 1.0);
					uv = vert_uv;
					primId = vert_primId;
				}
			)";
		const char * uberFsSrc = R"(
			#version 440
			layout(binding = 0, std430) buffer uniformBuffer
			{
				uvec4 uniformInput[];
			};
			layout(binding = 1, std430) buffer textBuffer
			{
				uint textContent[];
			};
			in vec2 pos;
			in vec2 uv;
			flat in int primId;
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
				uvec4 params0 = uniformInput[int(primId) * 2];
				uvec4 params1 = uniformInput[int(primId) * 2 + 1];
				int clipBoundX = int(params0.x & 65535);
				int clipBoundY = int(params0.x >> 16);
				int clipBoundX1 = int(params0.y & 65535);
				int clipBoundY1 = int(params0.y >> 16);
				int shaderType = int(params0.z);
				uint pColor = params0.w;
				vec4 inputColor = vec4(float(pColor&255), float((pColor>>8)&255), float((pColor>>16)&255), float((pColor>>24)&255)) * (1.0/255.0);
				if (shaderType == 0) // solid color
				{
					if (pos.x < clipBoundX) discard;
					if (pos.y < clipBoundY) discard;
					if (pos.x > clipBoundX1) discard;
					if (pos.y > clipBoundY1) discard;
					color = inputColor;
				}
				else if (shaderType == 1) // text
				{
					if (pos.x < clipBoundX) discard;
					if (pos.y < clipBoundY) discard;
					if (pos.x > clipBoundX1) discard;
					if (pos.y > clipBoundY1) discard;
					
					int textWidth = int(params1.x);
					int textHeight = int(params1.y);
					int startAddr = int(params1.z);
					
					ivec2 fetchPos = ivec2(uv * ivec2(textWidth, textHeight));
					int relAddr = fetchPos.y * textWidth + fetchPos.x;
					int ptr = startAddr + (relAddr >> 2);
					uint word = textContent[ptr >> 2];
					uint ptrMod = (ptr & 3);
					word >>= (ptrMod<<3);
					int bitPtr = relAddr & 3;
					word >>= (bitPtr << 1);
					float alpha = float(word & 3) * (1.0/3.0);
					color = inputColor;
					color.w *= alpha;
				}
				else if (shaderType == 2)
				{
					if (pos.x > clipBoundX && pos.x < clipBoundX1 && pos.y > clipBoundY && pos.y < clipBoundY1)
						discard;
					vec2 origin, size;
					origin.x = float(params1.x & 65535);
					origin.y = float(params1.x >> 16);
					size.x = float(params1.y & 65535);
					size.y = float(params1.y >> 16);
					float shadowSize = float(params1.z);
					float shadow = boxShadow(origin, origin+size, pos, shadowSize);
					color = vec4(inputColor.xyz, inputColor.w*shadow);
				}
				else
					discard;
			}
		)";
	private:
		GL::Shader uberVs, uberFs;
		GL::Program uberProgram;
		GL::BufferObject vertexBuffer, indexBuffer, uniformBuffer;
		GL::BufferObject shaderBuffers[2];
		GL::VertexArray uberVertexArray;
		GL::TextureSampler linearSampler;
		List<UniformField> uniformFields;
		List<UberVertex> vertexStream;
		List<int> indexStream;
		int primCounter;
		WinGLSystemInterface * system;
		Vec4 clipRect;
	public:
		GLUIRenderer(WinGLSystemInterface * pSystem, GL::HardwareRenderer * hw)
		{
			system = pSystem;
			clipRect = Vec4::Create(0.0f, 0.0f, 1e20f, 1e20f);
			glContext = hw;

			uberVs = glContext->CreateShader(GL::ShaderType::VertexShader, uberVsSrc);
			uberFs = glContext->CreateShader(GL::ShaderType::FragmentShader, uberFsSrc);
			uberProgram = glContext->CreateProgram(uberVs, uberFs);

			uniformBuffer = glContext->CreateBuffer(GL::BufferUsage::ShadeStorageBuffer);

			vertexBuffer = glContext->CreateBuffer(GL::BufferUsage::ArrayBuffer);
			indexBuffer = glContext->CreateBuffer(GL::BufferUsage::ArrayBuffer);

			uberVertexArray = glContext->CreateVertexArray();
			List<GL::VertexAttributeDesc> attribs;
			attribs.Add(GL::VertexAttributeDesc(GL::DataType::Float2, 0, 0, 0));
			attribs.Add(GL::VertexAttributeDesc(GL::DataType::Float2, 0, 8, 1));
			attribs.Add(GL::VertexAttributeDesc(GL::DataType::Int, 0, 16, 2));
			uberVertexArray.SetVertex(vertexBuffer, attribs.GetArrayView(), 20);
			uberVertexArray.SetIndex(indexBuffer);
			linearSampler = glContext->CreateTextureSampler();
			linearSampler.SetFilter(GL::TextureFilter::Linear);
		}
		~GLUIRenderer()
		{
			glContext->DestroyShader(uberVs);
			glContext->DestroyShader(uberFs);
			glContext->DestroyProgram(uberProgram);
			glContext->DestroyBuffer(uniformBuffer);
			glContext->DestroyBuffer(vertexBuffer);
			glContext->DestroyBuffer(indexBuffer);
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
			shaderBuffers[0] = uniformBuffer;
			shaderBuffers[1] = system->GetTextBufferObject();
			vertexStream.Clear();
			uniformFields.Clear();
			indexStream.Clear();
			primCounter = 0;
		}
		void EndUIDrawing()
		{
			vertexBuffer.SetData(vertexStream.Buffer(), sizeof(UberVertex) * vertexStream.Count());
			uniformBuffer.SetData(uniformFields.Buffer(), sizeof(UniformField) * uniformFields.Count());
			indexBuffer.SetData(indexStream.Buffer(), sizeof(int) * indexStream.Count());
			glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
			glContext->SetBlendMode(GL::BlendMode::AlphaBlend);
			glContext->SetZTestMode(GL::BlendOperator::Disabled);
			glContext->SetViewport(0, 0, screenWidth, screenHeight);
			uberProgram.Use();
			uberProgram.SetUniform(0, orthoMatrix);
			glContext->BindShaderBuffers(ArrayView<GL::BufferObject>(shaderBuffers, 2));
			glContext->BindVertexArray(uberVertexArray);
			glEnable(GL_PRIMITIVE_RESTART);
			glPrimitiveRestartIndex((GLuint)(-1));
			glContext->DrawElements(GL::PrimitiveType::TriangleFans, indexStream.Count(), GL::DataType::UInt);
			glContext->SetBlendMode(GL::BlendMode::Replace);
			glContext->BindVertexArray(GL::VertexArray());
		}
		void DrawLine(const Color & color, float x0, float y0, float x1, float y1)
		{
			UberVertex points[4];
			Vec2 p0 = Vec2::Create(x0, y0);
			Vec2 p1 = Vec2::Create(x1, y1);
			Vec2 lineDir = (p1 - p0) * 0.5f;
			lineDir = lineDir.Normalize();
			lineDir = lineDir * 0.5f;
			Vec2 lineDirOtho = Vec2::Create(-lineDir.y, lineDir.x);
			p0.x -= lineDir.x; p0.y -= lineDir.y;
			//p1.x += lineDir.x; p1.y += lineDir.y;
			points[0].x = p0.x - lineDirOtho.x; points[0].y = p0.y - lineDirOtho.y; points[0].inputIndex = primCounter;
			points[1].x = p0.x + lineDirOtho.x; points[1].y = p0.y + lineDirOtho.y; points[1].inputIndex = primCounter;
			points[2].x = p1.x + lineDirOtho.x; points[2].y = p1.y + lineDirOtho.y; points[2].inputIndex = primCounter;
			points[3].x = p1.x - lineDirOtho.x; points[3].y = p1.y - lineDirOtho.y; points[3].inputIndex = primCounter;
			indexStream.Add(vertexStream.Count());
			indexStream.Add(vertexStream.Count() + 1);
			indexStream.Add(vertexStream.Count() + 2);
			indexStream.Add(vertexStream.Count() + 3);
			indexStream.Add(-1);
			vertexStream.AddRange(points, 4);
			UniformField fields;
			fields.ClipRectX = (unsigned short)clipRect.x;
			fields.ClipRectY = (unsigned short)clipRect.y;
			fields.ClipRectX1 = (unsigned short)clipRect.z;
			fields.ClipRectY1 = (unsigned short)clipRect.w;
			fields.ShaderType = 0;
			fields.InputColor = color;
			uniformFields.Add(fields);
			primCounter++;
		}
		void DrawSolidPolygon(const Color & color, CoreLib::ArrayView<Vec2> points)
		{
			for (auto p : points)
			{
				UberVertex vtx;
				vtx.x = p.x; vtx.y = p.y; vtx.inputIndex = primCounter;
				indexStream.Add(vertexStream.Count());
				vertexStream.Add(vtx);
			}
			indexStream.Add(-1);
			UniformField fields;
			fields.ClipRectX = (unsigned short)clipRect.x;
			fields.ClipRectY = (unsigned short)clipRect.y;
			fields.ClipRectX1 = (unsigned short)clipRect.z;
			fields.ClipRectY1 = (unsigned short)clipRect.w;
			fields.ShaderType = 0;
			fields.InputColor = color;
			uniformFields.Add(fields);
			primCounter++;
		}

		void DrawSolidQuad(const Color & color, float x, float y, float x1, float y1)
		{
			indexStream.Add(vertexStream.Count());
			indexStream.Add(vertexStream.Count() + 1);
			indexStream.Add(vertexStream.Count() + 2);
			indexStream.Add(vertexStream.Count() + 3);
			indexStream.Add(-1);

			UberVertex points[4];
			points[0].x = x; points[0].y = y; points[0].inputIndex = primCounter;
			points[1].x = x; points[1].y = y1; points[1].inputIndex = primCounter;
			points[2].x = x1; points[2].y = y1; points[2].inputIndex = primCounter;
			points[3].x = x1; points[3].y = y; points[3].inputIndex = primCounter;
			vertexStream.AddRange(points, 4);

			UniformField fields;
			fields.ClipRectX = (unsigned short)clipRect.x;
			fields.ClipRectY = (unsigned short)clipRect.y;
			fields.ClipRectX1 = (unsigned short)clipRect.z;
			fields.ClipRectY1 = (unsigned short)clipRect.w;
			fields.ShaderType = 0;
			fields.InputColor = color;
			uniformFields.Add(fields);
			primCounter++;
		}
		void DrawTextureQuad(GL::Texture2D /*texture*/, float /*x*/, float /*y*/, float /*x1*/, float /*y1*/)
		{
			/*Vec4 vertexData[4];
			vertexData[0] = Vec4::Create(x, y, 0.0f, 0.0f);
			vertexData[1] = Vec4::Create(x, y1, 0.0f, -1.0f);
			vertexData[2] = Vec4::Create(x1, y1, 1.0f, -1.0f);
			vertexData[3] = Vec4::Create(x1, y, 1.0f, 0.0f);
			vertexBuffer.SetData(vertexData, sizeof(float) * 16);

			textureProgram.Use();
			textureProgram.SetUniform(0, orthoMatrix);
			textureProgram.SetUniform(2, 0);
			textureProgram.SetUniform(3, clipRect);

			glContext->UseTexture(0, texture, linearSampler);
			glContext->BindVertexArray(posUvVertexArray);
			glContext->DrawArray(GL::PrimitiveType::TriangleFans, 0, 4);*/
		}
		void DrawTextQuad(BakedText * text, const Color & fontColor, float x, float y, float x1, float y1)
		{
			indexStream.Add(vertexStream.Count());
			indexStream.Add(vertexStream.Count() + 1);
			indexStream.Add(vertexStream.Count() + 2);
			indexStream.Add(vertexStream.Count() + 3);
			indexStream.Add(-1);

			UberVertex vertexData[4];
			vertexData[0].x = x; vertexData[0].y = y; vertexData[0].u = 0.0f; vertexData[0].v = 0.0f; vertexData[0].inputIndex = primCounter;
			vertexData[1].x = x; vertexData[1].y = y1; vertexData[1].u = 0.0f; vertexData[1].v = 1.0f; vertexData[1].inputIndex = primCounter;
			vertexData[2].x = x1; vertexData[2].y = y1; vertexData[2].u = 1.0f; vertexData[2].v = 1.0f; vertexData[2].inputIndex = primCounter;
			vertexData[3].x = x1; vertexData[3].y = y; vertexData[3].u = 1.0f; vertexData[3].v = 0.0f; vertexData[3].inputIndex = primCounter;
			vertexStream.AddRange(vertexData, 4);

			UniformField fields;
			fields.ClipRectX = (unsigned short)clipRect.x;
			fields.ClipRectY = (unsigned short)clipRect.y;
			fields.ClipRectX1 = (unsigned short)clipRect.z;
			fields.ClipRectY1 = (unsigned short)clipRect.w;
			fields.ShaderType = 1;
			fields.InputColor = fontColor;
			fields.TextParams.TextWidth = text->Width;
			fields.TextParams.TextHeight = text->Height;
			fields.TextParams.StartPointer = system->GetTextBufferRelativeAddress(text->textBuffer);
			uniformFields.Add(fields);
			primCounter++;
		}
		void DrawRectangleShadow(const Color & color, float x, float y, float w, float h, float offsetX, float offsetY, float shadowSize)
		{
			indexStream.Add(vertexStream.Count());
			indexStream.Add(vertexStream.Count() + 1);
			indexStream.Add(vertexStream.Count() + 2);
			indexStream.Add(vertexStream.Count() + 3);
			indexStream.Add(-1);
			
			UberVertex vertexData[4];
			vertexData[0].x = x + offsetX - shadowSize * 1.5f; vertexData[0].y = y + offsetY - shadowSize * 1.5f; vertexData[0].u = 0.0f; vertexData[0].v = 0.0f; vertexData[0].inputIndex = primCounter;
			vertexData[1].x = x + offsetX - shadowSize * 1.5f; vertexData[1].y = (y + h + offsetY) + shadowSize * 1.5f; vertexData[1].u = 0.0f; vertexData[1].v = 1.0f; vertexData[1].inputIndex = primCounter;
			vertexData[2].x = x + w + offsetX + shadowSize * 1.5f; vertexData[2].y = (y + h + offsetY) + shadowSize * 1.5f; vertexData[2].u = 1.0f; vertexData[2].v = 1.0f; vertexData[2].inputIndex = primCounter;
			vertexData[3].x = x + w + offsetX + shadowSize * 1.5f; vertexData[3].y = y + offsetY - shadowSize * 1.5f; vertexData[3].u = 1.0f; vertexData[3].v = 0.0f; vertexData[3].inputIndex = primCounter;
			vertexStream.AddRange(vertexData, 4);

			UniformField fields;
			fields.ClipRectX = (unsigned short)x;
			fields.ClipRectY = (unsigned short)y;
			fields.ClipRectX1 = (unsigned short)(x + w);
			fields.ClipRectY1 = (unsigned short)(y + h);
			fields.ShaderType = 2;
			fields.InputColor = color;
			fields.ShadowParams.ShadowOriginX = (unsigned short)(x + offsetX);
			fields.ShadowParams.ShadowOriginY = (unsigned short)(y + offsetY);
			fields.ShadowParams.ShadowWidth = (unsigned short)(w);
			fields.ShadowParams.ShadowHeight = (unsigned short)(h);
			fields.ShadowParams.ShadowSize = (int)(shadowSize * 0.5f);
			uniformFields.Add(fields);
			primCounter++;
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
				uiRenderer->DrawLine(cmd.SolidColorParams.color, cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::SolidQuad:
				uiRenderer->DrawSolidQuad(cmd.SolidColorParams.color, cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::TextQuad:
				uiRenderer->DrawTextQuad((BakedText*)cmd.TextParams.text, cmd.TextParams.color, cmd.x0, cmd.y0, cmd.x1, cmd.y1);
				break;
			case DrawCommandName::ShadowQuad:
				uiRenderer->DrawRectangleShadow(cmd.ShadowParams.color, (float)cmd.ShadowParams.x, (float)cmd.ShadowParams.y, (float)cmd.ShadowParams.w,
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
				uiRenderer->DrawSolidPolygon(cmd.TriangleParams.color, verts.GetArrayView());
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
				uiRenderer->DrawSolidPolygon(cmd.SolidColorParams.color, verts.GetArrayView());
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
		defaultFont = new WinGLFont(this, Font(L"Segoe UI", 13));
		titleFont = new WinGLFont(this, Font(L"Segoe UI", 13, true, false, false));
		symbolFont = new WinGLFont(this, Font(L"Webdings", 13));
		textBufferObj = ctx->CreateBuffer(GL::BufferUsage::ShadeStorageBuffer);
		textBufferObj.BufferStorage(TextBufferSize, nullptr,
			(GL::BufferStorageFlag)((int)GL::BufferStorageFlag::DynamicStorage | (int)GL::BufferStorageFlag::MapRead | (int)GL::BufferStorageFlag::MapWrite | (int)GL::BufferStorageFlag::MapPersistent));
		textBuffer = (unsigned char*)textBufferObj.Map(GL::BufferAccess::ReadWritePersistent, 0, TextBufferSize);
		textBufferPool.Init(textBuffer, 6, TextBufferSize >> 6);
		uiRenderer = new GLUIRenderer(this, ctx);
		tmrHover.Interval = Global::HoverTimeThreshold;
		tmrHover.OnTick.Bind(this, &WinGLSystemInterface::HoverTimerTick);
		tmrTick.Interval = 50;
		tmrTick.OnTick.Bind(this, &WinGLSystemInterface::TickTimerTick);
		tmrTick.StartTimer();
	}

	WinGLSystemInterface::~WinGLSystemInterface()
	{
		tmrTick.StopTimer();
		textBufferObj.Unmap();
		glContext->DestroyBuffer(textBufferObj);
		delete uiRenderer;
	}

	IFont * WinGLSystemInterface::CreateFontObject(const Font & f)
	{
		return new WinGLFont(this, f);
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
		auto imageData = rasterizer.RasterizeText(system, text, width);
		result->system = system;
		result->Width = imageData.Size.x;
		result->Height = imageData.Size.y;
		result->textBuffer = imageData.ImageData;
		result->BufferSize = imageData.BufferSize;
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

	TextRasterizationResult TextRasterizer::RasterizeText(WinGLSystemInterface * system, const CoreLib::String & text, int w) // Set the text that is going to be displayed.
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

		int pixelCount = (TextWidth * TextHeight);
		int bytes = pixelCount >> Log2TextPixelsPerByte;
		if (pixelCount & ((1 << Log2TextPixelsPerByte) - 1))
			bytes++;
		auto buffer = system->AllocTextBuffer(bytes);
		if (buffer)
		{
			for (int i = 0; i < TextHeight; i++)
			{
				for (int j = 0; j < TextWidth; j++)
				{
					int idx = i * TextWidth + j;
					auto val = 255 - (Bit->ScanLine[i][j * 3 + 2] + Bit->ScanLine[i][j * 3 + 1] + Bit->ScanLine[i][j * 3]) / 3;
					auto packedVal = Math::FastFloor(val / (255.0f / 3.0f) + 0.5f);
					int addr = idx >> Log2TextPixelsPerByte;
					int mod = idx & ((1 << Log2TextPixelsPerByte) - 1);
					int mask = 3 << (mod * TextPixelBits);
					buffer[addr] = (unsigned char)((buffer[addr] & (~mask)) | (packedVal << (mod * TextPixelBits)));
				}
			}
		}
		TextRasterizationResult result;
		result.ImageData = buffer;
		result.Size.x = TextWidth;
		result.Size.y = TextHeight;
		result.BufferSize = bytes;
		return result;
	}

	inline TextSize TextRasterizer::GetTextSize(const CoreLib::String & text)
	{
		return Bit->canvas->GetTextSize(text);
	}

	BakedText::~BakedText()
	{
		if (textBuffer)
			system->FreeTextBuffer(textBuffer, BufferSize);
	}

}

#ifdef RCVR_UNICODE
#define UNICODE
#endif

