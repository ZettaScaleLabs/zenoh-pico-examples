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
    #error "PULL mode is not yet implemented for peer mode in Zenoh-Pico."
#else
    #error "Unknown Zenoh operation mode. Check Z_CLIENT_OR_PEER value."
#endif

#define Z_URI "/demo/example/**"

zn_session_t *zs = NULL;
zn_subscriber_t *sub = NULL;

void data_handler(const zn_sample_t *sample, const void *arg)
{
    Serial.print(" >> [Subscription listener] Received (");
    for (int i = 0; i < sample->key.len; i++)
        Serial.print(sample->key.val[i]);
    Serial.print(", ");
    for (int i = 0; i < sample->value.len; i++)
        Serial.print((char)sample->value.val[i]);
    Serial.println(")");
}

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

    Serial.print("Declaring subscriber for '");
    Serial.print(Z_URI);
    Serial.print("'...");
    zn_subinfo_t subinfo;
    subinfo.reliability = zn_reliability_t_RELIABLE;
    subinfo.mode = zn_submode_t_PULL;
    subinfo.period = NULL;
    sub = zn_declare_subscriber(zs, zn_rname(Z_URI), subinfo, data_handler, NULL);
    if (sub == NULL)
    {
        Serial.println("FAIL!");
        while(true);
    }
    Serial.println("OK!");

    delay(5000);
}

void loop()
{
    if (zs == NULL || sub == NULL)
        return;

    delay(5000);

    Serial.print("Pulling subscription for '");
    Serial.print(Z_URI);
    Serial.println("'...");
    zn_pull(sub);
}
