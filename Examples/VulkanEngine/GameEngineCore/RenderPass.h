#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "Common.h"
#include "Level.h"

namespace GameEngine
{
	class RenderPass : public CoreLib::Object
	{
	public:
		virtual void RecordStaticCommandBuffer(Level * level) = 0;
		virtual void RecordDynamicCommandBuffer(Level * level) = 0;

	};
}

#endif