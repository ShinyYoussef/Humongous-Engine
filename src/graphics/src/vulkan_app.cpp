#include <vulkan_app.hpp>

namespace Humongous
{

VulkanApp::VulkanApp() { Init(); }

VulkanApp::~VulkanApp() { Cleanup(); }

void VulkanApp::Init()
{
    instance = std::make_unique<Instance>();
    physicalDevice = std::make_unique<PhysicalDevice>(instance->GetVkInstance());
    logicalDevice = std::make_unique<LogicalDevice>(instance->GetVkInstance(), physicalDevice->GetVkPhysicalDevice());
    window = std::make_unique<Window>();
}

void VulkanApp::Cleanup() {}

} // namespace Humongous
