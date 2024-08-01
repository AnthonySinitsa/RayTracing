#include "lve_renderer.hpp"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {

	/**
	 * @brief Manages the Vulkan rendering process including swap chain handling and command buffer management.
	 *
	 * The `LveRenderer` class is responsible for setting up and managing Vulkan resources related to rendering,
	 * such as the swap chain and command buffers. It handles the initialization, recreation, and cleanup of
	 * these resources, as well as managing the lifecycle of rendering commands for each frame.
	 */
	LveRenderer::LveRenderer(LveWindow& window, LveDevice& device) : lveWindow{ window }, lveDevice{ device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	/**
	 * @brief Destructor for `LveRenderer`.
	 *
	 * Frees command buffers and performs necessary cleanup.
	 */
	LveRenderer::~LveRenderer() { freeCommandBuffers(); }

	/**
	 * @brief Recreates the swap chain and associated resources.
	 *
	 * This method is called when the window size changes or if the swap chain is invalidated. It handles
	 * the recreation of the swap chain and verifies if the swap chain image formats have changed.
	 */
	void LveRenderer::recreateSwapChain() {
		auto extent = lveWindow.getExtent();
		while (extent.width = 0 || extent.height == 0) {
			extent = lveWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(lveDevice.device());

		if (lveSwapChain == nullptr) {
			lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent);
		} else {
			std::shared_ptr<LveSwapChain> oldSwapChain = std::move(lveSwapChain);
			lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*lveSwapChain.get())) {
				throw std::runtime_error("Swap chain image(or dept) format has changed.");
			}
		}
	}

	/**
	 * @brief Allocates command buffers for rendering.
	 *
	 * This method allocates command buffers based on the number of frames in flight and associates them
	 * with the command pool.
	 */
	void LveRenderer::createCommandBuffers() {
		commandBuffers.resize(LveSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers!");
		}
	}

	/**
	 * @brief Frees allocated command buffers.
	 *
	 * This method cleans up and deallocates command buffers that were previously allocated.
	 */
	void LveRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(
			lveDevice.device(),
			lveDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());
		commandBuffers.clear();
	}

	/**
	 * @brief Begins a new frame by acquiring an image from the swap chain.
	 *
	 * @return VkCommandBuffer The command buffer to record commands for the current frame.
	 *
	 * This method acquires the next available image from the swap chain and begins recording commands into
	 * the associated command buffer. It handles the case where the swap chain is out of date and needs to
	 * be recreated.
	 */
	VkCommandBuffer LveRenderer::beginFrame() {
		assert(!isFrameStarted && "Can't call beginFrame while already in progress.");

		auto result = lveSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		return commandBuffer;
	}

	/**
	 * @brief Ends the current frame and submits the command buffer to the swap chain.
	 *
	 * This method ends the recording of the current command buffer and submits it to the swap chain for
	 * presentation. It handles the case where the swap chain needs to be recreated or the window was resized.
	 */
	void LveRenderer::endFrame() {
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress.");
		auto commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}

		auto result = lveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || lveWindow.wasWindowResized()) {
			lveWindow.resetWindowResizedFlag();
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % LveSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	/**
	 * @brief Begins a render pass for the current frame.
	 *
	 * @param commandBuffer The command buffer to begin the render pass on.
	 *
	 * This method sets up the render pass, including clearing values and viewport settings. It must be called
	 * before recording any rendering commands into the command buffer.
	 */
	void LveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame.");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = lveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, lveSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	/**
	 * @brief Ends the current render pass.
	 *
	 * @param commandBuffer The command buffer to end the render pass on.
	 *
	 * This method concludes the render pass and finalizes any pending rendering commands. It must be called
	 * after all rendering commands have been recorded.
	 */
	void LveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame.");

		vkCmdEndRenderPass(commandBuffer);
	}
}