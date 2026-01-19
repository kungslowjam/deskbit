
import os

template_path = "components/lvgl/lv_conf_template.h"
conf_path = "components/lvgl/lv_conf.h"

if not os.path.exists(template_path):
    print(f"Error: Template {template_path} not found.")
    exit(1)

with open(template_path, "r") as f:
    content = f.read()

# Enable the config
content = content.replace("#if 0 /*Set it to \"1\" to enable content*/", "#if 1 /*Set it to \"1\" to enable content*/")

# Enable GIF
content = content.replace("#define LV_USE_GIF 0", "#define LV_USE_GIF 1")

# Enable specific fonts if needed (optional, just to be safe for previous user request)
# content = content.replace("#define LV_FONT_MONTSERRAT_48 0", "#define LV_FONT_MONTSERRAT_48 1")

with open(conf_path, "w") as f:
    f.write(content)

print(f"Successfully created {conf_path} with GIF enabled.")
