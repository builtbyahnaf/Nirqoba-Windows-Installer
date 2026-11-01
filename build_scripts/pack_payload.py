#!/usr/bin/env python3
"""
pack_payload.py - Packages files from dist/package into dist/payload.bin
Format:
  4 bytes: Magic 'NRQB'
  4 bytes: uint32 file count
  For each file:
    2 bytes: uint16 relative path length (bytes)
    N bytes: UTF-8 relative path (using backslashes for Windows)
    4 bytes: uint32 file data size
    M bytes: raw file data
"""
import os
import sys
import struct

def pack(package_dir, output_bin):
    package_dir = os.path.abspath(package_dir)
    output_bin = os.path.abspath(output_bin)
    
    files_to_pack = []
    total_size = 0
    
    for root, dirs, files in os.walk(package_dir):
        for f in files:
            full_path = os.path.join(root, f)
            rel_path = os.path.relpath(full_path, package_dir).replace('/', '\\')
            size = os.path.getsize(full_path)
            files_to_pack.append((rel_path, full_path, size))
            total_size += size

    print(f"[PACK] Packing {len(files_to_pack)} files ({total_size / (1024*1024):.2f} MB)...")
    
    with open(output_bin, 'wb') as out_f:
        # Magic + file count
        out_f.write(b'NRQB')
        out_f.write(struct.pack('<I', len(files_to_pack)))
        
        for rel_path, full_path, size in files_to_pack:
            path_bytes = rel_path.encode('utf-8')
            out_f.write(struct.pack('<H', len(path_bytes)))
            out_f.write(path_bytes)
            out_f.write(struct.pack('<I', size))
            
            with open(full_path, 'rb') as in_f:
                # Copy in 1MB chunks
                while True:
                    chunk = in_f.read(1024 * 1024)
                    if not chunk:
                        break
                    out_f.write(chunk)
                    
    out_size = os.path.getsize(output_bin)
    print(f"[PACK] Successfully created {output_bin} ({out_size / (1024*1024):.2f} MB)")

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)
    pkg = os.path.join(root_dir, 'dist', 'package')
    out = os.path.join(root_dir, 'dist', 'payload.bin')
    
    if len(sys.argv) > 1:
        pkg = sys.argv[1]
    if len(sys.argv) > 2:
        out = sys.argv[2]
        
    pack(pkg, out)
