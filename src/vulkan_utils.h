#include "vulkan_common.h"

// inline void vkCheck(VkResult result) {
//     fprintf(stderr, "Fatal : VkResult is %d, file: %s line: %d\n", result, __FILE__, __LINE__);
//     assert((result == VK_SUCCESS));
// }

#define vkCheck(result) \
    do { \
        if (result != VK_SUCCESS) { \
             fprintf(stderr, "Fatal : VkResult is %d, file: %s line: %d\n", result, __FILE__, __LINE__); \
            abort(); \
        } \
    } while (0)
