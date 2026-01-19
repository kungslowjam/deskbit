import re
import os

filepath = r'c:\Users\hello\Desktop\desktop\components\user_app\settings_icon.c'

print(f"Reading {filepath}...")
with open(filepath, 'r') as f:
    content = f.read()

# Find the array body
match = re.search(r'uint8_t settings_map\[\] = \{', content)
if not match:
    print("Array start not found")
    exit(1)
start_idx = match.end()

end_marker = '};'
end_idx = content.find(end_marker, start_idx)
if end_idx == -1:
    print("Array end not found")
    exit(1)

# Extract the hex data part
data_str = content[start_idx:end_idx]

# Find all hex bytes
hex_pattern = re.compile(r'0x[0-9a-fA-F]{2}')
all_hex = hex_pattern.findall(data_str)

total_bytes = len(all_hex)
print(f"Found {total_bytes} bytes.")

if total_bytes < 4:
    print("Too few bytes")
    exit(1)

print(f"First 4 bytes read: {all_hex[0]}, {all_hex[1]}, {all_hex[2]}, {all_hex[3]}")

if total_bytes % 2 != 0:
    print(f"Error: Odd number of bytes found: {total_bytes}")
    exit(1)

print("Swapping bytes...")
# Swap pairs
swapped_hex = []
for i in range(0, total_bytes, 2):
    swapped_hex.append(all_hex[i+1]) # low byte
    swapped_hex.append(all_hex[i])   # high byte

print(f"First 4 bytes swapped: {swapped_hex[0]}, {swapped_hex[1]}, {swapped_hex[2]}, {swapped_hex[3]}")

# Reconstruct the array body
lines = []
chunk_size = 16
for i in range(0, len(swapped_hex), chunk_size):
    chunk = swapped_hex[i:i+chunk_size]
    line = "  " + ", ".join(chunk) + ","
    lines.append(line)

new_body = "\n" + "\n".join(lines) + "\n"

# Reconstruct file
new_content = content[:start_idx] + new_body + content[end_idx:]

print(f"Writing back to {filepath}...")
with open(filepath, 'w') as f:
    f.write(new_content)

print("Done.")
