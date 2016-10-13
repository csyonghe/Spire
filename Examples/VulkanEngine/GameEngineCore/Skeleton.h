#ifndef GAME_ENGINE_RIG_H
#define GAME_ENGINE_RIG_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"

namespace GameEngine
{
	class BoneTransformation
	{
	public:
		VectorMath::Quaternion Rotation;
		VectorMath::Vec3 Translation, Scale;
		BoneTransformation()
		{
			Rotation = VectorMath::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
			Translation.SetZero();
			Scale.x = Scale.y = Scale.z = 1.0f;
		}
		VectorMath::Matrix4 ToMatrix()
		{
			auto rs = Rotation.ToMatrix4();
			rs.values[12] = Translation.x;
			rs.values[13] = Translation.y;
			rs.values[14] = Translation.z;
			rs.values[0] *= Scale.x;
			rs.values[1] *= Scale.x;
			rs.values[2] *= Scale.x;
			rs.values[4] *= Scale.y;
			rs.values[5] *= Scale.y;
			rs.values[6] *= Scale.y;
			rs.values[8] *= Scale.z;
			rs.values[9] *= Scale.z;
			rs.values[10] *= Scale.z;
			return rs;
		}
		static inline BoneTransformation Lerp(const BoneTransformation & t0, const BoneTransformation & t1, float t)
		{
			BoneTransformation result;
			result.Rotation = VectorMath::Quaternion::Slerp(t0.Rotation, t1.Rotation, t);
			result.Translation = VectorMath::Vec3::Lerp(t0.Translation, t1.Translation, t);
			result.Scale = VectorMath::Vec3::Lerp(t0.Scale, t1.Scale, t);
			return result;
		}
	};

	class Bone
	{
	public:
		int ParentId;
		CoreLib::String Name;
		BoneTransformation BindPose;
	};

	class Skeleton
	{
	public:
		CoreLib::List<Bone> Bones;
		CoreLib::List<VectorMath::Matrix4> InversePose;
		CoreLib::Dictionary<CoreLib::String, int> BoneMapping;
		void SaveToStream(CoreLib::IO::Stream * stream);
		void LoadFromStream(CoreLib::IO::Stream * stream);
		void SaveToFile(const CoreLib::String & filename);
		void LoadFromFile(const CoreLib::String & filename);
	};

	class Pose
	{
	public:
		CoreLib::List<BoneTransformation> Transforms;
		void GetMatrices(Skeleton * skeleton, CoreLib::List<VectorMath::Matrix4> & matrices)
		{
			matrices.Clear();
			matrices.SetSize(Transforms.Count());
			for (int i = 0; i < matrices.Count(); i++)
				matrices[i] = Transforms[i].ToMatrix();
			for (int i = 0; i < skeleton->Bones.Count(); i++)
			{
				auto tmp = matrices[i];
				if (skeleton->Bones[i].ParentId != -1)
					VectorMath::Matrix4::Multiply(matrices[i], matrices[skeleton->Bones[i].ParentId], tmp);
			}
			for (int i = 0; i < matrices.Count(); i++)
			{
				auto tmp = matrices[i];
				VectorMath::Matrix4::Multiply(matrices[i], tmp, skeleton->InversePose[i]);
				//VectorMath::Matrix4::CreateIdentityMatrix(matrices[i]);
			}
		}
	};

	class AnimationKeyFrame
	{
	public:
		float Time;
		BoneTransformation Transform;
	};

	class AnimationChannel
	{
	public:
		CoreLib::String BoneName;
		int BoneId = -1;
		CoreLib::List<AnimationKeyFrame> KeyFrames;
	};

	class SkeletalAnimation
	{
	public:
		CoreLib::String Name;
		float Speed;
		float Duration;
		int Reserved[15];
		CoreLib::List<AnimationChannel> Channels;
		void SaveToStream(CoreLib::IO::Stream * stream);
		void LoadFromStream(CoreLib::IO::Stream * stream);
		void SaveToFile(const CoreLib::String & filename);
		void LoadFromFile(const CoreLib::String & filename);
	};

}

#endif