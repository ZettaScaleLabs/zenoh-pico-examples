//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>
//

#include <Arduino.h>
#include <WiFi.h>

extern "C" {
    #include "zenoh-pico.h"
}

// WiFi-specific parameters
#define SSID "SSID"
#define PASS "PASS"

void print_pid(z_bytes_t pid)
{
    if (pid.start == NULL) {
        Serial.print("None");
    } else {
        Serial.print("Some(");
        for (unsigned int i = 0; i < pid.len; i++) {
            Serial.print(pid.start[i], HEX);
        }
        Serial.print(")");
    }
}

void print_whatami(unsigned int whatami)
{
    if (whatami == Z_ROUTER) {
        Serial.print("'Router'");
    } else if (whatami == Z_PEER) {
        Serial.print("'Peer'");
    } else {
        Serial.print("'Other'");
    }
}

void print_locators(z_str_array_t *locs)
{
    Serial.print("[");
    size_t len = z_str_array_len(locs);
    for (unsigned int i = 0; i < len; i++) {
        Serial.print("'");
        Serial.print(*z_str_array_get(locs, i));
        Serial.print("'");
        if (i < len - 1) {
            Serial.print(", ");
        }
    }
    Serial.print("]");
}

void print_hello(z_hello_t *hello)
{
    Serial.print(" >> Hello { pid: ");
    print_pid(hello->pid);
    Serial.print(", whatami: ");
    print_whatami(hello->whatami);
    Serial.print(", locators: ");
    print_locators(&hello->locators);
    Serial.println(" }");
}

void setup()
{
    // Initialize Serial for debug
    Serial.begin(115200);
    while (!Serial) {
        delay(1000);
    }

    // Set WiFi in STA mode and trigger attachment
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println("OK");
}

void loop()
{
    z_owned_config_t config = zp_config_default();

    Serial.println("Scouting...");
    z_owned_hello_array_t hellos = z_scout(Z_ROUTER | Z_PEER, z_config_move(&config), 1);
    if (z_hello_array_len(z_hello_array_loan(&hellos)) > 0) {
        for (unsigned int i = 0; i < z_hello_array_len(z_hello_array_loan(&hellos)); ++i) {
            print_hello(z_hello_array_get(z_hello_array_loan(&hellos), i));
        }
    } else {
        Serial.println("Did not find any zenoh process.");
    }

    z_hello_array_drop(z_hello_array_move(&hellos));
}
