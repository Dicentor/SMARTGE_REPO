#include "time_sync.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctime>

static const char* TAG = "time_sync";
static volatile bool s_time_set = false;

static void sntp_cb(struct timeval *tv) {
    s_time_set = true;
}

void time_sync_init() {
    s_time_set = false;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)"pool.ntp.org");
    sntp_set_time_sync_notification_cb(sntp_cb);
    sntp_init();

    // bis zu 10s auf Zeit warten
    for (int i=0; i<100 && !s_time_set; ++i) vTaskDelay(pdMS_TO_TICKS(100));

    time_t now = 0; time(&now);
    if (now > 1600000000) {
        s_time_set = true;
        ESP_LOGI(TAG, "Time synced.");
    } else {
        ESP_LOGW(TAG, "Time not set yet.");
    }
}

bool time_is_set() { return s_time_set; }
