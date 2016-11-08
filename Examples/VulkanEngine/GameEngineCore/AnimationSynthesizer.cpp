#include "AnimationSynthesizer.h"

using namespace CoreLib;

namespace GameEngine
{
	int BinarySearchForKeyFrame(List<AnimationKeyFrame> & keyFrames, float time)
	{
		int begin = 0;
		int end = keyFrames.Count();
		while (begin < end)
		{
			int mid = (begin + end) >> 1;
			if (keyFrames[mid].Time > time)
				end = mid;
			else if (keyFrames[mid].Time == time)
				return mid;
			else
				begin = mid + 1;
		}
		if (begin >= keyFrames.Count())
			begin = keyFrames.Count() - 1;
		if (keyFrames[begin].Time > time)
			begin--;
		if (begin < 0)
			begin = 0;
		return begin;
	}
	void SimpleAnimationSynthesizer::GetPose(Pose & p, float time)
	{
		p.Transforms.SetSize(skeleton->Bones.Count());
		float animTime = fmod(time * anim->Speed, anim->Duration);
        for (int i = 0; i < skeleton->Bones.Count(); i++)
            p.Transforms[i] = skeleton->Bones[i].BindPose;
		for (int i = 0; i < anim->Channels.Count(); i++)
		{
			if (anim->Channels[i].BoneId == -1)
				skeleton->BoneMapping.TryGetValue(anim->Channels[i].BoneName, anim->Channels[i].BoneId);
			if (anim->Channels[i].BoneId != -1)
			{
				int frame0 = BinarySearchForKeyFrame(anim->Channels[i].KeyFrames, animTime);
				int frame1 = frame0 + 1;
				float t = 0.0f;
				if (frame0 < anim->Channels[i].KeyFrames.Count() - 1)
				{
					t = (animTime - anim->Channels[i].KeyFrames[frame0].Time) / (anim->Channels[i].KeyFrames[frame1].Time - anim->Channels[i].KeyFrames[frame0].Time);
				}
				else
				{
					float b = (anim->Duration - anim->Channels[i].KeyFrames[frame0].Time);
					if (b <= 1e-4)
						t = 0.0f;
					else 
						t = (animTime - anim->Channels[i].KeyFrames[frame0].Time) / b;
					frame1 = 0;
				}
                auto & f0 = anim->Channels[i].KeyFrames[frame0];
				auto & f1 = anim->Channels[i].KeyFrames[frame1];
                
                p.Transforms[anim->Channels[i].BoneId] = BoneTransformation::Lerp(f0.Transform, f1.Transform, t); //BoneTransformation(); //
				//p.Transforms[anim->Channels[i].BoneId].Rotation = VectorMath::Quaternion();
			}
		}
        /*for (int i = 0; i < p.Transforms.Count(); i++)
            p.Transforms[i] = BoneTransformation();*/
	}

}