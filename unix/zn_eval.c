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

void query_handler(zn_query_t *query, const void *arg)
{
    z_string_t res = zn_query_res_name(query);
    z_string_t pred = zn_query_predicate(query);
    printf(" >> [Query handler] Replying Data ('%s': '%s')\n", query->rname, Z_VALUE);
    zn_send_reply(query, uri, (const unsigned char *)value, strlen(value));
}

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
    znp_start_read_task(s);
    znp_start_lease_task(s);
    printf("OK!\n");

    sleep(5);

    printf("Declaring queryable eval '%s'...", Z_URI);
    zn_reskey_t reskey = zn_rname(Z_URI);
    zn_queryable_t *qable = zn_declare_queryable(zs, reskey, ZN_QUERYABLE_EVAL, query_handler, NULL);
    if (qable == NULL)
    {
        printf("FAIL!\n");
        exit(-1);
    }
    printf("OK!\n");

    printf("Press 'q' to exit.\n");
    char c = 0;
    while (c != 'q')
    {
        c = fgetc(stdin);
    }

    printf("Closing Zenoh Session...");
    znp_stop_read_task(zs);
    znp_stop_lease_task(zs);
    zn_close(zs);
    printf("OK!\n");

    return 0;
}
