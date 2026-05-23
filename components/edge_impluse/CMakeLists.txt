file(GLOB_RECURSE cpp_srcs
    "*.cpp"
)

file(GLOB_RECURSE cc_srcs
    "*.cc"
)

file(GLOB_RECURSE c_srcs
    "*.c"
)

set(srcs
    ${cpp_srcs}
    ${cc_srcs}
    ${c_srcs}
)

idf_component_register(
    SRCS ${srcs}

    INCLUDE_DIRS
        "."
        "edge-impulse-sdk"
        "edge-impulse-sdk/classifier"
        "edge-impulse-sdk/CMSIS/Core/Include"
        "model-parameters"

        "../../managed_components/espressif__esp-dsp/modules/support/include"
        "../../managed_components/espressif__esp-dsp/modules/fft/include"

    REQUIRES
        esp_timer
        esp-dsp
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error
    -Wno-format
    -Wno-unused-but-set-parameter
    -Wno-maybe-uninitialized
    -Wno-error=maybe-uninitialized
)
