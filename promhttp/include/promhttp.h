/*
 Copyright 2019-2020 DigitalOcean Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * @file promhttp.h
 * @brief Provides a HTTP endpoint for metric exposition
 * References:
 *   * MHD_FLAG: https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dconst
 *   * MHD_AcceptPolicyCallback:
 * https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#index-_002aMHD_005fAcceptPolicyCallback
 */

#include <curl/curl.h>
#include <string.h>
#include <pthread.h>

#include "microhttpd.h"
#include "prom_collector_registry.h"

/**
 * @brief Sets the active registry for metric scraping.
 *
 * @param active_registry The target prom_collector_registry_t*. If null is passed, the default registry is used.
 *                        The registry MUST be initialized.
 */
void promhttp_set_active_collector_registry(prom_collector_registry_t *active_registry);

/**
 *  @brief Starts a daemon in the background and returns a pointer to an HMD_Daemon.
 *
 * References:
 *  * https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dinit
 *
 * @return struct MHD_Daemon*
 */
struct MHD_Daemon *promhttp_start_daemon(unsigned int flags, unsigned short port, MHD_AcceptPolicyCallback apc,
                                         void *apc_cls);

/**
 * @brief this is a handle to a background timer thread which pushes metrics to
 * a push-gateway.
 */
struct promhttp_push_handle;
typedef struct promhttp_push_handle promhttp_push_handle_t;

/**
 * @brief Stops and waits for exit of a previously started push timer.
 */
void
promhttp_stop_push_metrics(promhttp_push_handle_t *handle);

/**
 * @brief Starts a background timer thread to push metrics to the given URL with the given.
 * push interval
 * The registry collector must be initialized via promhttp_set_active_collector_registry
 * before this call
 *
 * @return struct promhttp_push_handle_t* which is a handle to the background thread,
 * or NULL on error
 */
promhttp_push_handle_t *
promhttp_start_push_metrics(const char *url, unsigned long long interval_ms);
