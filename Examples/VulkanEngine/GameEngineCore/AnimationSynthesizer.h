#ifndef ANIMATION_SYNTHESIZER_H
#define ANIMATION_SYNTHESIZER_H

#include "Skeleton.h"
#include "MotionGraph.h"

namespace GameEngine
{
	class AnimationSynthesizer : public CoreLib::Object
	{
	public:
		virtual void GetPose(Pose & p, float time) = 0;
	};

	class SimpleAnimationSynthesizer : public AnimationSynthesizer
	{
	private:
		Skeleton * skeleton = nullptr;
		SkeletalAnimation * anim = nullptr;
	public:
		SimpleAnimationSynthesizer() = default;
		SimpleAnimationSynthesizer(Skeleton * pSkeleton, SkeletalAnimation * pAnim)
			: skeleton(pSkeleton), anim(pAnim)
		{}
		void SetSource(Skeleton * pSkeleton, SkeletalAnimation * pAnim)
		{
			this->skeleton = pSkeleton;
			this->anim = pAnim;
		}
		virtual void GetPose(Pose & p, float time) override;
	};

    class MotionGraphAnimationSynthesizer : public AnimationSynthesizer
    {
    private:
        Skeleton * skeleton = nullptr;
        MotionGraph * motionGraph = nullptr;
    public:
        MotionGraphAnimationSynthesizer() = default;
        MotionGraphAnimationSynthesizer(Skeleton * pSkeleton, MotionGraph * pMotionGraph)
            : skeleton(pSkeleton), motionGraph(pMotionGraph)
        {}
        void SetSource(Skeleton * pSkeleton, MotionGraph * pMotionGraph)
        {
            this->skeleton = pSkeleton;
            this->motionGraph = pMotionGraph;
        }
        virtual void GetPose(Pose & p, float time) override;
    };

}

#endif