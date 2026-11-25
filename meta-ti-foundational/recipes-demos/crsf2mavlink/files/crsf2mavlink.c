/*
 * crsf2mavlink.c
 *
 * Reads CRSF RC frames from a serial UART (e.g. /dev/ttyS2 at 420000 baud)
 * and forwards them as MAVLink v2 RC_CHANNELS_OVERRIDE UDP datagrams to
 * mavlink-router on localhost:14540.
 *
 * Usage:
 *   crsf2mavlink [-d <device>] [-b <baud>] [-h <host>] [-p <port>]
 *
 * Defaults come from /etc/crsf2mavlink/crsf2mavlink.conf (KEY=VALUE lines).
 * CLI arguments override config file values.
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Texas Instruments Incorporated
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <syslog.h>
#include <getopt.h>
#include <sys/select.h>

#include "crsf.h"
#include "mavlink_bridge.h"

/* -------------------------------------------------------------------------
 * Defaults (overridden by config file / CLI)
 * ---------------------------------------------------------------------- */
#define DEFAULT_DEVICE    "/dev/ttyS2"
#define DEFAULT_BAUD      420000
#define DEFAULT_HOST      "127.0.0.1"
#define DEFAULT_PORT      14540
#define DEFAULT_CONF      "/etc/crsf2mavlink/crsf2mavlink.conf"

/* -------------------------------------------------------------------------
 * Config
 * ---------------------------------------------------------------------- */
typedef struct {
    char     device[64];
    int      baud;
    char     host[48];
    uint16_t port;
} config_t;

static void config_defaults(config_t *c)
{
    strncpy(c->device, DEFAULT_DEVICE, sizeof(c->device) - 1);
    c->baud = DEFAULT_BAUD;
    strncpy(c->host, DEFAULT_HOST, sizeof(c->host) - 1);
    c->port = DEFAULT_PORT;
}

static void config_load(config_t *c, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        /* strip newline */
        line[strcspn(line, "\r\n")] = '\0';
        /* skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\0') continue;

        char key[64], val[64];
        if (sscanf(line, "%63[^=]=%63s", key, val) != 2) continue;

        if      (strcmp(key, "DEVICE") == 0) strncpy(c->device, val, sizeof(c->device) - 1);
        else if (strcmp(key, "BAUD")   == 0) c->baud = atoi(val);
        else if (strcmp(key, "HOST")   == 0) strncpy(c->host, val, sizeof(c->host) - 1);
        else if (strcmp(key, "PORT")   == 0) c->port = (uint16_t)atoi(val);
    }
    fclose(f);
}

/* -------------------------------------------------------------------------
 * Serial port
 * ---------------------------------------------------------------------- */
static speed_t baud_to_speed(int baud)
{
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    case 460800: return B460800;
    case 921600: return B921600;
    /* 420000 is non-standard; use BOTHER via cfsetspeed on Linux */
    default:     return (speed_t)baud;
    }
}

static int serial_open(const char *dev, int baud)
{
    int fd = open(dev, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return -1;

    struct termios t;
    if (tcgetattr(fd, &t) < 0) { close(fd); return -1; }

    cfmakeraw(&t);
    t.c_cflag |= (CLOCAL | CREAD);
    t.c_cflag &= ~CRTSCTS;
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;

    speed_t spd = baud_to_speed(baud);
    cfsetispeed(&t, spd);
    cfsetospeed(&t, spd);

    if (tcsetattr(fd, TCSANOW, &t) < 0) { close(fd); return -1; }
    return fd;
}

/* -------------------------------------------------------------------------
 * Signal handling
 * ---------------------------------------------------------------------- */
static volatile sig_atomic_t g_running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* -------------------------------------------------------------------------
 * Main loop
 * ---------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    config_t cfg;
    config_defaults(&cfg);
    config_load(&cfg, DEFAULT_CONF);

    /* CLI overrides */
    int opt;
    while ((opt = getopt(argc, argv, "d:b:h:p:")) != -1) {
        switch (opt) {
        case 'd': strncpy(cfg.device, optarg, sizeof(cfg.device) - 1); break;
        case 'b': cfg.baud = atoi(optarg); break;
        case 'h': strncpy(cfg.host, optarg, sizeof(cfg.host) - 1); break;
        case 'p': cfg.port = (uint16_t)atoi(optarg); break;
        default:
            fprintf(stderr,
                "Usage: %s [-d device] [-b baud] [-h host] [-p port]\n",
                argv[0]);
            return 1;
        }
    }

    openlog("crsf2mavlink", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "starting: device=%s baud=%d dst=%s:%u",
           cfg.device, cfg.baud, cfg.host, cfg.port);

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    int serial_fd = serial_open(cfg.device, cfg.baud);
    if (serial_fd < 0) {
        syslog(LOG_ERR, "cannot open %s: %s", cfg.device, strerror(errno));
        closelog();
        return 1;
    }

    mavlink_udp_t mav;
    if (mavlink_udp_init(&mav, cfg.host, cfg.port) < 0) {
        syslog(LOG_ERR, "cannot create UDP socket: %s", strerror(errno));
        close(serial_fd);
        closelog();
        return 1;
    }

    uint8_t       rxbuf[256];
    crsf_parser_t parser = {.len = 0};
    crsf_channels_t chans;
    uint16_t      chan_us[CRSF_RC_CHANNEL_COUNT];

    while (g_running) {
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(serial_fd, &rset);
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

        int r = select(serial_fd + 1, &rset, NULL, NULL, &tv);
        if (r < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "select: %s", strerror(errno));
            break;
        }
        if (r == 0) continue; /* timeout */

        ssize_t n = read(serial_fd, rxbuf, sizeof(rxbuf));
        if (n <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            syslog(LOG_ERR, "read: %s", strerror(errno));
            break;
        }

        for (ssize_t i = 0; i < n; i++) {
            uint8_t byte = rxbuf[i];

            /* Re-sync on SYNC byte if parser is empty */
            if (parser.len == 0 && byte != CRSF_SYNC_BYTE)
                continue;

            if (parser.len < (int)sizeof(parser.buf))
                parser.buf[parser.len++] = byte;

            /* Need at least SYNC + LEN before we know frame size */
            if (parser.len < 2) continue;

            int expected = parser.buf[1] + CRSF_HEADER_LEN;
            if (parser.len < expected) continue;

            /* Full frame received - try to parse RC channels */
            if (crsf_parse_rc(parser.buf, parser.len, &chans)) {
                for (int c = 0; c < CRSF_RC_CHANNEL_COUNT; c++)
                    chan_us[c] = crsf_to_us(chans.chan[c]);

                if (mavlink_send_rc_override(&mav, chan_us,
                                             CRSF_RC_CHANNEL_COUNT) < 0)
                    syslog(LOG_WARNING, "sendto: %s", strerror(errno));
            }

            /* Reset parser for next frame */
            parser.len = 0;
        }
    }

    syslog(LOG_INFO, "stopping");
    close(serial_fd);
    close(mav.fd);
    closelog();
    return 0;
}
