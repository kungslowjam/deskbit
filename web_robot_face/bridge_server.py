#!/usr/bin/env python3
import http.server
import socketserver
import json
import os
import sys
from pathlib import Path

# Import existing logic (reuse your current powerful script)
try:
    from export_to_project import generate_c_file, generate_h_file, update_cmake, update_registry
except ImportError:
    pass

try:
    from delete_anim import delete_animation
except ImportError:
    pass

PORT = 8000

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/save-anim':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                data = json.loads(post_data)
                anim_name = data.get('name', 'my_anim')
                
                # Setup Paths
                script_dir = Path(__file__).parent
                project_dir = script_dir.parent
                user_app_dir = project_dir / "components" / "user_app"
                anim_dir = user_app_dir / "animations"
                
                # Check directories
                if not user_app_dir.exists():
                    self.send_error(500, "Project user_app directory not found")
                    return
                    
                if not anim_dir.exists():
                    anim_dir.mkdir(parents=True)
                
                print(f"\n⚡ Received Animation: {anim_name}")
                
                # 1. Generate C/H Files
                generated_name = generate_c_file(data, anim_dir)
                
                if generated_name:
                    # 1.5. Generate H File
                    generate_h_file(generated_name, anim_dir)
                    
                    # 2. Update CMake
                    update_cmake(generated_name, user_app_dir / "CMakeLists.txt")
                    
                    # 3. Update Registry
                    update_registry(generated_name, user_app_dir / "anim_registry.c", data.get('width', 466))
                    
                    # Success Response
                    self.send_response(200)
                    self.send_header('Content-type', 'application/json')
                    self.end_headers()
                    response = {"status": "success", "message": f"Saved {generated_name} to project!"}
                    self.wfile.write(json.dumps(response).encode())
                    print("✅ Success! Sent response to browser.")
                else:
                    self.send_error(500, "Failed to generate C files")
                    
            except Exception as e:
                print(f"❌ Error: {e}")
                self.send_error(500, str(e))
                
        elif self.path == '/delete-anim':
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            
            try:
                data = json.loads(post_data)
                anim_name = data.get('name')
                
                if not anim_name:
                    self.send_error(400, "No animation name provided")
                    return
                    
                print(f"\n🗑️  Request to Delete: {anim_name}")
                
                # Call delete logic
                delete_animation(anim_name)
                
                # Success Response
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                response = {"status": "success", "message": f"Deleted {anim_name} from project!"}
                self.wfile.write(json.dumps(response).encode())
                print("✅ Success! Sent response to browser.")
                
            except Exception as e:
                print(f"❌ Error: {e}")
                self.send_error(500, str(e))
                
        else:
            self.send_error(404)

    def do_OPTIONS(self):
        self.send_response(200)
        # End headers adds Access-Control-Allow-Origin
        self.send_header('Access-Control-Allow-Methods', 'POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

print(f"🤖 Bridge Server Running on http://localhost:{PORT}")
print("   Waiting for '⚡ Send' from Robot Studio...")

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping server...")
