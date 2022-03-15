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
#include <stdio.h>
#include <stdlib.h>

#include <zenoh-pico.h>

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

void data_handler(const zn_sample_t *sample, const void *arg)
{
    (void)(arg); // Unused argument

    printf(" >> [Subscription listener] Received (%.*s, %.*s)\n",
           (int)sample->key.len, sample->key.val,
           (int)sample->value.len, sample->value.val);
}

int main(int argc, char **argv)
{
    sleep(5);

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

    printf("Declaring pull subscriber on '%s'...", Z_URI);
    zn_subinfo_t subinfo;
    subinfo.reliability = zn_reliability_t_RELIABLE;
    subinfo.mode = zn_submode_t_PULL;
    subinfo.period = NULL;
    zn_subscriber_t *sub = zn_declare_subscriber(zs, zn_rname(Z_URI), subinfo, data_handler, NULL);
    if (sub == NULL)
    {
        printf("FAIL!\n");
        while(true);
    }
    printf("OK!\n");

    for (int i = 0; i < 20; i++)
    {
        sleep(5);
        printf("Pulling data from '%s'...\n", Z_URI);
        zn_pull(sub);
    }

    printf("Closing Zenoh Session...");
    znp_stop_read_task(zs);
    znp_stop_lease_task(zs);
    zn_close(zs);
    printf("OK!\n");

    return 0;
}
