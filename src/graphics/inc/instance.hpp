#pragma once

#include <asserts.hpp>
#include <defines.hpp>
#include <non_copyable.hpp>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace Humongous
{

class Instance : NonCopyable
{
public:
    Instance();
    ~Instance();

    VkInstance GetVkInstance() const { return instance; };

private:
    VkInstance instance;

    VkDebugUtilsMessengerEXT       debugMessenger;
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifndef _DEBUG
    const bool EnableValidationLayers = false;
#else
    const bool ENABLE_VALIDATION_LAYERS = true;
#endif

    void                     InitInstance();
    bool                     CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();

    void SetupDebugMessenger();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

} // namespace Humongous
