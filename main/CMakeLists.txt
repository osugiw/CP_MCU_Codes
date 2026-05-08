idf_component_register(
    REQUIRES 
                fatfs 
                spi_flash 
                bt 
                nvs_flash 
                esp_http_client
                esp_driver_i2s
                esp-tls
    
    PRIV_REQUIRES   spiffs
                    esp_driver_rmt

    INCLUDE_DIRS 
                "."
                "./sd_card"
                "./ble_conn"
                "./wifi"
                "./http_handle"
                "./mic_pdm"
                "./NTP"
                "./buttons"
                "./led"
                "./spiffs"
    SRC_DIRS     
                "."
                "./sd_card"
                "./ble_conn"
                "./wifi"
                "./http_handle"
                "./mic_pdm"
                "./NTP"
                "./buttons"
                "./led"
                "./spiffs"
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