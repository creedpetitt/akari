#!/usr/bin/env python3
import os
import re

HEADERS_DIRS = ['include', 'vendor/picohttpparser', 'vendor/jsmn']
SOURCES_DIRS = ['src', 'vendor/picohttpparser']
IGNORE_FILES = ['main.c']
OUTPUT_FILE = 'akari.h'

def simple_process(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    
    processed = []
    for line in lines:
        if re.match(r'#include\s+"(akari_|pico|jsmn|(?:\.\./)*include/|(?:\.\./)*vendor/).*h"', line):
            continue
        processed.append(line)
        
    return "".join(processed)

def main():
    print(f"Generating {OUTPUT_FILE}...")
    
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
        content = simple_process(h)
        header_content.append(f"// --- {h} ---\n{content}\n")

    sources = [
        'src/akari_core.c',
        'src/akari_event.c',
        'src/akari_http.c',
        'vendor/picohttpparser/picohttpparser.c'
    ]
    for s in sources:
        if not os.path.exists(s): continue
        content = simple_process(s)
        source_content.append(f"// --- {s} ---\n{content}\n")

    # Handle the event engines with platform guards
    source_content.append("\n// --- PLATFORM SPECIFIC EVENT ENGINES ---\n")
    
    epoll_content = simple_process('src/akari_event_epoll.c')
    source_content.append("#ifndef AKARI_USE_POLL\n")
    source_content.append(epoll_content)
    source_content.append("#endif // AKARI_USE_POLL\n\n")

    poll_content = simple_process('src/akari_event_poll.c')
    source_content.append("#ifdef AKARI_USE_POLL\n")
    source_content.append(poll_content)
    source_content.append("#endif // AKARI_USE_POLL\n\n")

    with open(OUTPUT_FILE, 'w') as out:
        out.write("// Akari Single-Header Web Server Library\n")
        out.write("// Generated automatically.\n\n")
        
        out.write("#ifndef AKARI_SINGLE_HEADER_H\n")
        out.write("#define AKARI_SINGLE_HEADER_H\n\n")
        
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
