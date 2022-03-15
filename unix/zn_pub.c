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
#include <string.h>
#include <unistd.h>

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

#define Z_URI "/demo/example/zenoh-pico-eval"
#define Z_VALUE "[UNIX]{UNIX} Eval from Zenoh-Pico!"

int main(int argc, char **argv)
{
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
        exit(-1);
    }
    printf("OK!\n");

    printf("Starting Zenoh tasks (read and lease tasks)...");
    znp_start_read_task(zs);
    znp_start_lease_task(zs);
    printf("OK!\n");

    sleep(5);

#if Z_CLIENT_OR_PEER == 0
    printf("Declaring resource '%s'", Z_URI);
    unsigned long rid = zn_declare_resource(zs, zn_rname(Z_URI));
    printf(" => RId %lu\n", rid);
    zn_reskey_t reskey = zn_rid(rid);
#else
    zn_reskey_t reskey = zn_rname(Z_URI);
#endif

    char buf[strlen(Z_VALUE) + 15];
    for (int idx = 0; idx < 20; ++idx)
    {
        sprintf(buf, "[%4d] %s", idx, Z_VALUE);
        printf("Writing Data ('%lu': '%s')...\n", reskey.rid, buf);
        zn_write(zs, reskey, (const uint8_t *)buf, strlen(buf));
        sleep(1);
    }

    printf("Closing Zenoh Session...");
    znp_stop_read_task(zs);
    znp_stop_lease_task(zs);
    zn_close(zs);
    printf("OK!\n");

    return 0;
}
