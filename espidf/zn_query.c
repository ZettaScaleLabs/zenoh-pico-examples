/*
 * Copyright (c) 2017, 2022 ZettaScale Technology SARL.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ZettaScale zenoh team, <zenoh@zettascale.tech>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#define ESP_WIFI_SSID      "SSID"
#define ESP_WIFI_PASS      "PASS"
#define ESP_MAXIMUM_RETRY  5
#define WIFI_CONNECTED_BIT BIT0

static bool s_is_wifi_connected = false;
static EventGroupHandle_t s_event_group_handler;
static int s_retry_count = 0;

#include <zenoh-pico.h>

#define Z_CLIENT_OR_PEER 0 // 0: Client mode; 1: Peer mode
#define Z_SCOUT 1 // 0: Disabled; 1: Enabled

#if Z_CLIENT_OR_PEER == 0
    #define Z_MODE "client"
    #define Z_PEER "tcp/192.168.0.1:7447"
#elif Z_CLIENT_OR_PEER == 1
    #define Z_MODE "peer"
    #define Z_PEER "udp/224.0.0.225:7447#iface=en0"
#else
    #error "Unknown Zenoh operation mode. Check Z_CLIENT_OR_PEER value."
#endif

#define Z_URI "/demo/example/**"

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_count < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_count++;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_event_group_handler, WIFI_CONNECTED_BIT);
        s_retry_count = 0;
    }
}

void wifi_init_sta(void)
{
    s_event_group_handler = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    esp_event_handler_instance_t handler_any_id;
    esp_event_handler_instance_t handler_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &handler_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &handler_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_event_group_handler,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
        s_is_wifi_connected = true;

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, handler_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, handler_any_id));
    vEventGroupDelete(s_event_group_handler);
}

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

    printf(" >> [Subscription listener] Received (%.*s, %.*s)\n",
           (int)sample->key.len, sample->key.val,
           (int)sample->value.len, sample->value.val);
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("Connecting to WiFi...");
    wifi_init_sta();
    while (!s_is_wifi_connected)
    {
        printf(".");
        sleep(1);
    }
    printf("OK!\n");

    printf("Openning Zenoh Session...");
    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(Z_MODE));
#if Z_SCOUT == 0
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(Z_PEER));
#endif
    zn_session_t *zs = zn_open(config);
    if (zs == NULL)
    {
        printf("FAIL!\n");
        esp_restart();
    }
    printf("OK!\n");

    printf("Starting Zenoh tasks (read and lease tasks)...");
    znp_start_read_task(zs);
    znp_start_lease_task(zs);
    printf("OK!\n");

    sleep(5);

    printf("Sending query '%s'...\n", Z_URI);
    zn_reply_data_array_t replies = zn_query_collect(zs, zn_rname(Z_URI), "", zn_query_target_default(), zn_query_consolidation_default());

    for (unsigned int i = 0; i < replies.len; i++)
    {
        printf(" >> [Reply handler] received (%.*s, %.*s)\n",
               (int)replies.val[i].data.key.len, replies.val[i].data.key.val,
               (int)replies.val[i].data.value.len, replies.val[i].data.value.val);
    }
    zn_reply_data_array_free(replies);

    printf("Closing Zenoh Session...");
    znp_stop_read_task(zs);
    znp_stop_lease_task(zs);
    zn_close(zs);
    printf("OK!\n");
}
