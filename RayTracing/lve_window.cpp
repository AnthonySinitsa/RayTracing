#include "lve_window.hpp"

// std
#include <stdexcept>

namespace lve {
	
	/**
	 * @brief Manages the creation and handling of a GLFW window for Vulkan rendering.
	 *
	 * The `LveWindow` class handles the creation and management of a GLFW window, including initializing GLFW,
	 * setting window hints, creating the window, and managing window size changes. It also provides functionality
	 * to create a Vulkan surface for rendering.
	 */
	LveWindow::LveWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
		initWindow();
	}

	/**
	 * @brief Destructor for `LveWindow`.
	 *
	 * Cleans up the GLFW window and terminates GLFW.
	 */
	LveWindow::~LveWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	/**
	 * @brief Initializes GLFW and creates a window.
	 *
	 * Sets GLFW window hints, creates a window with the specified width, height, and name, and sets up
	 * the framebuffer size callback to handle window resizing.
	 */
	void LveWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	/**
	 * @brief Creates a Vulkan surface for the window.
	 *
	 * @param instance The Vulkan instance to associate with the surface.
	 * @param surface Pointer to a `VkSurfaceKHR` handle where the created surface will be stored.
	 *
	 * This method creates a Vulkan surface from the GLFW window, which is necessary for rendering with Vulkan.
	 *
	 * @throws std::runtime_error If the surface creation fails.
	 */
	void LveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface");
		}
	}

	/**
	 * @brief GLFW callback for handling framebuffer size changes.
	 *
	 * @param window The GLFW window that triggered the callback.
	 * @param width The new width of the framebuffer.
	 * @param height The new height of the framebuffer.
	 *
	 * This static method is called by GLFW when the framebuffer size changes. It updates the `framebufferResized`
	 * flag and the dimensions of the window.
	 */
	void LveWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto lveWindow = reinterpret_cast<LveWindow*>(glfwGetWindowUserPointer(window));
		lveWindow->framebufferResized = true;
		lveWindow->width = width;
		lveWindow->height = height;
	}

}