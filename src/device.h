#pragma once

#include "vulkan_common.h"

namespace Vulkan {

struct Device {
    Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    VkInstance instance;
};
}
