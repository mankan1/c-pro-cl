/**
 * Copyright 2019-2020 DigitalOcean Inc.
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

#include <curl/curl.h>
#include <string.h>

#include "microhttpd.h"
#include "prom.h"
#include "promhttp.h"

struct promhttp_push_handle {
    unsigned long long interval_us;
    int should_run;
    CURL *curl;
    pthread_t thread;
    unsigned long long last_posted_ns;
};

prom_collector_registry_t *PROM_ACTIVE_REGISTRY;

void* promhttp_post_metrics(void *arg) {
    #define MAX_PROM_SLEEP 1000000
    promhttp_push_handle_t *handle = (promhttp_push_handle_t *)arg;
    while (handle->should_run) {
        unsigned long long now_ns = prom_get_clock_ns();
        unsigned long long elapsed_us = (now_ns - handle->last_posted_ns) / 1000;
        if (elapsed_us < handle->interval_us) {
            unsigned long long tosleep = handle->interval_us - elapsed_us;
            usleep(MAX_PROM_SLEEP < tosleep ? MAX_PROM_SLEEP : tosleep);
        } else {
            // time to emit metrics
            const char *buf = prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);

            int res = curl_easy_setopt(handle->curl, CURLOPT_POSTFIELDS, buf);
            if (CURLE_OK != res) {
                fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(res));
            }
            res = curl_easy_perform(handle->curl);
            prom_free((char*)buf);

            if (CURLE_OK != res) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
            handle->last_posted_ns = now_ns;
        }
    }
    return 0;
}

void promhttp_stop_push_metrics(promhttp_push_handle_t *handle) {
    if (!handle) return;
    handle->should_run = 0;
    pthread_join(handle->thread, NULL);
    curl_easy_cleanup(handle->curl);
    curl_global_cleanup();
    prom_free(handle);
}

promhttp_push_handle_t *
promhttp_start_push_metrics(const char *url, unsigned long long interval_ms) {
    if (!PROM_ACTIVE_REGISTRY) {
        fprintf(stderr, "you need to setup the active collector registry with promhttp_set_active_collector_registry()\n");
        return NULL;
    }

    promhttp_push_handle_t *res = (promhttp_push_handle_t *)prom_malloc(sizeof(promhttp_push_handle_t));
    if (!res) {
        fprintf(stderr, "bad_alloc: unable to allocate from promhttp\n");
        return NULL;
    }
    res->interval_us = interval_ms * 1000;
    res->last_posted_ns = 0;
    res->should_run = 1;
    int ccode = curl_global_init(CURL_GLOBAL_ALL);
    if (CURLE_OK != ccode) {
        prom_free(res);
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(ccode));
        return NULL;
    }
    res->curl = curl_easy_init();
    if (!res->curl) {
        curl_global_cleanup();
        prom_free(res);
        fprintf(stderr, "curl_easy_init() failed\n");
        return NULL;
    }
    ccode = curl_easy_setopt(res->curl, CURLOPT_URL, url);
    if (CURLE_OK != ccode) {
        fprintf(stderr, "curl_easy_setopt() failed: %s\n", curl_easy_strerror(ccode));

        curl_easy_cleanup(res->curl);
        curl_global_cleanup();
        prom_free(res);
        return NULL;
    }
    if (pthread_create(&res->thread, NULL, promhttp_post_metrics, res)) {
        perror("pthread create error\n");
        curl_easy_cleanup(res->curl);
        curl_global_cleanup();
        prom_free(res);
        return NULL;
    }
    return res;
}

void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry) {
    if (!active_registry) {
        PROM_ACTIVE_REGISTRY = PROM_COLLECTOR_REGISTRY_DEFAULT;
    } else {
        PROM_ACTIVE_REGISTRY = active_registry;
    }
}

int promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method,
                     const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "GET") != 0) {
        char *buf = "Invalid HTTP Method\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        return ret;
    }
    if (strcmp(url, "/") == 0) {
        char *buf = "OK\n";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    if (strcmp(url, "/metrics") == 0) {
        const char *buf = prom_collector_registry_bridge(PROM_ACTIVE_REGISTRY);
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_MUST_FREE);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }
    char *buf = "Bad Request\n";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);
    return ret;
}

struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
                                         void *apc_cls) {
    return MHD_start_daemon(flags, port, apc, apc_cls, &promhttp_handler, NULL, MHD_OPTION_END);
}
