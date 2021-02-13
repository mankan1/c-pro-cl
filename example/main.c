/**
 * Copyright 2019 DigitalOcean Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <microhttpd.h>
#include <prom.h>
#include <promhttp.h>

#include "foo.h"
#include "bar.h"

prom_histogram_t *test_histogram;
struct MHD_Daemon *prom_daemon;

static void init(void) {
    // Initialize the Default registry
    prom_collector_registry_default_init();

    // Register file-based metrics for each file
    foo_init();
    bar_init();

    test_histogram = prom_collector_registry_must_register_metric(
        prom_histogram_new(
        "test_histogram",
        "histogram under test",
        prom_histogram_buckets_exponential(1, 1.3, 60),
        0,
        NULL
        )
    );

    // Set the active registry for the HTTP handler (NULL means use the built-in default)
    promhttp_set_active_collector_registry(NULL);
}

void intHandler(int signal) {
    printf("\nshutting down...\n");
    fflush(stdout);
    prom_collector_registry_destroy(PROM_COLLECTOR_REGISTRY_DEFAULT);
    MHD_stop_daemon(prom_daemon);
}

int main(int argc, const char **argv) {
    init();
    const char *labels[] = { "one", "two", "three", "four", "five" };

    prom_daemon = promhttp_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, 8489, NULL, NULL);
    if (prom_daemon == NULL) {
        return 1;
    }

    signal(SIGINT, intHandler);
    unsigned long long prev = prom_get_clock_ns();
    int cnt = 0;
    int r = 0;
    while(1) {
        usleep(10*1000);
        unsigned long long now = prom_get_clock_ns();
        unsigned long long elapsed_usec = (now - prev) / 1000;
        prev = now;
        r = prom_histogram_observe(test_histogram, elapsed_usec, NULL);
        if (r) exit(r);
        r = foo(cnt, labels[cnt%5]);
        if (r) exit(r);
        r = bar(cnt + elapsed_usec, labels[cnt%5]);
        if (r) exit(r);
    }

    return 0;
}
