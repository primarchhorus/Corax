#pragma once

#include "vulkan_common.h"

#include <variant>

namespace Vulkan {
    namespace Camera {
        struct FirstPerson {
            glm::vec3 velocity{0.f, 0.f, 0.f};
            glm::vec3 position{0.f, 0.f, 0.f};
            double x_relative{0};
            double y_relative{0};
            double last_pos_x{0};
            double last_pos_y{0};
            float pitch{0.f};
            float yaw{0.f};
        };

        using Type = std::variant<FirstPerson>;

        void updateVelocityFromEvent(Type& camera, int key, int scancode, int action, int mods);
        void updatePitchYawFromEvent(Type& camera, double xpos, double ypos);
        void updatePosition(Type& camera, float delta_time);
        glm::mat4 getViewMatrix(Type& camera);
        glm::mat4 getRotationMatrix(Type& camera);
        glm::vec4 getPosition(Type& camera);
    }
}