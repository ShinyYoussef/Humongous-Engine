#pragma once

#include "non_copyable.hpp"
#include <asserts.hpp>
#include <optional>

#include <vulkan/vulkan_core.h>

namespace Humongous
{
class PhysicalDevice : NonCopyable
{
    struct QueueFamilyIndices
    {
        std::optional<u32> graphicsFamily;

        bool IsComplete() { return graphicsFamily.has_value(); }
    };

public:
    PhysicalDevice(VkInstance instance);
    ~PhysicalDevice();

    VkPhysicalDevice GetVkPhysicalDevice() const { return m_physicalDevice; }

private:
    VkPhysicalDevice m_physicalDevice;

    void PickPhysicalDevice(VkInstance instance);

    bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);

    QueueFamilyIndices FindQueueFamilies();
};
} // namespace Humongous
