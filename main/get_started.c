// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mdf_common.h"
#include "mwifi.h"
#include "esp_timer.h"

// #define MEMORY_DEBUG

static const char *TAG = "get_started";
int64_t rtt, sendTime, recvTime, total, count;


static void root_task(void *arg)
{
    mdf_err_t ret                    = MDF_OK;
    char *data                       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size                      = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0};

    MDF_LOGI("Root is running");

    for (int i = 0;; ++i) {
        if (!mwifi_is_started()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        if(ret == ESP_OK)
            MDF_LOGI("Data rcvd by root");
        else
            MDF_LOGI("Data rcv from root FAILED");

        size = sprintf(data, "(%d) Sending from root to leaf", i);
        ret = mwifi_root_write(src_addr, 1, &data_type, data, size, true);
        if(ret == ESP_OK)
            MDF_LOGI("Data sent by root");
        else
            MDF_LOGI("Data send from root FAILED");

        vTaskDelay(500 / portTICK_RATE_MS);
    }

    MDF_LOGW("Root is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}

static void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Note read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        recvTime = esp_timer_get_time();
        count++;
        if(ret == ESP_OK)
            MDF_LOGI("Data rcvd by node");
        else
            MDF_LOGI("Data rcv from node FAILED");

        rtt=recvTime - sendTime;
        total+=rtt;
        MDF_LOGI("RTT = %lld,  Avg RTT:", rtt, total/count);
    }

    MDF_LOGW("Note read task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}

void node_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    int count     = 0;
    size_t size   = 0;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    mwifi_data_type_t data_type = {0x0};

    MDF_LOGI("Node write task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = sprintf(data, "(%d) Data from node to root!", count++);
        sendTime = esp_timer_get_time();
        ret = mwifi_write(NULL, &data_type, data, size, true);
        if(ret == ESP_OK)
            MDF_LOGI("Data sent by root");
        else
            MDF_LOGI("Data send from root FAILED");

        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    MDF_LOGW("Node write task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}



static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config   = {
        .channel   = CONFIG_MESH_CHANNEL,
        .mesh_id   = CONFIG_MESH_ID,
        .mesh_type = CONFIG_DEVICE_TYPE,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Data transfer between wifi mesh devices
     */
    if (config.mesh_type == MESH_ROOT) {
        xTaskCreate(root_task, "root_task", 4 * 1024,
                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    } else {
        xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
        xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    }

    //TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,
    //                                   true, NULL, print_system_info_timercb);
    //xTimerStart(timer, 0);
}
