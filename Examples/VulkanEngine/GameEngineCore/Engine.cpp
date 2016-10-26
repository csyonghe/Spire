#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "FreeRoamCameraController.h"
#include "CoreLib/LibIO.h"

#ifndef DWORD
typedef unsigned long DWORD;
#endif
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

namespace GameEngine
{
	Engine * Engine::instance = nullptr;

	using namespace CoreLib;
	using namespace CoreLib::IO;
	using namespace CoreLib::Diagnostics;
	using namespace GraphicsUI;

	void Engine::InternalInit(const EngineInitArguments & args)
	{
		try
		{
			startTime = lastGameLogicTime = lastRenderingTime = Diagnostics::PerformanceCounter::Start();

			GpuId = args.GpuId;

			gameDir = args.GameDirectory;
			engineDir = args.EngineDirectory;

			// initialize input dispatcher
			inputDispatcher = new InputDispatcher(CreateHardwareInputInterface(args.Window));
			auto bindingFile = Path::Combine(gameDir, L"bindings.config");
			if (File::Exists(bindingFile))
				inputDispatcher->LoadMapping(bindingFile);

			// register internal actor classes
			RegisterActorClass(L"StaticMesh", []() {return new StaticMeshActor(); });
			RegisterActorClass(L"SkeletalMesh", []() {return new SkeletalMeshActor(); });
			RegisterActorClass(L"Camera", []() {return new CameraActor(); });
			RegisterActorClass(L"FreeRoamCameraController", []() {return new FreeRoamCameraController(); });

			// initialize renderer
			renderer = CreateRenderer(args.Window, args.API);

			renderer->Resize(args.Width, args.Height);

			uiSystemInterface = new UIWindowsSystemInterface(renderer->GetHardwareRenderer());
			Global::Colors = CreateDarkColorTable();
			uiEntry = new UIEntry(args.Width, args.Height, uiSystemInterface.Ptr());
			uiSystemInterface->SetEntry(uiEntry.Ptr());
			auto cmdForm = new CommandForm(uiEntry.Ptr());
			uiEntry->ShowWindow(cmdForm);

			auto configFile = Path::Combine(gameDir, L"game.config");
			if (File::Exists(configFile))
			{
				CoreLib::Text::Parser parser(File::ReadAllText(configFile));
				if (parser.LookAhead(L"DefaultLevel"))
				{
					parser.ReadToken();
					parser.Read(L"=");
					auto defaultLevelName = parser.ReadStringLiteral();
					LoadLevel(defaultLevelName);
				}
			}
		}
		catch (const Exception & e)
		{
			MessageBox(NULL, e.Message.Buffer(), L"Error", MB_ICONEXCLAMATION);
			exit(1);
		}
	}

	Engine::~Engine()
	{
		renderer->Wait();
		level = nullptr;
		uiEntry = nullptr;
		uiSystemInterface = nullptr;
		renderer = nullptr;
	}

	float Engine::GetTimeDelta(EngineThread thread)
	{
		if (thread == EngineThread::GameLogic)
			return gameLogicTimeDelta;
		else
			return renderingTimeDelta;
	}

	static float aggregateTime = 0.0f;
	static int frameCount = 0;

	void Engine::Tick()
	{
		auto thisGameLogicTime = PerformanceCounter::Start();
		gameLogicTimeDelta = PerformanceCounter::EndSeconds(lastGameLogicTime);

		if (enableInput && !uiEntry->KeyInputConsumed)
			inputDispatcher->DispatchInput();

		if (level)
		{
			for (auto & actor : level->StaticActors)
				actor->Tick();
			for (auto & actor : level->GeneralActors)
				actor->Tick();
		}
		lastGameLogicTime = thisGameLogicTime;

		auto thisRenderingTime = PerformanceCounter::Start();
		renderingTimeDelta = PerformanceCounter::EndSeconds(lastRenderingTime);
		renderer->TakeSnapshot();
		renderer->RenderFrame();
		lastRenderingTime = thisRenderingTime;

		frameCount++;
		aggregateTime += renderingTimeDelta;

		if (aggregateTime > 1.0f)
		{
			printf("                                  \r");
			printf("%.2f ms (%u FPS)\r", aggregateTime/frameCount*1000.0f, (int)(frameCount / aggregateTime));
		
			aggregateTime = 0.0f;
			frameCount = 0;
		}

		auto uiCommands = uiEntry->DrawUI();
		uiSystemInterface->ExecuteDrawCommands(renderer->GetRenderedImage(), uiCommands);
		renderer->GetHardwareRenderer()->Present(uiSystemInterface->GetRenderedImage());
		//renderer->GetHardwareRenderer()->Present(renderer->GetRenderedImage());

	}

	void Engine::Resize(int w, int h)
	{
		if (w > 2 && h > 2)
		{
			renderer->Resize(w, h);
			uiSystemInterface->SetResolution(w, h);
		}
	}

	void Engine::EnableInput(bool value)
	{
		enableInput = value;
	}

	int Engine::HandleWindowsMessage(HWND hwnd, UINT message, WPARAM & wparam, LPARAM & lparam)
	{
		return uiSystemInterface->HandleSystemMessage(hwnd, message, wparam, lparam);
	}

	Actor * Engine::CreateActor(const CoreLib::String & name)
	{
		Func<Actor*> createFunc;
		if (actorClassRegistry.TryGetValue(name, createFunc))
			return createFunc();
		return nullptr;
	}

	void Engine::RegisterActorClass(const String &name, const Func<Actor*>& actorCreator)
	{
		actorClassRegistry[name] = actorCreator;
	}

	void Engine::LoadLevel(const CoreLib::String & fileName)
	{
		if (level)
			renderer->DestroyContext();
		level = nullptr;
		try
		{
			auto actualFileName = FindFile(fileName, ResourceType::Level);
			level = new GameEngine::Level(actualFileName);
			for (auto & actor : level->StaticActors)
				actor->OnLoad();
			for (auto & actor : level->GeneralActors)
				actor->OnLoad();
			renderer->InitializeLevel(level.Ptr());
		}
		catch (const Exception & e)
		{
			Print(L"error loading level '%s': %s\n", fileName.Buffer(), e.Message.Buffer());
		}
	}

	RefPtr<Actor> Engine::ParseActor(GameEngine::Level * pLevel, Text::Parser & parser)
	{
		RefPtr<Actor> actor = CreateActor(parser.NextToken().Str);
		bool isInvalid = false;
		if (actor)
			actor->Parse(pLevel, parser, isInvalid);
		if (!isInvalid)
			return actor;
		else
			return nullptr;
	}

	CoreLib::String Engine::FindFile(const CoreLib::String & fileName, ResourceType type)
	{
		String subDirName;
		switch (type)
		{
		case ResourceType::Level:
			subDirName = L"Levels";
			break;
		case ResourceType::Mesh:
			subDirName = L"Models";
			break;
		case ResourceType::Shader:
			subDirName = L"Shaders";
			break;
		case ResourceType::Texture:
		case ResourceType::Material:
			subDirName = L"Materials";
			break;
		}
		auto localFile = Path::Combine(gameDir, subDirName, fileName);
		if (File::Exists(localFile))
			return localFile;
		auto engineFile = Path::Combine(engineDir, subDirName, fileName);
		if (File::Exists(engineFile))
			return engineFile;
		return CoreLib::String();
	}

	void Engine::Init(const EngineInitArguments & args)
	{
		instance = new Engine();
		instance->InternalInit(args);
	}
	void Engine::Destroy()
	{
		delete instance;
		instance = nullptr;
	}
}


