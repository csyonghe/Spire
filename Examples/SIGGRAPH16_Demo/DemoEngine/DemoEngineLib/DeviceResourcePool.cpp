#include "DeviceResourcePool.h"

namespace DemoEngine
{
	StaticMesh::StaticMesh(DeviceResourcePool * engine)
	{
		this->engine = engine;
		this->verticesBuffer = engine->GetHardwareRenderer()->CreateBuffer(BufferUsage::ArrayBuffer);
		this->indicesBuffer = engine->GetHardwareRenderer()->CreateBuffer(BufferUsage::IndexBuffer);
		this->vertexArray = engine->GetHardwareRenderer()->CreateVertexArray();
		this->vertexArray.SetIndex(indicesBuffer);
	}

	void StaticMesh::Free()
	{
		if (engine)
		{
			engine->GetHardwareRenderer()->DestroyBuffer(verticesBuffer);
			engine->GetHardwareRenderer()->DestroyBuffer(indicesBuffer);
			engine->GetHardwareRenderer()->DestroyVertexArray(vertexArray);
		}
	}
}