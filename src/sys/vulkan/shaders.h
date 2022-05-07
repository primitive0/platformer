#pragma once

#include <vector>
#include <fstream>

#include "device.h"

inline std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    file.close();

    return buffer;
}

struct Shaders {
    VkShaderModule vertShader;
    VkShaderModule fragShader;

    static Shaders loadShaders(const Device& device) {
        auto vertShaderCode = readFile("vert.spv");
        VkShaderModule vertShader = device.createShaderModule(vertShaderCode);
        auto fragShaderCode = readFile("frag.spv");
        VkShaderModule fragShader = device.createShaderModule(fragShaderCode);
        return {vertShader, fragShader};
    }

    void destroy(const Device& device) const noexcept {
        vkDestroyShaderModule(device.getHandle(), vertShader, nullptr);
        vkDestroyShaderModule(device.getHandle(), fragShader, nullptr);
    }
};