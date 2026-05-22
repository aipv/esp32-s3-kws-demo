set(requires
#    hardware_driver
    esp_driver_i2s
#    player
    )

idf_component_register(SRCS main.c i2s_audio.c
                       REQUIRES ${requires})

