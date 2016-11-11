#ifndef MOTION_GRAPH_H
#define MOTION_GRAPH_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "Skeleton.h"

namespace GameEngine
{
    class MGState
    {
    public:
        Pose Pose;
        CoreLib::List<VectorMath::Vec3> Positions, Velocities;
        CoreLib::EnumerableHashSet<int> ChildrenIds;
    };

    class MotionGraph
    {
    public:
        CoreLib::List<MGState> States;
        float Speed;
        float Duration;

        void SaveToStream(CoreLib::IO::Stream * stream);
        void LoadFromStream(CoreLib::IO::Stream * stream);
        void SaveToFile(const CoreLib::String & filename);
        void LoadFromFile(const CoreLib::String & filename);
    };
}

#endif
