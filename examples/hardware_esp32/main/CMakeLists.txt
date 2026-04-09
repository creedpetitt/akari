idf_component_register(SRCS "main.c" "wifi.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "index.html")
                    
target_compile_definitions(${COMPONENT_LIB} PUBLIC AKARI_RES_BUF_SIZE=2048)
