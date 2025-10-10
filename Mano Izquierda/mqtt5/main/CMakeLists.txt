idf_component_register(
    SRCS "main.c" 
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_lvgl_port esp_lcd nvs_flash esp_netif esp_event mqtt driver protocol_examples_common esp_adc
)