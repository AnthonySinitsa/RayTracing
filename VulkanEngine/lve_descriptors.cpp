/**
 * @file lve_descriptors.cpp
 * @brief Implementation of the descriptor set layout, descriptor pool, and descriptor writer for Vulkan.
 *
 * This file contains the implementation of classes for managing Vulkan descriptor sets, layouts, and pools.
 * These classes facilitate the creation, allocation, and updating of descriptor sets used in Vulkan pipelines.
 */

#include "lve_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>

namespace lve {

    // *************** Descriptor Set Layout Builder *********************

    /**
     * @brief Adds a binding to the descriptor set layout.
     *
     * This method adds a descriptor set layout binding to the layout being built.
     *
     * @param binding The binding number for the descriptor.
     * @param descriptorType The type of descriptor.
     * @param stageFlags The shader stages that will access this descriptor.
     * @param count The number of descriptors in the binding.
     * @return A reference to the Builder for chaining.
     */
    LveDescriptorSetLayout::Builder& LveDescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    /**
     * @brief Builds the descriptor set layout.
     *
     * This method finalizes the descriptor set layout and returns a unique pointer to the created layout.
     *
     * @return A unique pointer to the created LveDescriptorSetLayout.
     */
    std::unique_ptr<LveDescriptorSetLayout> LveDescriptorSetLayout::Builder::build() const {
        return std::make_unique<LveDescriptorSetLayout>(lveDevice, bindings);
    }

    // *************** Descriptor Set Layout *********************

    /**
     * @brief Constructor for LveDescriptorSetLayout.
     *
     * This constructor initializes the descriptor set layout with the specified device and bindings.
     *
     * @param lveDevice A reference to the LveDevice.
     * @param bindings A map of descriptor set layout bindings.
     */
    LveDescriptorSetLayout::LveDescriptorSetLayout(
        LveDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
        : lveDevice{ lveDevice }, bindings{ bindings } {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
            lveDevice.device(),
            &descriptorSetLayoutInfo,
            nullptr,
            &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    /**
     * @brief Destructor for LveDescriptorSetLayout.
     *
     * This destructor destroys the descriptor set layout.
     */
    LveDescriptorSetLayout::~LveDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(lveDevice.device(), descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool Builder *********************

    /**
     * @brief Adds a pool size to the descriptor pool.
     *
     * This method adds a descriptor type and its count to the pool sizes.
     *
     * @param descriptorType The type of descriptor.
     * @param count The number of descriptors of this type.
     * @return A reference to the Builder for chaining.
     */
    LveDescriptorPool::Builder& LveDescriptorPool::Builder::addPoolSize(
        VkDescriptorType descriptorType, uint32_t count) {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    /**
     * @brief Sets the pool flags for the descriptor pool.
     *
     * This method sets the flags for the descriptor pool.
     *
     * @param flags The flags to set for the descriptor pool.
     * @return A reference to the Builder for chaining.
     */
    LveDescriptorPool::Builder& LveDescriptorPool::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    /**
     * @brief Sets the maximum number of sets for the descriptor pool.
     *
     * This method sets the maximum number of descriptor sets that can be allocated from the pool.
     *
     * @param count The maximum number of descriptor sets.
     * @return A reference to the Builder for chaining.
     */
    LveDescriptorPool::Builder& LveDescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }

    /**
     * @brief Builds the descriptor pool.
     *
     * This method finalizes the descriptor pool and returns a unique pointer to the created pool.
     *
     * @return A unique pointer to the created LveDescriptorPool.
     */
    std::unique_ptr<LveDescriptorPool> LveDescriptorPool::Builder::build() const {
        return std::make_unique<LveDescriptorPool>(lveDevice, maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    /**
     * @brief Constructor for LveDescriptorPool.
     *
     * This constructor initializes the descriptor pool with the specified parameters.
     *
     * @param lveDevice A reference to the LveDevice.
     * @param maxSets The maximum number of sets.
     * @param poolFlags The flags for the descriptor pool.
     * @param poolSizes The sizes of the descriptor pools.
     */
    LveDescriptorPool::LveDescriptorPool(
        LveDevice& lveDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : lveDevice{ lveDevice } {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(lveDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    /**
     * @brief Destructor for LveDescriptorPool.
     *
     * This destructor destroys the descriptor pool.
     */
    LveDescriptorPool::~LveDescriptorPool() {
        vkDestroyDescriptorPool(lveDevice.device(), descriptorPool, nullptr);
    }

    /**
     * @brief Allocates a descriptor set from the pool.
     *
     * This method allocates a descriptor set from the descriptor pool.
     *
     * @param descriptorSetLayout The layout of the descriptor set.
     * @param descriptor A reference to the VkDescriptorSet to be allocated.
     * @return True if allocation was successful, false otherwise.
     */
    bool LveDescriptorPool::allocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up. But this is beyond our current scope
        if (vkAllocateDescriptorSets(lveDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    /**
     * @brief Frees descriptor sets.
     *
     * This method frees the specified descriptor sets.
     *
     * @param descriptors A vector of VkDescriptorSet to be freed.
     */
    void LveDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(
            lveDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    /**
     * @brief Resets the descriptor pool.
     *
     * This method resets the descriptor pool, freeing all resources allocated from it.
     */
    void LveDescriptorPool::resetPool() {
        vkResetDescriptorPool(lveDevice.device(), descriptorPool, 0);
    }

    // *************** Descriptor Writer *********************

    /**
     * @brief Constructor for LveDescriptorWriter.
     *
     * This constructor initializes the descriptor writer with the specified set layout and pool.
     *
     * @param setLayout A reference to the LveDescriptorSetLayout.
     * @param pool A reference to the LveDescriptorPool.
     */
    LveDescriptorWriter::LveDescriptorWriter(LveDescriptorSetLayout& setLayout, LveDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {}

    /**
     * @brief Writes a buffer to the descriptor set.
     *
     * This method writes a buffer descriptor to the descriptor set being built.
     *
     * @param binding The binding number for the descriptor.
     * @param bufferInfo A reference to the VkDescriptorBufferInfo.
     * @return A reference to the LveDescriptorWriter for chaining.
     */
    LveDescriptorWriter& LveDescriptorWriter::writeBuffer(
        uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    /**
     * @brief Writes an image to the descriptor set.
     *
     * This method writes an image descriptor to the descriptor set being built.
     *
     * @param binding The binding number for the descriptor.
     * @param imageInfo A reference to the VkDescriptorImageInfo.
     * @return A reference to the LveDescriptorWriter for chaining.
     */
    LveDescriptorWriter& LveDescriptorWriter::writeImage(
        uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    /**
     * @brief Builds the descriptor set.
     *
     * This method finalizes and updates the descriptor set with the specified writes.
     *
     * @param set A reference to the VkDescriptorSet to be built.
     * @return True if the descriptor set was successfully built, false otherwise.
     */
    bool LveDescriptorWriter::build(VkDescriptorSet& set) {
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    /**
     * @brief Overwrites the descriptor set.
     *
     * This method updates the descriptor set with the specified writes.
     *
     * @param set A reference to the VkDescriptorSet to be overwritten.
     */
    void LveDescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.lveDevice.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

}