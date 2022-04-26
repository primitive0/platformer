#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <functional>

#include "glfw.h"

#include "debug.h"
#include "window.h"
#include "helper/vulkan.h"
#include "vulkan.h"


void cleanup() {
    terminateGlfw();
}

void printVulkanAvailableExtensions() {
    auto vkExtensions = enumerateInstanceExtensionProperties();

    std::cout << "available extensions:\n";

    for (const auto& extension : vkExtensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

int main() {
    setupDebug();

    printVulkanAvailableExtensions();

    Window window(800, 600, "Vulkan sample");

    if (debugEnabled && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    auto instance = createVulkanInstance(VulkanApplicationInfo{
            .applicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .engineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    });

    auto surface = createVulkanSurface(instance, window);
    auto physicalDevice = findPhysicalDevice(instance, surface);
    auto device = createVulkanLogicalDevice(physicalDevice);

    auto swapChainCreateDetails = lookupSwapChainCreateDetails(physicalDevice, window);

    auto swapChain = createSwapChain(physicalDevice, device, surface, swapChainCreateDetails);

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

//    while (!window.shouldClose()) {
//        glfwPollEvents();
//    }

    cleanup();

    return EXIT_SUCCESS;
}
