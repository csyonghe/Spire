#include "FreeRoamCameraController.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace VectorMath;

	bool FreeRoamCameraController::ParseField(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead(L"TargetCamera"))
		{
			parser.ReadToken();
			targetCameraName = parser.ReadStringLiteral();
			return true;
		}
		else if (parser.LookAhead(L"Speed"))
		{
			parser.ReadToken();
			cameraSpeed = (float)parser.ReadDouble();
			return true;
		}
		else if (parser.LookAhead(L"TurnPrecision"))
		{
			parser.ReadToken();
			turnPrecision = (float)parser.ReadDouble();
			return true;
		}
		return false;
	}
	void FreeRoamCameraController::OnLoad()
	{
		Actor::OnLoad();
		auto actor = Engine::Instance()->GetLevel()->FindActor(targetCameraName);
		if (actor && actor->GetEngineType() == EngineActorType::Camera)
			targetCamera = (CameraActor*)actor;

		Engine::Instance()->GetInputDispatcher()->BindActionHandler(L"MoveForward", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveForward));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler(L"MoveRight", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveRight));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler(L"MoveUp", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveUp));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler(L"TurnRight", ActionInputHandlerFunc(this, &FreeRoamCameraController::TurnRight));
		Engine::Instance()->GetInputDispatcher()->BindActionHandler(L"TurnUp", ActionInputHandlerFunc(this, &FreeRoamCameraController::TurnUp));
	}
	void FreeRoamCameraController::OnUnload()
	{
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler(L"MoveForward", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveForward));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler(L"MoveRight", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveRight));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler(L"MoveUp", ActionInputHandlerFunc(this, &FreeRoamCameraController::MoveUp));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler(L"TurnRight", ActionInputHandlerFunc(this, &FreeRoamCameraController::TurnRight));
		Engine::Instance()->GetInputDispatcher()->UnbindActionHandler(L"TurnUp", ActionInputHandlerFunc(this, &FreeRoamCameraController::TurnUp));
	}
	EngineActorType FreeRoamCameraController::GetEngineType()
	{
		return EngineActorType::UserController;
	}
	bool FreeRoamCameraController::MoveForward(const CoreLib::String & /*axisName*/, float scale)
	{
		if (targetCamera)
		{
			Vec3 moveDir = -Vec3::Create(targetCamera->LocalTransform.values[2], targetCamera->LocalTransform.values[6], targetCamera->LocalTransform.values[10]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraController::MoveRight(const CoreLib::String & /*axisName*/, float scale)
	{
		if (targetCamera)
		{
			Vec3 moveDir = Vec3::Create(targetCamera->LocalTransform.values[0], targetCamera->LocalTransform.values[4], targetCamera->LocalTransform.values[8]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraController::MoveUp(const CoreLib::String & /*axisName*/, float scale)
	{
		if (targetCamera)
		{
			Vec3 moveDir = Vec3::Create(targetCamera->LocalTransform.values[1], targetCamera->LocalTransform.values[5], targetCamera->LocalTransform.values[9]);
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPosition(targetCamera->GetPosition() + moveDir * (scale * dTime * cameraSpeed));
			return true;
		}
		return false;
	}
	bool FreeRoamCameraController::TurnRight(const CoreLib::String & /*axisName*/, float scale)
	{
		if (targetCamera)
		{
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetYaw(targetCamera->GetYaw() + scale * dTime * turnPrecision);
			return true;
		}
		return false;
	}
	bool FreeRoamCameraController::TurnUp(const CoreLib::String & /*axisName*/, float scale)
	{
		if (targetCamera)
		{
			float dTime = Engine::Instance()->GetTimeDelta(EngineThread::GameLogic);
			targetCamera->SetPitch(targetCamera->GetPitch() - scale * dTime * turnPrecision);
			return true;
		}
		return false;
	}
}
