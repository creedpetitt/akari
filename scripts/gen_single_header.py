#!/usr/bin/env python3
import os
import re

HEADERS_DIRS = ['include', 'vendor/picohttpparser', 'vendor/jsmn']
SOURCES_DIRS = ['src', 'vendor/picohttpparser']
IGNORE_FILES = ['main.c']
OUTPUT_FILE = 'akari.h'

def get_files(dirs, extensions):
    files = []
    for d in dirs:
        if not os.path.exists(d): continue
        for f in os.listdir(d):
            if f.endswith(extensions) and f not in IGNORE_FILES:
                files.append(os.path.join(d, f))
    return files

def process_file(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    
    processed = []
    system_includes = set()
    
    for line in lines:
        if re.match(r'#include\s+"(akari_|pico|jsmn).*h"', line) or \
           re.match(r'#include\s+"\.\./include/akari_.*h"', line) or \
           re.match(r'#include\s+"\.\./vendor/.*h"', line):
            continue
        if re.match(r'#include\s+<.*>', line):
            system_includes.add(line.strip())
            continue
        if re.match(r'#ifndef\s+AKARI_.*_H', line) or \
           re.match(r'#define\s+AKARI_.*_H', line) or \
           re.match(r'#endif\s+//\s+AKARI_.*_H', line) or \
           re.match(r'#endif\s+/\*\s+AKARI_.*_H', line) or \
           re.match(r'#endif\s*$', line.strip()): 
            pass
        elif re.match(r'#pragma\s+once', line):
            continue
        else:
            processed.append(line)
        
    return "".join(processed), system_includes

def simple_process(path, is_header):
    with open(path, 'r') as f:
        content = f.read()
    
    # Remove internal includes
    content = re.sub(r'#include\s+"(akari_|pico|jsmn|(?:\.\./)*include/|(?:\.\./)*vendor/).*h"', '', content)
    
    # Extract system includes
    sys_inc = set(re.findall(r'#include\s+<.*>', content))
    content = re.sub(r'#include\s+<.*>', '', content)
    
    return content, sys_inc

def main():
    print(f"Generating {OUTPUT_FILE}...")
    
    all_system_includes = set()
    header_content = []
    source_content = []

    headers = [
        'include/akari_core.h',
        'include/akari_event.h',
        'include/akari_internal.h',
        'include/akari_http.h',
        'vendor/picohttpparser/picohttpparser.h',
        'vendor/jsmn/jsmn.h'
    ]
    
    for h in headers:
        if not os.path.exists(h): continue
        content, sys_inc = simple_process(h, True)
        all_system_includes.update(sys_inc)
        header_content.append(f"// --- {h} ---\n{content}\n")

    sources = [
        'src/akari_core.c',
        'src/akari_event.c',
        'src/akari_event_epoll.c',
        'src/akari_event_poll.c',
        'src/akari_http.c',
        'vendor/picohttpparser/picohttpparser.c'
    ]
    for s in sources:
        if not os.path.exists(s): continue
        content, sys_inc = simple_process(s, False)
        all_system_includes.update(sys_inc)
        source_content.append(f"// --- {s} ---\n{content}\n")

    with open(OUTPUT_FILE, 'w') as out:
        out.write("// Akari Single-Header Web Server Library\n")
        out.write("// Generated automatically.\n\n")
        
        out.write("#ifndef AKARI_SINGLE_HEADER_H\n")
        out.write("#define AKARI_SINGLE_HEADER_H\n\n")
        
        for inc in sorted(list(all_system_includes)):
            out.write(f"{inc}\n")
        out.write("\n")
        
        out.write("".join(header_content))
        out.write("#endif // AKARI_SINGLE_HEADER_H\n\n")
        
        out.write("#ifdef AKARI_IMPLEMENTATION\n\n")
        out.write("".join(source_content))
        out.write("#endif // AKARI_IMPLEMENTATION\n")

    print(f"Successfully generated {OUTPUT_FILE}!")

    print("Generating tools/akari_lib.h for CLI embedding...")
    with open(OUTPUT_FILE, 'rb') as f:
        data = f.read()
    
    with open('tools/akari_lib.h', 'w') as out:
        out.write("#ifndef AKARI_LIB_H\n#define AKARI_LIB_H\n\n")
        out.write(f"unsigned char AKARI_LIB_DATA[] = {{\n    ")
        for i, b in enumerate(data):
            out.write(f"0x{b:02x}, ")
            if (i + 1) % 12 == 0:
                out.write("\n    ")
        out.write("\n};\n")
        out.write(f"unsigned int AKARI_LIB_LEN = {len(data)};\n\n")
        out.write("#endif\n")
    print("Successfully generated tools/akari_lib.h!")

if __name__ == "__main__":
    main()
