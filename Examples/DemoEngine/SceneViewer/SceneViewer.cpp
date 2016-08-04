#include "CoreLib/Basic.h"
#include "CoreLib/WinForm.h"
#include "CoreLib/LibGL.h"
#include "Pipeline.h"
#include "CameraControl.h"
#include "ModelResource.h"
#include "ChoiceControl.h"
#include "DemoRecording.h"
#include "ShaderInfoForm.h"
#include "ShaderEditorForm.h"
#include "PerformanceForm.h"

using namespace CoreLib::WinForm;
using namespace CoreLib::Diagnostics;

using namespace GL;
using namespace DemoEngine;

#define EM(x) GraphicsUI::emToPixel(x)

#ifdef CreateDirectory
#undef CreateDirectory
#endif

namespace SceneViewer
{
	class MainForm : public GLForm, IChoiceControl
	{
	private:
		RefPtr<DeviceResourcePool> resourcePool;
		RefPtr<EnginePipeline> scene;
		GraphicsUI::CommandForm * cmdForm = nullptr;
		ChoiceForm * choiceForm = nullptr;
		ShaderInfoForm * shaderInfoForm = nullptr;
		ShaderEditorForm * shaderEditorForm = nullptr;
		PerformanceForm * perfForm = nullptr;
		RefPtr<CameraCurve> curveRecording;
		String currentView = L"color";
		CameraControl camera;
		SystemUniforms sysUniforms;
		MenuItem * renderTargetsMenu, *freezeTimeMenu;
		RefPtr<GraphicsUI::UICommandLineWriter> cmdWriter;
		float time;
		bool foveated = false;
		bool stereo = false;
		bool disableOutput = false;
		float FOV = 75.0f;
		bool advanceTime;
		float nearPlane = 5.0f;
		int superSampleRate;
	public:
		MainForm(String sceneName)
		{
			this->OnClose.Bind(this, &MainForm::Form_Closed);
			SetText("Scene Viewer");
			int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			int screenHeight = GetSystemMetrics(SM_CYSCREEN);
			SetClientWidth(1920);
			SetClientHeight(1080);
			this->SetPosition((screenWidth - GetWidth()) >> 1, (screenHeight - GetHeight()) >> 1, GetWidth(), GetHeight());
			InitUI();
				
			if (!glewIsSupported("GL_ARB_bindless_texture") ||
				!glewIsSupported("GL_NV_vertex_buffer_unified_memory") ||
				!glewIsSupported("GL_NV_uniform_buffer_unified_memory"))
				throw HardwareRendererException(L"The current hardware does not support required OpenGL capabilities.");

			resourcePool = new DeviceResourcePool(glContext.Ptr());
			resourcePool->SetScreenSize(GetClientWidth(), GetClientHeight(), 1);
			readFrameBuffer = resourcePool->CreateFrameBuffer();
			advanceTime = true;
			time = 0.0f;
			superSampleRate = 1;
			wglSwapIntervalEXT(0);
			if (sceneName != L"")
				ExecuteBatchFile(File::ReadAllText(sceneName));

		}
		void FreeRenderingContext()
		{
			scene = nullptr;
			resourcePool = nullptr;
		}
		~MainForm()
		{
			FreeRenderingContext();
		}
		void InitUI()
		{
			GraphicsUI::Global::Colors = GraphicsUI::CreateDarkColorTable();
			auto mainMenu = new MainMenu();

			auto fileMenu = new MenuItem(mainMenu);
			fileMenu->SetText(L"&File");
			auto openMenu = new MenuItem(fileMenu);
			openMenu->SetText(L"&Open...", L"Ctrl+O");
			openMenu->OnClick.Bind(this, &MainForm::OpenMenu_Clicked);
			auto createMeshMenu = new MenuItem(fileMenu);
			createMeshMenu->SetText(L"&Convert Drawable Mesh...");
			createMeshMenu->OnClick.Bind(this, &MainForm::ConvertObjMenu_Clicked);
			auto viewMenu = new MenuItem(mainMenu);
			viewMenu->SetText(L"&View");
			auto commandMenu = new MenuItem(viewMenu);
			commandMenu->SetText(L"&Console");
			commandMenu->SetShortcutText(L"F1");
			commandMenu->OnClick.Bind(this, &MainForm::ConsoleViewMenu_Clicked);
			auto choiceExplorerMenu = new MenuItem(viewMenu);
			choiceExplorerMenu->SetText(L"&Choice Explorer");
			choiceExplorerMenu->OnClick.Bind(this, &MainForm::ChoiceExplorerViewMenu_Clicked);
			choiceExplorerMenu->SetShortcutText(L"F2");
			auto shaderInfoMenu = new MenuItem(viewMenu);
			shaderInfoMenu->SetText(L"Shader &Info");
			shaderInfoMenu->OnClick.Bind(this, &MainForm::ShaderInfoViewMenu_Clicked);
			shaderInfoMenu->SetShortcutText(L"F3");
			auto perfMenu = new MenuItem(viewMenu);
			perfMenu->SetText(L"&Performance");
			perfMenu->SetShortcutText(L"F4");
			perfMenu->OnClick.Bind(this, &MainForm::PerfMenu_Clicked);
			(new MenuItem(viewMenu))->SetType(WinForm::MenuItem::mitSeperator);
			auto toggleFoveatedMenu = new MenuItem(viewMenu);
			toggleFoveatedMenu->SetText(L"Toggle &Foveated Rendering");
			toggleFoveatedMenu->OnClick.Bind(this, &MainForm::ToggleFoveatedRenderingMenu_Clicked);
			auto toggleShadowMap = new MenuItem(viewMenu);
			toggleShadowMap->SetText(L"Toggle &Shadow Map");
			toggleShadowMap->SetShortcutText(L"T");
			toggleShadowMap->OnClick.Bind(this, &MainForm::ToggleShadowMapMenu_Clicked);

			renderTargetsMenu = new MenuItem(mainMenu);
			renderTargetsMenu->SetText(L"&Render Targets");
			freezeTimeMenu = new MenuItem(viewMenu);
			freezeTimeMenu->SetText(L"&Freeze time");
			freezeTimeMenu->SetShortcutText(L"F");
			freezeTimeMenu->OnClick.Bind(this, &MainForm::FreezeTimeMenu_Clicked);

			auto demoMenu = new MenuItem(mainMenu);
			demoMenu->SetText(L"&Demo");

			auto demo1Menu = new MenuItem(demoMenu);
			demo1Menu->SetText(L"Demo &1");
			demo1Menu->OnClick.Bind([this](auto, auto) {LoadDemo1(); });
			auto demo2Menu = new MenuItem(demoMenu);
			demo2Menu->SetText(L"Demo &2");
			demo2Menu->OnClick.Bind([this](auto, auto) {LoadDemo2(); });
			auto demo3Menu = new MenuItem(demoMenu);
			demo3Menu->SetText(L"Demo &3");
			demo3Menu->OnClick.Bind([this](auto, auto) {LoadDemo3(); });

			this->SetMainMenu(mainMenu);
			this->RegisterAccel(Accelerator(0, VK_F1), commandMenu);
			this->RegisterAccel(Accelerator(0, VK_F2), choiceExplorerMenu);
			this->RegisterAccel(Accelerator(0, VK_F3), shaderInfoMenu);
			this->RegisterAccel(Accelerator(0, VK_F4), perfMenu);



			this->RegisterAccel(Accelerator(Accelerator::Ctrl, L'O'), openMenu);
			this->OnResized.Bind(this, &MainForm::Form_Resized);
			uiEntry->OnKeyDown.Bind(this, &MainForm::Form_KeyPressed);
			uiEntry->OnMouseMove.Bind(this, &MainForm::Form_MouseMove);
			uiEntry->OnMouseWheel.Bind(this, &MainForm::Form_MouseWheel);
			uiEntry->OnMouseDown.Bind(this, &MainForm::Form_MouseDown);
			uiEntry->OnMouseUp.Bind(this, &MainForm::Form_MouseUp);

			cmdForm = new GraphicsUI::CommandForm(uiEntry.Ptr());
			cmdForm->OnCommand.Bind(this, &MainForm::OnCommand);
			cmdForm->Left = 10;
			cmdForm->Top = GetClientHeight() - 80;
			uiEntry->CloseWindow(cmdForm);
			cmdWriter = new GraphicsUI::UICommandLineWriter(cmdForm);
			cmdWriter->OnWriteText.Bind(this, &MainForm::OnCommandOutput);
			CoreLib::IO::SetCommandLineWriter(cmdWriter.Ptr());

			CreateChoiceForm();

			shaderEditorForm = new ShaderEditorForm(uiEntry.Ptr(), uiSystemInterface->LoadFont(GraphicsUI::Font(L"Consolas", 13)));
			shaderEditorForm->OnShaderChange.Bind(this, &MainForm::ShaderChanged);
			uiEntry->CloseWindow(shaderEditorForm);

			perfForm = new PerformanceForm(uiEntry.Ptr(), uiSystemInterface.Ptr());
		}

		void OnCommandOutput(const String & /*text*/)
		{
			if (!disableOutput)
				MainLoop(this, EventArgs());
		}

		void LoadDemo1()
		{
			LoadScene(L"Scenes/ocean/ocean.world");
		}

		void LoadDemo2()
		{
			LoadScene(L"Scenes/terrain/terrain.world");
		}

		void LoadDemo3()
		{
			LoadScene(L"Scenes/couch/couch_autotuneDemo.world");
		}

		void UpdateShaderPerf()
		{
			if (scene)
				perfForm->Update((float)MeasurePerformance());
		}

		void ShaderChanged()
		{
			if (scene)
			{
				disableOutput = true;
				bool rs = scene->RecompileShader(shaderEditorForm->shaderName, L"");
				disableOutput = false;

				if (choiceForm)
					choiceForm->Update();
				if (rs)
					shaderEditorForm->pnlStatus->SetText(L"Compilation successful.");
				else
				{
					shaderEditorForm->pnlStatus->SetText(L"Compilation failed. See command prompt for details.");
					uiEntry->ShowWindow(cmdForm);
				}
				UpdateShaderPerf();
			}
		}

		void CreateShaderInfoForm()
		{
			if (!shaderInfoForm)
			{
				shaderInfoForm = new ShaderInfoForm(this->uiEntry.Ptr());
				shaderInfoForm->Posit(10, 10, this->GetClientWidth()-20, 250);
			}
			uiEntry->ShowWindow(shaderInfoForm);
			ChoiceForm_ShaderChanged(currentShader);
		}

		void CreateChoiceForm()
		{
			if (!choiceForm)
			{
				choiceForm = new ChoiceForm(this, this, this->uiEntry.Ptr());
				choiceForm->ShaderChanged.Bind(this, &MainForm::ChoiceForm_ShaderChanged);
				choiceForm->Update();
				choiceForm->Posit(EM(1.0f), EM(1.0f), EM(20.0f), EM(15.0f));
				choiceForm->OnEditShader.Bind(this, &MainForm::ChoiceForm_EditShader);
			}
			uiEntry->ShowWindow(choiceForm);
		}
		void ToggleShadowMapMenu_Clicked(Object *, EventArgs)
		{
			if (scene)
				scene->SetUseShadowMap(!scene->GetUseShadowMap());
		}
		void ToggleFoveatedRenderingMenu_Clicked(Object *, EventArgs)
		{
			foveated = !foveated;
			if (scene)
				scene->SetFoveatedRendering(foveated);
			if (foveated)
				uiprintf(L"Foveated rendering enabled.\n");
			else
				uiprintf(L"Foveated rendering disabled.\n");
		}
		void ConsoleViewMenu_Clicked(Object * , EventArgs )
		{
			uiEntry->ShowWindow(cmdForm);
		}

		void PerfMenu_Clicked(Object *, EventArgs)
		{
			uiEntry->ShowWindow(perfForm);
		}

		void ShaderInfoViewMenu_Clicked(Object *, EventArgs)
		{
			CreateShaderInfoForm();
		}

		void ChoiceExplorerViewMenu_Clicked(Object *, EventArgs)
		{
			CreateChoiceForm();
		}

		String currentShader;

		void ChoiceForm_ShaderChanged(String shaderName)
		{
			currentShader = shaderName;
			if (shaderInfoForm && scene)
			{
				shaderInfoForm->Update(scene->GetShaderMetaData(shaderName));
			}
			if (scene)
			{
				UpdateShaderPerf();
			}
		}

		void ChoiceForm_EditShader(String shaderName)
		{
			if (scene)
			{
				auto fileName = scene->GetShaderFileName(shaderName);
				if (File::Exists(fileName))
				{
					shaderEditorForm->SetShaderFile(shaderName, fileName);
					uiEntry->ShowWindow(shaderEditorForm);
				}
			}
		}

		void ShaderInfoForm_OnClosed(Object *, WindowCloseEventArgs &)
		{
			shaderInfoForm = nullptr;
		}

		float fovRadScale = 0.5f;
		void Form_MouseWheel(GraphicsUI::UI_Base *, GraphicsUI::UIMouseEventArgs & e)
		{
			if (scene && scene->GetFoveatedRendering())
			{
				if (e.Delta > 0)
					fovRadScale *= 1.2f;
				else
					fovRadScale /= 1.2f;
				fovRadScale = Math::Clamp(fovRadScale, 0.05f, 2.0f);
				scene->FoveaRad = (int)((this->GetClientHeight() / 2) * fovRadScale);
			}
		}

		int lastMouseX, lastMouseY;
		bool isMouseDown = false;

		void Form_MouseDown(GraphicsUI::UI_Base *, GraphicsUI::UIMouseEventArgs & e)
		{
			if (scene && e.Shift == GraphicsUI::SS_BUTTONLEFT)
			{
				SetCapture(this->GetHandle());
				lastMouseX = e.X;
				lastMouseY = e.Y;
				isMouseDown = true;
			}
		}

		void Form_MouseUp(GraphicsUI::UI_Base *, GraphicsUI::UIMouseEventArgs & /*e*/)
		{
			isMouseDown = false;
			ReleaseCapture();
		}

		void Form_MouseMove(GraphicsUI::UI_Base *, GraphicsUI::UIMouseEventArgs & e)
		{
			if (scene)
			{
				if (isMouseDown)
				{
					int dx = e.X - lastMouseX;
					int dy = e.Y - lastMouseY;
					camera.Cam.TurnLeft(-dx * 0.005f);
					camera.Cam.TurnUp(-dy * 0.005f);
					lastMouseX = e.X;
					lastMouseY = e.Y;
				}
				else if (scene->GetFoveatedRendering())
				{
					scene->FoveaX = e.X;
					scene->FoveaY = this->GetClientHeight() - e.Y;
					scene->FoveaRad = (int)((this->GetClientHeight() / 2) * fovRadScale);
				}
			}
		}

		void PlayRecord(float totalTime)
		{
			if (!curveRecording)
				return;
			auto timePoint = PerformanceCounter::Start();
			float vTime = 0.0f;
			while (vTime < totalTime)
			{
				vTime = (float)PerformanceCounter::ToSeconds(PerformanceCounter::End(timePoint));
				if (vTime > totalTime)
					break;
				camera.Cam = curveRecording->ExtractCamera(vTime / totalTime * curveRecording->TotalDistance);
				MainLoop(nullptr, EventArgs());
				Application::DoEvents();
			}
		}
		GL::FrameBuffer readFrameBuffer;
		void SaveCurrentViewToFile(String fileName, bool isPng)
		{
			auto tex = resourcePool->GetTexture(currentView);
			CoreLib::Imaging::ImageRef imgRef;
			tex.GetSize(imgRef.Width, imgRef.Height);
			List<float> imageBuffer;
			imageBuffer.SetSize(imgRef.Width * imgRef.Height * 4);
			auto hw = resourcePool->GetHardwareRenderer();
			readFrameBuffer.SetColorRenderTarget(0, tex);
			hw->SetReadFrameBuffer(readFrameBuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, imgRef.Width, imgRef.Height);
			glReadPixels(0, 0, imgRef.Width, imgRef.Height, GL_RGBA, GL_FLOAT, imageBuffer.Buffer());
			hw->SetReadFrameBuffer(GL::FrameBuffer());
			imgRef.Pixels = (Vec4*)imageBuffer.Buffer();
			if (isPng)
				imgRef.SaveAsPngFile(fileName, true);
			else
				imgRef.SaveAsBmpFile(fileName, true);
		}

		void RenderRecord(int frames, String fileName)
		{
			if (!curveRecording)
				return;
			auto timePoint = PerformanceCounter::Start();
			for (int i = 0; i < frames; i++)
			{
				float t = (float)i/(float)frames;
				this->time = t * 10.0f;
				camera.Cam = curveRecording->ExtractCamera(t * curveRecording->TotalDistance);
				MainLoop(nullptr, EventArgs());
				Path::CreateDirectory(fileName);
				SaveCurrentViewToFile(Path::Combine(fileName, String(i) + L".bmp"), false);
				Application::DoEvents();
			}
		}

		void ExecuteBatchFile(String content)
		{
			auto lines = Split(content, L'\n');
			for (auto & line : lines)
				if (!line.StartsWith(L"//"))
					OnCommand(line);
		}

		void OnCommand(String command)
		{
			CoreLib::Text::Parser parser(command);
			try
			{
				if (parser.LookAhead(L"load"))
				{
					parser.ReadToken();
					LoadScene(parser.ReadStringLiteral());

				}
				else if (parser.LookAhead("fov"))
				{
					parser.ReadToken();
					FOV = (float)parser.ReadDouble();
				}
				else if (parser.LookAhead(L"stereo"))
				{
					parser.ReadToken();
					stereo = !stereo;
				}
				else if (parser.LookAhead(L"storecam"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					IO::BinaryWriter writer(new IO::FileStream(fileName, IO::FileMode::Create));
					writer.Write(&camera.Cam, 1);
				}
				else if (parser.LookAhead(L"loadcam"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					IO::BinaryReader reader(new IO::FileStream(fileName, IO::FileMode::Open));
					reader.Read(&camera.Cam, 1);
					UpdateShaderPerf();
				}
				else if (parser.LookAhead(L"newrec"))
				{
					parser.ReadToken();
					curveRecording = new CameraCurve();
				}
				else if (parser.LookAhead(L"rec"))
				{
					parser.ReadToken();
					if (curveRecording)
						curveRecording->AddPoint(camera.Cam.pos, camera.Cam.pos + camera.Cam.dir);
				}
				else if (parser.LookAhead(L"stat"))
				{
					parser.ReadToken();
					if (scene)
					{
						uiprintf(L"Command lists submitted: %d\n", scene->CommandListsDrawn);
						scene->PrintStats();
					}
				}
				else if (parser.LookAhead(L"delrec"))
				{
					parser.ReadToken();
					if (curveRecording)
						curveRecording->RemoveLastPoint();
				}
				else if (parser.LookAhead(L"saverec"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					if (curveRecording && curveRecording->Points.Count() >= 2)
					{
						curveRecording->Save(fileName);
						curveRecording->Initialize();
						uiprintf(L"Camera curve saved. %d control points, total distance %f\n", curveRecording->Points.Count(), curveRecording->TotalDistance);
					}
					else
						MessageBox(L"At least two camera points should be specified.", L"Error", MB_ICONEXCLAMATION);
				}
				else if (parser.LookAhead(L"loadrec"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					curveRecording = new CameraCurve();
					curveRecording->Load(fileName);
					curveRecording->Initialize();
				}
				else if (parser.LookAhead(L"playrec"))
				{
					parser.ReadToken();
					auto totalTime = (float)parser.ReadDouble();
					PlayRecord(totalTime);
				}
				else if (parser.LookAhead(L"renderrec"))
				{
					parser.ReadToken();
					auto frames = parser.ReadInt();
					auto fileName = parser.ReadStringLiteral();
					RenderRecord(frames, fileName);
				}
				else if (parser.LookAhead(L"save"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					SaveFrame(fileName);
				}
				else if (parser.LookAhead(L"res"))
				{
					parser.ReadToken();
					auto w = parser.ReadInt();
					auto h = parser.ReadInt();
					auto s = 1;
					if (parser.NextToken().TypeID == TokenType_Int)
						s = parser.ReadInt();
					superSampleRate = Math::Clamp(s, 1, 4);
					SetClientWidth(w);
					SetClientHeight(h);
				}
				else if (parser.LookAhead(L"perf"))
				{
					parser.ReadToken();
					MeasurePerformance();
				}
				else if (parser.LookAhead(L"view"))
				{
					parser.ReadToken();
					currentView = parser.ReadStringLiteral();
				}
				else if (parser.LookAhead(L"save"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					SaveFrame(fileName);
				}
				else if (parser.LookAhead(L"listshaders"))
				{
					parser.ReadToken();
					ListShaders();
				}
				else if (parser.LookAhead(L"choices"))
				{
					parser.ReadToken();
					auto id = parser.ReadInt();
					ListChoices(id);
				}
				else if (parser.LookAhead(L"recompile"))
				{
					parser.ReadToken();
					auto id = parser.ReadInt();
					auto schedule = parser.ReadStringLiteral();
					auto shaders = scene->GetShaders();
					if (id >= 0 && id < shaders.Count())
						scene->RecompileShader(shaders[id], schedule);
				}
				else if (parser.LookAhead(L"lightdir"))
				{
					parser.ReadToken();
					Vec3 lightDir;
					lightDir.x = (float)parser.ReadDouble();
					lightDir.y = (float)parser.ReadDouble();
					lightDir.z = (float)parser.ReadDouble();
					lightDir = lightDir.Normalize();
					sysUniforms.LightDir = lightDir;
				}
				else if (parser.LookAhead(L"lightcolor"))
				{
					parser.ReadToken();
					Vec3 color;
					color.x = (float)parser.ReadDouble();
					color.y = (float)parser.ReadDouble();
					color.z = (float)parser.ReadDouble();
					sysUniforms.LightColor = color;
				}
				else if (parser.LookAhead(L"sellod"))
				{
					parser.ReadToken();
					int lod = parser.ReadInt();
					if (scene)
						scene->SelectedLOD = lod;
				}
				else if (parser.LookAhead(L"heightmap"))
				{
					parser.ReadToken();
					auto fileName = parser.ReadStringLiteral();
					int w = parser.ReadInt();
					int h = parser.ReadInt();
					BinaryReader reader(new FileStream(fileName));
					CoreLib::Imaging::ImageRef img;
					List<Vec4> pixels;
					pixels.SetSize(w * h);
					for (int i = 0; i < w*h; i++)
					{
						pixels[i] = Vec4::Create(((unsigned short)reader.ReadInt16()) / 65535.0f);
						pixels[i].w = 1.0f;
					}
					img.Width = w;
					img.Height = h;
					img.Pixels = pixels.Buffer();
					img.SaveAsPngFile(Path::ReplaceExt(fileName, L"png"));
					img.SaveAsPfmFile(Path::ReplaceExt(fileName, L"pfm"));
				}
				else if (parser.LookAhead(L"combineimg"))
				{
					parser.ReadToken();
					auto rs = parser.ReadStringLiteral();
					auto r = parser.ReadStringLiteral();
					auto g = parser.ReadStringLiteral();
					auto b = parser.ReadStringLiteral();
					auto a = parser.ReadStringLiteral();
					CoreLib::Imaging::Bitmap br(r);
					CoreLib::Imaging::Bitmap bg(g);
					CoreLib::Imaging::Bitmap bb(b);
					CoreLib::Imaging::Bitmap ba(a);
					auto cmb = CoreLib::Imaging::CombineImageFromChannels(&br, &bg, &bb, &ba);
					cmb.GetImageRef().SaveAsBmpFile(rs);
				}
				else if (parser.LookAhead(L"foveated"))
				{
					ToggleFoveatedRenderingMenu_Clicked(nullptr, EventArgs());
				}
			}
			catch (Exception)
			{
				printf("Cannot parse command.\n");
			}
		}
		void ListShaders()
		{
			if (scene)
			{
				int id = 0;
				printf("Shaders:\n");
				for (auto & shader : scene->GetShaders())
				{
					printf("%d: %s\n", id++, shader.ToMultiByteString());
				}
			}
		}
		void ListChoices(int id)
		{
			if (scene)
			{
				printf("Choices of shader %d:\n", id);
				auto shaders = scene->GetShaders();
				EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> existingChoices;
				for (auto & choice : scene->GetChoices(shaders[id], existingChoices))
				{
					printf("%s = {", choice.ChoiceName.ToMultiByteString());
					for (auto & opt : choice.Options)
					{
						printf("%s; ", opt.ToString().ToMultiByteString());
					}
					printf("}\n");
				}
			}
		}
		void SaveFrame(String fileName)
		{
			time = 0.0f;
			MainLoop(nullptr, EventArgs());
			SaveCurrentViewToFile(fileName, fileName.ToLower().EndsWith(L"png"));
		}
			
		void Form_KeyPressed(GraphicsUI::UI_Base*, GraphicsUI::UIKeyEventArgs&)
		{
			if (scene)
			{
				if (GetAsyncKeyState('F') || GetAsyncKeyState(L'f'))
				{
					FreezeTimeMenu_Clicked(freezeTimeMenu, EventArgs());
				}
				if (GetAsyncKeyState('R') || GetAsyncKeyState(L'r'))
				{
					time = 0.0f;
				}
				if (GetAsyncKeyState('P') || GetAsyncKeyState(L'p'))
				{
					PrintPerformance();
				}
				if (GetAsyncKeyState('T') || GetAsyncKeyState(L't'))
				{
					ToggleShadowMapMenu_Clicked(this, EventArgs());
				}
			}
		}
		void PrintPerformance()
		{

		}
		float GetCameraSpeed()
		{
			return 0.0f;
		}
			
		void LoadScene(String fileName)
		{
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			scene = nullptr;
			RefPtr<EnginePipeline> newScene = new EnginePipeline();
			newScene->Initialize(resourcePool.Ptr());
			newScene->LoadFromFile(fileName);
			newScene->SetFoveatedRendering(foveated);
			scene = newScene;
			camera.Reset();
			camera.MaxSpeed = 100.0f;
			sysUniforms.LightColor = Vec3::Create(1.0f);
			sysUniforms.LightDir = Vec3::Create(0.0f, 1.0f, -0.2f).Normalize();
			if (choiceForm)
				choiceForm->Update();
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			InitViews();
			OnCommand(L"loadcam " + CoreLib::Text::Parser::EscapeStringLiteral(Path::Combine(Path::GetDirectoryName(fileName), L"cam0.cam")));
		}
		void InitViews()
		{
			auto finalColorMenu = new MenuItem(renderTargetsMenu);
			finalColorMenu->SetText(L"color");
			finalColorMenu->OnClick.Bind(this, &MainForm::RenderTargetItemMenu_Clicked);
		}
		void FreezeTimeMenu_Clicked(Object * sender, EventArgs e)
		{
			advanceTime = !advanceTime;
			((MenuItem*)sender)->SetChecked(!advanceTime);
		}
		void ConvertObjMenu_Clicked(Object *, EventArgs e)
		{
			FileDialog dlgOpen(this);
			dlgOpen.Filter = L"Supported files|*.ase;*.obj";
			if (dlgOpen.ShowOpen())
			{
				if (dlgOpen.FileName.ToLower().EndsWith(L"ase"))
					ModelResource::ConvertAseToMeshes(dlgOpen.FileName);
				else
					ModelResource::ConvertObjToMeshes(dlgOpen.FileName);
			}
		}
		void OpenMenu_Clicked(Object *, EventArgs e)
		{
			FileDialog dlgOpen(this);
			dlgOpen.Filter = L"World|*.world|All Files|*.*";
			if (dlgOpen.ShowOpen())
			{
				try
				{
					LoadScene(dlgOpen.FileName);
				}
				catch (Exception & e)
				{
					scene = nullptr;
					MessageBox(e.Message, L"Error", MB_ICONEXCLAMATION);
				}
			}
		}
		void Form_Closed(Object *, WindowCloseEventArgs &)
		{
			FreeRenderingContext();
		}
		void Form_Resized(Object *, EventArgs)
		{
			if (resourcePool)
			{
				resourcePool->SetScreenSize(GetClientWidth()*superSampleRate, GetClientHeight()*superSampleRate, 1);
				resourcePool->GetHardwareRenderer()->SetViewport(0, 0, GetClientWidth(), GetClientHeight());
				if (scene)
					scene->FrameResized();
			}
		}
		double MeasurePerformance()
		{
			double rs = 10000.0f;
			for (int p = 0; p < 25; p++)
			{
				RenderFrame();
				glFinish();
			}
			for (int p = 0; p < 10; p++)
			{
				LARGE_INTEGER start, end, freq;
				glFinish();
				QueryPerformanceCounter(&start);
				for (int i = 0; i < 10; i++)
					RenderFrame();
				glFinish();
				QueryPerformanceCounter(&end);
				QueryPerformanceFrequency(&freq);
				double vtime = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart * 100.0;
				rs = Math::Min(rs, vtime);
			}
			return rs;
		}
		void RenderTargetItemMenu_Clicked(Object * sender, EventArgs e)
		{
			currentView = ((MenuItem*)sender)->GetText();
		}
		void FillPerspectiveInfo(ViewFrustum & view)
		{
			view.Aspect = GetClientWidth() / (float)GetClientHeight();
			view.FOV = FOV;
			view.zMin = nearPlane;
			view.zMax = 40000.0f;
		}
		void UpdateWindow() override
		{
			glContext->SwapBuffers();
		}
		GL::Texture2D RenderFrame() override
		{
			camera.Cam.pos.y = Math::Max(camera.Cam.pos.y, scene->GetAltitude(camera.Cam.pos) + nearPlane*2.0f);
			camera.Cam.GetView(sysUniforms.Views[0]);
			FillPerspectiveInfo(sysUniforms.Views[0]);
			auto cam = camera; 
			cam.Cam.MoveLeft(10.0f);
			cam.Cam.GetView(sysUniforms.Views[1]);
			sysUniforms.Time = time;
			FillPerspectiveInfo(sysUniforms.Views[1]);

			scene->UpdateSysUniform(sysUniforms);
			scene->DrawObjSpace();
			scene->Draw(stereo);
			return scene->GetColorBuffer();
		}
		void MainLoop(Object *, EventArgs e)
		{
			static float frameRenderTime = 0.0f;
			static float dtime = 0.0f;
			static auto timePoint = PerformanceCounter::Start();
			dtime = (float)PerformanceCounter::ToSeconds(PerformanceCounter::End(timePoint));
			timePoint = PerformanceCounter::Start();
			if (Focused() && uiEntry->FocusedControl == nullptr)
			{
				camera.HandleKeys(dtime);
			}
			if (glContext)
			{
				if (scene)
				{
					RenderFrame();
				}
				else
				{
					glContext->SetClearColor(Vec4::Create(0.5f, 0.75f, 1.0f, 1.0f));
					glContext->Clear(false, true, false);
				}
				this->DrawUIOverlay();
				glContext->SwapBuffers();
			}
			if (advanceTime)
			{
				time += dtime;
			}
		}

		// Inherited via IChoiceControl
		virtual List<String> GetShaders() override
		{
			if (scene)
				return scene->GetShaders();
			return List<String>();
		}
		virtual List<Spire::Compiler::ShaderChoice> GetChoices(String shader, const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>& existingChoices) override
		{
			if (scene)
				return scene->GetChoices(shader, existingChoices);
			return List<Spire::Compiler::ShaderChoice>();
		}
		virtual void RecompileShader(String shader, const String & schedule) override
		{
			disableOutput = true;

			if (scene)
				scene->RecompileShader(shader, schedule);
			disableOutput = false;
		}

		virtual EnumerableDictionary<String, GL::Texture2D> GetPrecomputedTextures(String shader) override
		{
			if (scene)
			{
				return scene->GetPrecomputedTextures(shader);
			}
			return EnumerableDictionary<String, GL::Texture2D>();
		}
	};
}
//
//int CALLBACK wWinMain(
//	_In_ HINSTANCE hInstance,
//	_In_ HINSTANCE hPrevInstance,
//	_In_ LPWSTR     lpCmdLine,
//	_In_ int       nCmdShow
//)
//{
//	Application::Init(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
int main()
{
	Application::Init();
	try
	{
		String sceneName;
		CommandLineParser cmdParser(GetCommandLine());
		sceneName = cmdParser.GetFileName(true);
		
		auto form = new SceneViewer::MainForm(L"");
		if (sceneName.Length())
		{
			form->Show();
			form->ExecuteBatchFile(File::ReadAllText(sceneName));
			form->Close();
		}
		else
		{
			Application::SetMainLoopEventHandler(new NotifyEvent(form, &SceneViewer::MainForm::MainLoop));
			Application::Run(form, true);
		}
	}
	catch (const HardwareRendererException & e)
	{
		MessageBoxW(0, e.Message.Buffer(), L"Error", MB_ICONEXCLAMATION);
	}
	Application::Dispose();
	return 0;
}
