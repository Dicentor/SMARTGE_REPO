idf_component_register(
  SRCS 
    "main.cpp"
    "wifi.cpp"
    "dht22.cpp"
    "sensors.cpp"
    "relays.cpp"
    "leds.cpp"
    "firestore.cpp"
    "time_sync.cpp"     # <— hinzugefügt
  INCLUDE_DIRS "."
  REQUIRES 
    esp_wifi
    esp_event
    nvs_flash
    driver
    esp_http_client
    json
    led_strip
)
