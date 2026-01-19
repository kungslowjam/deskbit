
import os

conf_path = "components/lvgl/src/lv_conf.h"
template_path = "components/lvgl/lv_conf_template.h"

if not os.path.exists(template_path):
    print("Error: Template not found at", template_path)
    exit(1)

with open(template_path, "r") as f:
    content = f.read()

# Enable content
content = content.replace("#if 0 /*Set it to \"1\" to enable content*/", "#if 1 /*Set it to \"1\" to enable content*/")

# Enable GIF
content = content.replace("#define LV_USE_GIF 0", "#define LV_USE_GIF 1")

# Configure Memory for PSRAM override
# We need to replace the entire memory block or specific macros
# The original code:
# #define LV_MEM_CUSTOM 0
# #if LV_MEM_CUSTOM == 0
#     /*Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB)*/
#     #define LV_MEM_SIZE (48U * 1024U)          /*[bytes]*/
# ...
# #else       /*LV_MEM_CUSTOM*/
#     #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
#     #define LV_MEM_CUSTOM_ALLOC   malloc
#     #define LV_MEM_CUSTOM_FREE    free
#     #define LV_MEM_CUSTOM_REALLOC realloc
# #endif     /*LV_MEM_CUSTOM*/

# We will replace LV_MEM_CUSTOM 0 with 1, and then replace the custom macros.
content = content.replace("#define LV_MEM_CUSTOM 0", "#define LV_MEM_CUSTOM 1")

# Now replace the include and alloc macros
original_custom_block = """    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /*Header for the dynamic memory function*/
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc"""

new_custom_block = """    #define LV_MEM_CUSTOM_INCLUDE "esp_heap_caps.h"
    #define LV_MEM_CUSTOM_ALLOC(size)   heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
    #define LV_MEM_CUSTOM_FREE(ptr)     heap_caps_free(ptr)
    #define LV_MEM_CUSTOM_REALLOC(ptr, size) heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM)"""

content = content.replace(original_custom_block, new_custom_block)

with open(conf_path, "w") as f:
    f.write(content)

print(f"Successfully created {conf_path} with GIF enabled and PSRAM memory allocation.")
