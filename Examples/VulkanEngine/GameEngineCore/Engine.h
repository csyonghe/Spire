#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "CoreLib/Basic.h"
#include "CoreLib/PerformanceCounter.h"
#include "Level.h"
#include "Renderer.h"
#include "CoreLib/Parser.h"
#include "InputDispatcher.h"
#include "CoreLib/LibUI/LibUI.h"
#include "UISystem_Windows.h"

namespace GameEngine
{
	class EngineInitArguments
	{
	public:
		RenderAPI API;
		WindowHandle Window;
		int Width = 400, Height = 400;
		int GpuId;
		CoreLib::String GameDirectory, EngineDirectory;
	};
	enum class EngineThread
	{
		GameLogic, Rendering
	};
	enum class ResourceType
	{
		Mesh, Shader, Level, Texture, Material
	};
	class Engine
	{
	private:
		static Engine * instance;
	private:
		bool enableInput = true;
		CoreLib::Diagnostics::TimePoint startTime, lastGameLogicTime, lastRenderingTime;
		float gameLogicTimeDelta = 0.0f, renderingTimeDelta = 0.0f;
		CoreLib::String gameDir, engineDir;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::Func<Actor*>> actorClassRegistry;
		CoreLib::RefPtr<Level> level;
		CoreLib::RefPtr<Renderer> renderer;
		CoreLib::RefPtr<InputDispatcher> inputDispatcher;
		
		CoreLib::RefPtr<RenderTargetLayout> uiFrameBufferLayout;
		CoreLib::RefPtr<GraphicsUI::UIEntry> uiEntry;
		CoreLib::RefPtr<GraphicsUI::UIWindowsSystemInterface> uiSystemInterface;

		Engine() {};
		void InternalInit(const EngineInitArguments & args);
		~Engine();
	public:
		float GetTimeDelta(EngineThread thread);
		float GetCurrentTime()
		{
			return CoreLib::Diagnostics::PerformanceCounter::EndSeconds(startTime);
		}
		Level * GetLevel()
		{
			return level.Ptr();
		}
		Renderer * GetRenderer()
		{
			return renderer.Ptr();
		}
		InputDispatcher * GetInputDispatcher()
		{
			return inputDispatcher.Ptr();
		}
	public:
		Actor * CreateActor(const CoreLib::String & name);
		void RegisterActorClass(const CoreLib::String &name, const CoreLib::Func<Actor*> & actorCreator);
		void LoadLevel(const CoreLib::String & fileName);
		CoreLib::RefPtr<Actor> ParseActor(GameEngine::Level * level, CoreLib::Text::Parser & parser);
	public:
		CoreLib::String FindFile(const CoreLib::String & fileName, ResourceType type);
	public:
		void Tick();
		void Resize(int w, int h);
		void EnableInput(bool value);
		int HandleWindowsMessage(HWND hwnd, UINT message, WPARAM &wparam, LPARAM &lparam);
	public:
		int GpuId = 0;
		static Engine * Instance()
		{
			return instance;
		}
		static void Init(const EngineInitArguments & args);
		static void Destroy();
	};

	template<typename ...Args>
	void Print(const wchar_t * message, Args... args)
	{
		wprintf(message, args...);
	}
}
#endif