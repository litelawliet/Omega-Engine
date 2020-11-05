#pragma once
#include <OgRendering/Utils/VulkanTools.h>

struct GeometryInstance
{
	glm::mat3x4 transform;
	uint32_t instanceId : 24;
	uint32_t mask : 8;
	uint32_t instanceOffset : 24;
	uint32_t flags : 8;
	uint64_t accelerationStructureHandle;
};

struct GeometryBuffer
{
public:
	Buffer m_geometryBuffer;
	Buffer m_vertBuffer;
	Buffer m_indexBuffer;
};

struct GeometryInfo
{
public:
	GeometryInfo(VkAccelerationStructureGeometryKHR& g, VkAccelerationStructureBuildOffsetInfoKHR& o, VkAccelerationStructureCreateGeometryTypeInfoKHR& gi, GeometryBuffer* b)
	{
		geometry = g;
		offset = o;
		geometryInfo = gi;
		buffers = b;
	}
	GeometryBuffer* buffers;
	VkAccelerationStructureGeometryKHR geometry;
	VkAccelerationStructureBuildOffsetInfoKHR offset;
	VkAccelerationStructureCreateGeometryTypeInfoKHR geometryInfo;
};

struct AccelerationStructure
{
	VkDeviceMemory memory{};
	VkDeviceSize memSize;
	VkAccelerationStructureKHR accelerationStructure{};
	GeometryInfo* info;

	uint64_t handle{ 0u };
	bool isBuilt;
};