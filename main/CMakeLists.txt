set(requires
    esp_driver_i2s
    )

idf_component_register(SRCS main.cpp i2s_audio.c
                       REQUIRES ${requires} edge_impluse)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error
    -Wno-format
    -Wno-unused-but-set-parameter
)