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

#include "port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "socket.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
// #define PLL_SYS_KHZ (133 * 1000)
#define PLL_SYS_KHZ (90 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 16)

/* Socket */
#define SOCKET_IPERF 0

/* Port */
#define PORT_IPERF 5001

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

/* iperf */
static uint8_t g_iperf_buf[ETHERNET_BUF_MAX_SIZE * 2] = {
    0,
};

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    int retval = 0;
    uint32_t pack_len = 0;

    set_clock_khz();

    stdio_init_all();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    network_initialize(g_net_info);

    /* Get network information */
    print_network_information(g_net_info);
    while (1)
    {
        switch(getSn_SR(SOCKET_IPERF))
        {
            case SOCK_ESTABLISHED :
                while(1)
                {

                    getsockopt(SOCKET_IPERF, SO_RECVBUF, &pack_len);
                    if (pack_len > 0)
                    {
                        recv_iperf(SOCKET_IPERF, (uint8_t *)g_iperf_buf, ETHERNET_BUF_MAX_SIZE / 2);
                    }  
                }
            case SOCK_CLOSE_WAIT :
                disconnect(SOCKET_IPERF);
                break;
            case SOCK_INIT :
                listen(SOCKET_IPERF);
                break;
            case SOCK_CLOSED:
                socket(SOCKET_IPERF, Sn_MR_TCP, PORT_IPERF, 0x00);
                break;
            default:
                break;
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
