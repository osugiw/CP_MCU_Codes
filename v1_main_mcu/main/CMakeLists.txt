idf_component_register(
    REQUIRES fatfs spi_flash bt nvs_flash esp_http_client
    INCLUDE_DIRS 
                "."
                "./sd_card"
                "./ble_conn"
                "./wifi"
                "./http_handle"
    SRC_DIRS     
                "."
                "./sd_card"
                "./ble_conn"
                "./wifi"
                "./http_handle"
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