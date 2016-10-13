#include "InputDispatcher.h"
#include "CoreLib/Parser.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::Text;
	using namespace CoreLib::IO;

	InputDispatcher::InputDispatcher(const RefPtr<HardwareInputInterface>& pInputInterface)
	{
		inputInterface = pInputInterface;
	}

	void InputDispatcher::LoadMapping(const CoreLib::String & fileName)
	{
		Parser parser(File::ReadAllText(fileName));
		while (!parser.IsEnd())
		{
			bool isAxis = true;
			InputMappingValue mapVal;
			if (parser.LookAhead(L"action"))
			{
				parser.Read(L"action");
				isAxis = false;
			}
			else
				parser.Read(L"axis");
			mapVal.ActionName = parser.ReadWord();
			parser.Read(L":");
			auto ch = parser.ReadStringLiteral().ToUpper();
			if (isAxis)
			{
				if (parser.LookAhead(L","))
				{
					parser.Read(L",");
					mapVal.Value = (float)parser.ReadDouble();
				}
			}
			wchar_t key = ch[0];
			if (ch == L"SPACE")
				key = SpecialKeys::Space;
			else if (ch == L"MBLEFT")
				key = SpecialKeys::MouseLeftButton;
			else if (ch == L"MBRIGHT")
				key = SpecialKeys::MouseRightButton;
			else if (ch == L"MBMIDDLE")
				key = SpecialKeys::MouseMiddleButton;
			else if (ch == L"CTRL")
				key = SpecialKeys::Control;
			else if (ch == L"UP")
				key = SpecialKeys::UpArrow;
			else if (ch == L"DOWN")
				key = SpecialKeys::DownArrow;
			else if (ch == L"LEFT")
				key = SpecialKeys::LeftArrow;
			else if (ch == L"RIGHT")
				key = SpecialKeys::RightArrow;
			else if (ch == L"ESCAPE")
				key = SpecialKeys::Escape;
			else if (ch == L"ENTER")
				key = SpecialKeys::Enter;
			else if (ch == L"SHIFT")
				key = SpecialKeys::Shift;
			if (isAxis)
				keyboardAxisMappings[key] = mapVal;
			else
				keyboardActionMappings[key] = mapVal;
			actionHandlers[mapVal.ActionName] = List<ActionInputHandlerFunc>();
		}
	}
	void InputDispatcher::BindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc handlerFunc)
	{
		auto list = actionHandlers.TryGetValue(axisName);
		if (list)
			list->Add(handlerFunc);
	}
	void InputDispatcher::UnbindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc func)
	{
		auto list = actionHandlers.TryGetValue(axisName);
		if (list)
		{
			for (int i = list->Count() - 1; i >= 0; i--)
			{
				if ((*list)[i] == func)
				{
					list->RemoveAt(i);
					break;
				}
			}
		}
	}
	void InputDispatcher::BindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc)
	{
		mouseActionHandlers.Add(handlerFunc);
	}
	void InputDispatcher::UnbindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc)
	{
		for (int i = mouseActionHandlers.Count() - 1; i >= 0; i--)
		{
			if (mouseActionHandlers[i] == handlerFunc)
			{
				mouseActionHandlers.RemoveAt(i);
				break;
			}
		}
	}
	void InputDispatcher::DispatchInput()
	{
		for (auto & binding : keyboardActionMappings)
		{
			if (inputInterface->QueryKeyState(binding.Key).HasPressed)
			{
				auto handlers = actionHandlers.TryGetValue(binding.Value.ActionName);
				if (handlers)
				{
					for (auto & handler : *handlers)
						if (handler(binding.Value.ActionName, binding.Value.Value))
							break;
				}
			}
		}

		for (auto & binding : keyboardAxisMappings)
		{
			if (inputInterface->QueryKeyState(binding.Key).IsDown)
			{
				auto handlers = actionHandlers.TryGetValue(binding.Value.ActionName);
				if (handlers)
				{
					for (auto & handler : *handlers)
						if (handler(binding.Value.ActionName, binding.Value.Value))
							break;
				}
			}
		}
	}
}
