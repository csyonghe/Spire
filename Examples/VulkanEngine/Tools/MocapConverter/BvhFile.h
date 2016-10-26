#ifndef GAME_ENGINE_BVH_FILE_H
#define GAME_ENGINE_BVH_FILE_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"

namespace GameEngine
{
	namespace Tools
	{
		enum class ChannelType
		{
			XPos, YPos, ZPos, XRot, YRot, ZRot, XScale, YScale, ZScale
		};

		class BvhJoint
		{
		public:
			CoreLib::String Name;
			VectorMath::Vec3 Offset;
			CoreLib::Array<ChannelType, 9> Channels;
			CoreLib::List<CoreLib::RefPtr<BvhJoint>> SubJoints;
		};

		class BvhFile
		{
		public:
			CoreLib::RefPtr<BvhJoint> Hierarchy;
			float FrameDuration;
			CoreLib::List<float> FrameData;
			static BvhFile FromFile(const CoreLib::String & fileName);
		};
	}
}

#endif