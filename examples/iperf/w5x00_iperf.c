/**
 * Copyright (c) 2021 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "socket.h"
#include "timer.h"

#include "cJSON.h" // JSON handling library
#include "iperf.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
#define PLL_SYS_KHZ (133 * 1000)
// #define PLL_SYS_KHZ (90 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 8)

/* Socket */
#define SOCKET_CTRL 0
#define SOCKET_DATA 1

/* Port */
// #define PORT_IPERF 5002
#define PORT_IPERF 5201     //iperf3

#define MAX_RESULT_LEN 1024

/* Cookie size */
#define COOKIE_SIZE 37

/* iperf3 Commands */
#define PARAM_EXCHANGE 9
#define CREATE_STREAMS 10
#define TEST_START 1
#define TEST_RUNNING 2
#define TEST_END 4
#define EXCHANGE_RESULTS 13
#define DISPLAY_RESULTS 14
#define IPERF_DONE 16

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 102},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

/* iperf */
static uint8_t g_iperf_buf[ETHERNET_BUF_MAX_SIZE * 2] = {
    0,
};
static uint8_t cookie[COOKIE_SIZE] = {0};


/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);
void handle_param_exchange(uint8_t socket_ctrl, bool *reverse, bool *udp);
void handle_create_streams(uint8_t socket_ctrl, bool udp);
void start_iperf_test(uint8_t socket_ctrl, uint8_t socket_data, Stats *stats, bool reverse, bool udp);
void exchange_results(uint8_t socket_ctrl, Stats *stats);

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    int retval = 0;
    bool reverse = false;
    bool udp = false;
    uint8_t socket_status;
    uint16_t received_len;
    uint32_t pack_len = 0;
    Stats stats;

    set_clock_khz();

    stdio_init_all();

    wizchip_spi_initialize((PLL_SYS_KHZ / 4) * 1000);
    // wizchip_spi_initialize((PLL_SYS_KHZ / 2) * 1000);
    // wizchip_spi_initialize();
    // wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    network_initialize(g_net_info);

    /* Get network information */
    print_network_information(g_net_info);

    socket(SOCKET_CTRL, Sn_MR_TCP, PORT_IPERF, 0);
    listen(SOCKET_CTRL);

    while (1)
    {
        stats_init(&stats, 1000);

        socket_status = getSn_SR(SOCKET_CTRL);

        if (socket_status == SOCK_ESTABLISHED) {

            handle_param_exchange(SOCKET_CTRL, &reverse, &udp);
            handle_create_streams(SOCKET_CTRL, udp);
            start_iperf_test(SOCKET_CTRL, SOCKET_DATA, &stats, reverse, udp);

            disconnect(SOCKET_DATA);
            disconnect(SOCKET_CTRL);
        } else if (socket_status == SOCK_CLOSE_WAIT) {
            disconnect(SOCKET_CTRL);
        } else if (socket_status == SOCK_CLOSED) {
            socket(SOCKET_CTRL, Sn_MR_TCP, PORT_IPERF, 0);
            listen(SOCKET_CTRL);
        }
    }
}

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

void handle_param_exchange(uint8_t socket_ctrl, bool *reverse, bool *udp) 
{
    char buffer[512] = {0};
    uint8_t cmd;
    uint16_t len = 0;
    uint8_t raw_len[4] = {0};
    int cookie_len;
    cJSON *json;
    cJSON *reverseItem;
    cJSON *udpItem;

    cookie_len = recv(socket_ctrl, cookie, COOKIE_SIZE);
    if (cookie_len != COOKIE_SIZE) {
        printf("[iperf] Failed to receive cookie. Received: %d bytes\n", cookie_len);
        return;
    }
    printf("[iperf] Received cookie: %s\n", cookie);

    cmd = PARAM_EXCHANGE;
    send(socket_ctrl, &cmd, 1);
    recv(socket_ctrl, raw_len, 4);

    len = (raw_len[0] << 24) | (raw_len[1] << 16) | (raw_len[2] << 8) | raw_len[3];
    printf("[iperf] Raw length bytes: 0x%02X 0x%02X 0x%02X 0x%02X, Parsed length: %d\n",
           raw_len[0], raw_len[1], raw_len[2], raw_len[3], len);

    recv(socket_ctrl, (uint8_t *)buffer, len);
    buffer[len] = '\0'; // Null-terminate

    printf("[iperf] Received parameters: %s\n", buffer);

    json = cJSON_Parse(buffer);
    if (json == NULL) {
        printf("[iperf] Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
    } else {
        printf("[iperf] Parsed JSON: %s\n", cJSON_Print(json));
        reverseItem = cJSON_GetObjectItem(json, "reverse");
        udpItem = cJSON_GetObjectItem(json, "udp");

        *reverse = (reverseItem && cJSON_IsBool(reverseItem)) ? reverseItem->valueint : 0;
        *udp = (udpItem && cJSON_IsBool(udpItem)) ? udpItem->valueint : 0;
        cJSON_Delete(json);
        printf("[iperf] Parsed JSON: reverse=%d, udp=%d\n", *reverse, *udp);
    }
}

void handle_create_streams(uint8_t socket_ctrl, bool udp) 
{
    uint8_t cmd = CREATE_STREAMS;
    uint8_t received;

    send(socket_ctrl, &cmd, 1);
    printf("[iperf] Sent CREATE_STREAMS command.\n");

    socket(SOCKET_DATA, Sn_MR_TCP, PORT_IPERF, 0);
    listen(SOCKET_DATA);

    // Wait for client to connect to data socket
    while (getSn_SR(SOCKET_DATA) != SOCK_ESTABLISHED) {
        if (getSn_SR(SOCKET_DATA) == SOCK_CLOSED) {
            printf("[iperf] Data socket closed unexpectedly.\n");
            return;
        }
    }
    printf("[iperf] Data connection established.\n");

    // Receive cookie on data socket
    received = recv(SOCKET_DATA, cookie, COOKIE_SIZE);
    if (received > 0) {
        printf("[iperf] Received data cookie: %s\n", cookie);
    }
}

void start_iperf_test(uint8_t socket_ctrl, uint8_t socket_data, Stats *stats, bool reverse, bool udp)
{
    bool running = true;
    uint8_t cmd = 0;
    uint32_t total_bytes = 0;
    uint32_t pack_len = 0;

    printf("[iperf] Starting data stream test...\n");

    // Start test
    cmd = TEST_START;
    send(socket_ctrl, &cmd, 1);

    // Running test
    cmd = TEST_RUNNING;
    send(socket_ctrl, &cmd, 1);

    stats_start(stats);

    while (running) {
        if (getSn_RX_RSR(SOCKET_CTRL) > 0) {
            recv(SOCKET_CTRL, &cmd, 1);
            if (cmd == TEST_END) {
                printf("[iperf] TEST_END command received. Stopping test...\n");
                running = false;
                break;
            }
        }

        if (reverse) {
            memset(g_iperf_buf, 0xAA, ETHERNET_BUF_MAX_SIZE / 2);
            send(socket_data, g_iperf_buf, ETHERNET_BUF_MAX_SIZE / 2);
            stats_add_bytes(stats, ETHERNET_BUF_MAX_SIZE / 2);
        } else {
            getsockopt(socket_data, SO_RECVBUF, &pack_len);
            if (pack_len > 0)
            {
                recv_iperf(socket_data, (uint8_t *)g_iperf_buf, ETHERNET_BUF_MAX_SIZE / 2);
                stats_add_bytes(stats, ETHERNET_BUF_MAX_SIZE / 2);
            }
            else if (pack_len == 0) {
                stats_update(stats, false);
            } else {
                printf("[iperf] Error during data reception\n");
                break;
            }
        }
        stats_update(stats, false);
    }
    stats_stop(stats);

    exchange_results(SOCKET_CTRL, stats);
}

void exchange_results(uint8_t socket_ctrl, Stats *stats) 
{
    uint8_t cmd = EXCHANGE_RESULTS;
    uint32_t result_len = 0;
    uint8_t length_bytes[4];
    char buffer[1024];
    char *results_str;
    uint32_t results_len;
    cJSON *results;
    cJSON *streams;
    cJSON *stream;

    // Ask to exchange results
    send(socket_ctrl, &cmd, 1);
    printf("[iperf] Sent EXCHANGE_RESULTS command.\n");

    // Receive client results
    recv(socket_ctrl, (uint8_t *)&result_len, 4);
    result_len = (result_len << 24) | ((result_len << 8) & 0x00FF0000) | ((result_len >> 8) & 0x0000FF00) | (result_len >> 24); // Convert to host-endian

    if (result_len > sizeof(buffer)) {
        printf("[iperf] Received result length exceeds buffer size.\n");
        return;
    }

    recv(socket_ctrl, (uint8_t *)buffer, result_len);
    buffer[result_len] = '\0'; // Null-terminate the received JSON data
    printf("[iperf] Client results received: %s\n", buffer);

    // Prepare server results
    results = cJSON_CreateObject();
    cJSON_AddNumberToObject(results, "cpu_util_total", 1);
    cJSON_AddNumberToObject(results, "cpu_util_user", 0.5);
    cJSON_AddNumberToObject(results, "cpu_util_system", 0.5);
    cJSON_AddNumberToObject(results, "sender_has_retransmits", 1);
    cJSON_AddStringToObject(results, "congestion_used", "cubic");

    // Streams object
    streams = cJSON_CreateArray();
    stream = cJSON_CreateObject();
    cJSON_AddNumberToObject(stream, "id", 1);
    cJSON_AddNumberToObject(stream, "bytes", stats->nb0);
    cJSON_AddNumberToObject(stream, "retransmits", 0);
    cJSON_AddNumberToObject(stream, "jitter", 0);
    cJSON_AddNumberToObject(stream, "errors", 0);
    cJSON_AddNumberToObject(stream, "packets", stats->np0);  // 총 패킷 수 추가
    cJSON_AddNumberToObject(stream, "start_time", 0);
    cJSON_AddNumberToObject(stream, "end_time", (double)(stats->t3 - stats->t0) / 1000000.0);  // 종료 시간 계산
    cJSON_AddItemToArray(streams, stream);
    cJSON_AddItemToObject(results, "streams", streams);

    // Serialize JSON to string
    results_str = cJSON_PrintUnformatted(results);
    results_len = strlen(results_str);

    // Send server results
    length_bytes[0] = (results_len >> 24) & 0xFF;
    length_bytes[1] = (results_len >> 16) & 0xFF;
    length_bytes[2] = (results_len >> 8) & 0xFF;
    length_bytes[3] = results_len & 0xFF;

    send(socket_ctrl, length_bytes, 4);
    send(socket_ctrl, (uint8_t *)results_str, results_len);

    printf("[iperf] Server results sent.\n");

    cJSON_Delete(results);

    // Ask to display results
    cmd = DISPLAY_RESULTS;
    send(socket_ctrl, &cmd, 1);

    // Wait for IPERF_DONE command
    recv(socket_ctrl, &cmd, 1);
    if (cmd == IPERF_DONE) {
        printf("[iperf] Test completed successfully.\n");
    } else {
        printf("[iperf] Unexpected command received: %d\n", cmd);
    }
}