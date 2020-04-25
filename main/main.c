/*
Copyright (c) 2017-2019 Tony Pottier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file main.c
@author Tony Pottier
@brief Entry point for the ESP32 application.
@see https://idyl.io
@see https://github.com/tonyp7/esp32-wifi-manager
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "wifi_manager.h"

extern struct wifi_settings_t wifi_settings;

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";
static const char WORK[] = "WORK===";

static bool pin2 = true;

/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code! This is an example on how you can integrate your code with wifi-manager
 */
void monitoring_task(void *pvParameter)
{
	for(;;){
		ESP_LOGI(TAG, "free heap: %d", esp_get_free_heap_size());
		ESP_LOGI(TAG, "GPIO0 %i", gpio_get_level(0));
		gpio_set_level(GPIO_NUM_2, (uint32_t)pin2);
		pin2 = !pin2;
		vTaskDelay( pdMS_TO_TICKS(2000) );
	}
}

void delete_config_task(void *pvParameter) {
	vTaskDelay(pdMS_TO_TICKS(2000));
	wifi_manager_disconnect_and_delete_config_async();
	UBaseType_t water = uxTaskGetStackHighWaterMark(NULL);
	ESP_LOGI(WORK, "Water line: %u", water);
	vTaskDelete( NULL );
}

/* brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event */
void cb_connection_ok(void *pvParameter){
	ESP_LOGI(WORK, "I have a connection!");
	xTaskCreate(&delete_config_task, "my_x_tsk", 2048, NULL, WIFI_MANAGER_TASK_PRIORITY + 2, NULL);
}

void cb_connection_dead(void *pvParameter) {
	ESP_LOGI(WORK, "Lost connection");
}

void app_main()
{
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	// Modify wifi settings at runtime
	//memcpy(wifi_settings.ap_ssid, "TimeCube", strlen("TimeCube") + 1);
	//wifi_settings.ap_pwd[0] = '\0';
	//wifi_settings.ap_authmode = WIFI_AUTH_OPEN;
	// Dump any saved settings
	//wifi_manager_disconnect_and_delete_config_async();

	/* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(EVENT_STA_GOT_IP, &cb_connection_ok);
	wifi_manager_set_callback(EVENT_STA_DISCONNECTED, &cb_connection_dead);

	/* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
}
