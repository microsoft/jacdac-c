if (NOT DEFINED JACDAC_USER_CONFIG_DIR)
    set(JACDAC_USER_CONFIG_DIR "..")
endif()

file(GLOB JDC_FILES 
  services/*.c
  source/*.c
  drivers/*.c
  source/interfaces/*.c
  client/*.c
  storage/*.c
  storage/ff/*.c
)

add_library(jacdac STATIC
    ${JDC_FILES}
)

target_include_directories(jacdac PUBLIC 
    ./inc
    .
    ${JACDAC_USER_CONFIG_DIR}
)
