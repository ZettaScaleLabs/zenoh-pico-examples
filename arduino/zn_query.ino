/*
 * Copyright (c) 2017, 2021 ADLINK Technology Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
 * which is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
 *
 * Contributors:
 *   ADLINK zenoh team, <zenoh@adlink-labs.tech>
 */

#include <Arduino.h>
#include <WiFi.h>

#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"

extern "C"
{
    #include <zenoh-pico.h>
}

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

zn_session_t *zs = NULL;

void setup()
{
    Serial.begin(115200);
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("OK!");

    Serial.print("Openning Zenoh Session...");
    zn_properties_t *config = zn_config_default();
    zn_properties_insert(config, ZN_CONFIG_MODE_KEY, z_string_make(Z_MODE));
#if Z_SCOUT == 0
    zn_properties_insert(config, ZN_CONFIG_PEER_KEY, z_string_make(Z_PEER));
#endif
    zs = zn_open(config);
    if (zs == NULL)
    {
        Serial.println("FAIL!");
        while(true);
    }
    Serial.println("OK!");

    Serial.print("Starting Zenoh tasks (read and lease tasks)...");
    znp_start_read_task(zs);
    znp_start_lease_task(zs);
    Serial.println("OK!");

    delay(5000);
}

void loop()
{
    delay(1000);

    if (zs == NULL)
        return;

    Serial.print("Sending query '");
    Serial.print(Z_URI);
    Serial.println("'...");
    zn_reply_data_array_t replies = zn_query_collect(zs, zn_rname(Z_URI), "", zn_query_target_default(), zn_query_consolidation_default());
    for (unsigned int i = 0; i < replies.len; i++)
    {
        Serial.print(" >> [Reply handler] Received (");
        for (int j = 0; j < replies.val[i].data.key.len; j++)
            Serial.print(replies.val[i].data.key.val[j]);
        Serial.print(", ");
        for (int j = 0; j < replies.val[i].data.value.len; j++)
            Serial.print((char)replies.val[i].data.value.val[j]);
        Serial.println(")");
    }
    zn_reply_data_array_free(replies);
}
