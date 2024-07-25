#include "sierpinski.hpp"

namespace lve {

    void sierpinski(
        std::vector<LveModel::Vertex>& vertices,
        int recursionDepth,
        glm::vec2 vertexA,
        glm::vec2 vertexB,
        glm::vec2 vertexC
    ) {
        if (recursionDepth == 0) {
            vertices.push_back({ vertexA });
            vertices.push_back({ vertexB });
            vertices.push_back({ vertexC });
        }
        else {
            glm::vec2 midpointAB = (vertexA + vertexB) / 2.0f;
            glm::vec2 midpointBC = (vertexB + vertexC) / 2.0f;
            glm::vec2 midpointCA = (vertexC + vertexA) / 2.0f;

            sierpinski(vertices, recursionDepth - 1, vertexA, midpointAB, midpointCA);
            sierpinski(vertices, recursionDepth - 1, midpointAB, vertexB, midpointBC);
            sierpinski(vertices, recursionDepth - 1, midpointCA, midpointBC, vertexC);
        }
    }

}
