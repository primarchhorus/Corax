#include "camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/projection.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <variant>


namespace Vulkan {

    namespace Camera {
        void updateVelocityFromEvent(Type& camera, int key, int scancode, int action, int mods) {
            auto visitor = [key, scancode, action, mods](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FirstPerson>) {
                    switch (key) {
                        case GLFW_KEY_W:
                            arg.velocity.z = (action != GLFW_RELEASE) ? -1 : 0;
                            break;
                        case GLFW_KEY_A:
                            arg.velocity.x = (action != GLFW_RELEASE) ? -1 : 0;
                            break;
                        case GLFW_KEY_S:
                            arg.velocity.z = (action != GLFW_RELEASE) ? 1 : 0;
                            break;
                        case GLFW_KEY_D:
                            arg.velocity.x = (action != GLFW_RELEASE) ? 1 : 0;
                            break;
                        default:
                        break;
                    }

                    arg.velocity = arg.velocity * 2.0f;
                    
                };
            };
            std::visit(visitor, camera);
        }
        
        void updatePitchYawFromEvent(Type& camera, double xpos, double ypos) {
            auto visitor = [xpos, ypos](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FirstPerson>) {
                    arg.x_relative = xpos - arg.last_pos_x;
                    arg.y_relative = ypos - arg.last_pos_y;
                    arg.yaw += static_cast<float>(arg.x_relative) / 1000.f;
                    arg.pitch -= static_cast<float>(arg.y_relative) / 1000.f;
                    arg.last_pos_x = xpos;
                    arg.last_pos_y = ypos;
                };
            };
            std::visit(visitor, camera);
        }

        void updatePosition(Type& camera, float delta_time) {
            auto visitor = [delta_time](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FirstPerson>) {
                    Type wrap = arg;
                    glm::mat4 camera_rotation = getRotationMatrix(wrap);
                    arg.position += glm::vec3(camera_rotation * glm::vec4(arg.velocity * delta_time, 0.f));
                };
            };
            std::visit(visitor, camera);
        }

        glm::mat4 getViewMatrix(Type& camera)
        {
            auto visitor = [](auto&& arg) -> glm::mat4 {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FirstPerson>) {
                    Type wrap = arg;
                    glm::mat4 camera_translation = glm::translate(glm::mat4(1.f), arg.position);
                    glm::mat4 camera_rotation = getRotationMatrix(wrap);
                    return glm::inverse(camera_translation * camera_rotation);
                };
                return glm::mat4(1.0f);
            };
            return std::visit(visitor, camera);
        }

        glm::mat4 getRotationMatrix(Type& camera)
        {
            auto visitor = [](auto&& arg) -> glm::mat4 {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FirstPerson>) {
                    glm::quat pitch_rotation = glm::angleAxis(arg.pitch, glm::vec3 { 1.f, 0.f, 0.f });
                    glm::quat yaw_rotation = glm::angleAxis(arg.yaw, glm::vec3 { 0.f, -1.f, 0.f });
                    return glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
                };
                return glm::mat4(1.0f);
            };
            return std::visit(visitor, camera);

        }
    }
}