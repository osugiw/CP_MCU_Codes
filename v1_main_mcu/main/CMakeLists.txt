idf_component_register(
    REQUIRES fatfs spi_flash bt nvs_flash
    INCLUDE_DIRS 
                "."
                "./sd_card"
                "./ble_conn"
    SRC_DIRS     
                "."
                "./sd_card"
                "./ble_conn"
)

target_compile_options(
    ${COMPONENT_LIB} PRIVATE
    -Wno-missing-field-initializers 
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-literal-suffix
    -Wno-unused-but-set-parameter
    -Wno-error=switch
    -fpermissive
)