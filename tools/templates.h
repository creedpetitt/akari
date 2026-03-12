#ifndef AKARI_TEMPLATES_H
#define AKARI_TEMPLATES_H

const char* INDEX_HTML_TEMPLATE = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>Akari Server</title>\n"
"    <style>\n"
"        body { font-family: sans-serif; text-align: center; padding: 50px; }\n"
"        h1 { color: #ff4500; }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <h1>🏮 Akari Web Server</h1>\n"
"    <p>Your zero-allocation C server is running perfectly.</p>\n"
"</body>\n"
"</html>\n";

const char* MAIN_C_TEMPLATE = 
"#define AKARI_IMPLEMENTATION\n"
"#include \"akari.h\"\n"
"#include <stdio.h>\n"
"#include <signal.h>\n"
"\n"
"void handle_signal(int sig) {\n"
"    (void)sig;\n"
"    printf(\"\\nShutting down Akari...\\n\");\n"
"    akari_stop();\n"
"}\n"
"\n"
"void handle_home(akari_context* ctx) {\n"
"    akari_res_file(ctx, \"index.html\");\n"
"}\n"
"\n"
"int main() {\n"
"    signal(SIGINT, handle_signal);\n"
"    printf(\"🏮 Akari starting on port 8080...\\n\");\n"
"    \n"
"    AKARI_GET(\"/\", handle_home);\n"
"    \n"
"    akari_http_start(8080);\n"
"    return 0;\n"
"}\n";

const char* MAKEFILE_TEMPLATE = 
"all:\n"
"	gcc -O2 main.c -o app\n"
"clean:\n"
"	rm -f app\n";

#endif
