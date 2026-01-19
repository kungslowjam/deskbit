
import sys
import os

def cmnt_header(out):
    out.write("/*\n")
    out.write(" *  Automatically generated file from a GIF\n")
    out.write(" *  Bin: " + sys.argv[1] + "\n")
    out.write(" */\n")
    out.write("\n")
    out.write("#include \"lvgl.h\"\n")
    out.write("\n")

def main():
    if len(sys.argv) < 4:
        print("Usage: gif_to_raw_c.py <input_file> <output_file> <var_name>")
        return

    in_path = sys.argv[1]
    out_path = sys.argv[2]
    var_name = sys.argv[3]

    if not os.path.exists(in_path):
        print(f"Error: Input file {in_path} not found.")
        return

    file_len = os.path.getsize(in_path)

    with open(in_path, "rb") as f, open(out_path, "w") as out:
        cmnt_header(out)
        out.write("#if LV_USE_GIF\n\n")
        out.write(f"const uint8_t {var_name}_map[] = {{\n")
        
        while True:
            chunk = f.read(16)
            if not chunk:
                break
            
            line_hex = []
            for b in chunk:
                line_hex.append(f"0x{b:02x}")
            
            out.write("    " + ", ".join(line_hex) + ",\n")
            
        out.write("};\n\n")
        
        # Write descriptor
        out.write(f"const lv_img_dsc_t {var_name} = {{\n")
        out.write("  .header.always_zero = 0,\n")
        out.write("  .header.w = 0,\n")
        out.write("  .header.h = 0,\n")
        out.write(f"  .data_size = {file_len},\n")
        out.write("  .header.cf = LV_IMG_CF_RAW,\n")
        out.write(f"  .data = {var_name}_map,\n")
        out.write("};\n\n")
        out.write("#endif\n")
    
    print("Filesize:", file_len)
    print("Done")

if __name__ == "__main__":
    main()
