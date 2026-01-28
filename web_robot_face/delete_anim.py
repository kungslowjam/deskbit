import sys
import os
import re
from pathlib import Path

def delete_animation(anim_name):
    print(f"🗑️  Deleting Animation: {anim_name}...")
    
    # Setup Paths
    script_dir = Path(__file__).parent
    project_dir = script_dir.parent
    user_app_dir = project_dir / "components" / "user_app"
    anim_dir = user_app_dir / "animations"
    
    if not user_app_dir.exists():
        print("❌ Error: Cannot find user_app directory!")
        return

    # 1. Delete Files
    c_file = anim_dir / f"{anim_name}.c"
    h_file = anim_dir / f"{anim_name}.h"
    
    if c_file.exists():
        os.remove(c_file)
        print(f"✅ Deleted: {c_file.name}")
    
    if h_file.exists():
        os.remove(h_file)
        print(f"✅ Deleted: {h_file.name}")
        
    # 2. Clean CMakeLists.txt
    cmake_file = user_app_dir / "CMakeLists.txt"
    if cmake_file.exists():
        with open(cmake_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Remove the file entry (handling quotes and spaces)
        # Regex looks for: "animations/name.c" possibly surrounded by spaces
        pattern = f'\\s*"animations/{anim_name}\\.c"'
        new_content = re.sub(pattern, '', content)
        
        if new_content != content:
            with open(cmake_file, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print("✅ Cleaned: CMakeLists.txt")
        else:
            print("ℹ️  CMakeLists.txt did not contain this animation.")

    # 3. Clean anim_registry.c
    registry_file = user_app_dir / "anim_registry.c"
    if registry_file.exists():
        with open(registry_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        new_lines = []
        modified = False
        
        for line in lines:
            # Check for Include
            if f'#include "animations/{anim_name}.h"' in line:
                modified = True
                continue # Skip this line
            
            # Check for Register call (looking for register("name", ...)
            # We look for the name usage in register function
            clean_name = anim_name.replace("_anim", "")
            if f'anim_manager_register("{clean_name}"' in line or \
               f'anim_manager_register("{anim_name}"' in line or \
               f'anim_manager_register_vector(&{anim_name}_data)' in line or \
               f'anim_manager_register_vector(&{clean_name}_data)' in line:
                modified = True
                continue # Skip
                
            new_lines.append(line)
            
        if modified:
            with open(registry_file, 'w', encoding='utf-8') as f:
                f.writelines(new_lines)
            print("✅ Cleaned: anim_registry.c")
        else:
            print("ℹ️  anim_registry.c did not contain this animation.")

    print("\n✨ Done! Animation removed successfully.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        name = sys.argv[1]
    else:
        name = input("Enter animation name to delete (e.g. my_anim): ").strip()
    
    if name:
        # Auto-add _anim suffix if user forgot, but check file existence first
        # Actually better to stick to exactly what user typed, or handle both
        delete_animation(name)
    else:
        print("❌ No name provided.")
