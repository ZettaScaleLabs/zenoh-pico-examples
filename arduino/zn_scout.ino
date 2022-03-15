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

zn_properties_t *config;

void print_pid(z_bytes_t pid)
{
    if (pid.val == NULL)
    {
        Serial.print("None");
    }
    else
    {
        Serial.print("Some(");
        for (unsigned int i = 0; i < pid.len; i++)
            Serial.print(pid.val[i], HEX);
        Serial.print(")");
    }
}

void print_whatami(unsigned int whatami)
{
    if (whatami == ZN_ROUTER)
        Serial.print("'Router'");
    else if (whatami == ZN_PEER)
        Serial.print("'Peer'");
    else
        Serial.print("'Other'");
}

void print_locators(z_str_array_t locs)
{
    Serial.print("[");
    for (unsigned int i = 0; i < locs.len; i++)
    {
        Serial.print("'");
        Serial.print(locs.val[i]);
        Serial.print("'");
        if (i < locs.len - 1)
            Serial.print(", ");
    }
    Serial.print("]");
}

void print_hello(zn_hello_t hello)
{
    Serial.print(" >> Hello { pid: ");
    print_pid(hello.pid);
    Serial.print(", whatami: ");
    print_whatami(hello.whatami);
    Serial.print(", locators: ");
    print_locators(hello.locators);
    Serial.print(" }\n");
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

    config = zn_config_default();
}

void loop()
{
    printf("Scouting...\n");
    zn_hello_array_t hellos = zn_scout(ZN_ROUTER, config, 1);
    if (hellos.len > 0)
    {
        for (size_t i = 0; i < hellos.len; i++)
        {
            print_hello(hellos.val[i]);
            fprintf(stdout, "\n");
        }

        zn_hello_array_free(hellos);
    }
    else
    {
        printf("Did not find any zenoh process.\n");
    }
    delay(5000);
}
