/**
 * @file keyboard_movement_controller.cpp
 * @brief Implementation of the KeyboardMovementController class, which handles keyboard inputs for moving a game object.
 *
 * This file contains the implementation of the KeyboardMovementController class, responsible for processing keyboard inputs
 * to move a game object in the XZ plane and rotate it based on user input.
 */

#include "keyboard_movement_controller.hpp"

// libs
#include <limits>

namespace lve {

	/**
	 * @brief Moves and rotates a game object based on keyboard input.
	 *
	 * This function processes the keyboard inputs to control the movement and rotation of a game object.
	 * It allows the game object to move forward, backward, right, left, up, and down, as well as look in different directions.
	 *
	 * @param window The GLFW window capturing the user input.
	 * @param dt The time delta between the current and the previous frame.
	 * @param gameObject The game object to be moved and rotated.
	 */
	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window, float dt, LveGameObject& gameObject) {
		
		glm::vec3 rotate{ 0 };
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
		}

		// limit pitch
		gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
		gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

		float yaw = gameObject.transform.rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 moveDir{ 0.f };
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
}