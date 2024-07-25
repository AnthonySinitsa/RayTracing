#pragma once

#include "lve_model.hpp"
#include <vector>
#include <glm/vec2.hpp>

namespace lve {

	void sierpinski(
		std::vector<LveModel::Vertex>& vertices,
		int recursionDepth,
		glm::vec2 vertexA,
		glm::vec2 vertexB,
		glm::vec2 vertexC
	);

}
