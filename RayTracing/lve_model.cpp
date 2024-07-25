#include "lve_model.hpp"

// std
#include <cassert>
#include <cstring>

namespace lve {
	LveModel::LveModel(LveDevice& device, const std::vector<Vertex>& vertices) : lveDevice{ device } {
		createVertexBuffers(vertices);
	}
	LveModel::~LveModel() {
		vkDestroyBuffer(lveDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(lveDevice.device(), vertexBufferMemory, nullptr);
	}

	void LveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		lveDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERANT_IT,
			vertexBuffer,
			vertexBufferMemory
		);

		void* data;
		vkMapMemory(lveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, verteices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(lveDevice.device(), vertexBufferMemory);
	}
}