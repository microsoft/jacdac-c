if (NOT DEFINED JACDAC_USER_CONFIG_DIR)
    set(JACDAC_USER_CONFIG_DIR "..")
endif()

file(GLOB JDC_FILES 
  jacscript/*.c
  services/*.c
  source/*.c
  source/interfaces/*.c
  client/*.c
  storage/*.c
  storage/ff/*.c
  network/*.c

  jacdac/dist/c/jd_spec_pack.c
)

add_library(jacdac STATIC
    ${JDC_FILES}
)

target_include_directories(jacdac PUBLIC 
    ./inc
    ./jacscript
    .
    ${JACDAC_USER_CONFIG_DIR}
)
