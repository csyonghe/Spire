#ifndef MOTION_GRAPH_BUILDER_H
#define MOTION_GRAPH_BUILDER_H

#include "MotionGraphBuilder.h"
#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Stream.h"
#include "GameEngineCore/MotionGraph.h"
#include "GameEngineCore/AnimationSynthesizer.h"


namespace GameEngine
{
    namespace Tools
    {
        MotionGraph BuildMotionGraph(const CoreLib::String & filename);
    }
}

#endif
