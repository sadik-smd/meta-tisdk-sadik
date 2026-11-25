/*
 * mavlink_bridge.h - Minimal MAVLink v2 RC_CHANNELS_OVERRIDE frame builder
 *
 * Implements only what crsf2mavlink needs: hand-encode a single
 * RC_CHANNELS_OVERRIDE (msg id 70) MAVLink v2 packet and send it over UDP.
 *
 * We avoid pulling in the full MAVLink C header library to keep the recipe
 * self-contained. The encoding matches MAVLink v2 spec exactly:
 *   https://mavlink.io/en/guide/serialization.html
 *
 * sysID=255 (GCS), compID=194 (MAV_COMP_ID_UART_BRIDGE)
 * target_system=1 (autopilot), target_component=1
 */

#ifndef MAVLINK_BRIDGE_H
#define MAVLINK_BRIDGE_H

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/* MAVLink v2 frame constants */
#define MAVLINK_STX_V2              0xFD
#define MAVLINK_MSG_ID_RC_OVERRIDE  70
#define MAVLINK_SYS_ID              255   /* GCS */
#define MAVLINK_COMP_ID             194   /* MAV_COMP_ID_UART_BRIDGE */

/* RC_CHANNELS_OVERRIDE payload: 18 × uint16 + target_system + target_component
 * = 38 bytes. First 8 channels are individually named; 9-18 follow in order.
 * Layout (little-endian):
 *   [0..1]  chan1_raw
 *   [2..3]  chan2_raw
 *   ...
 *   [14..15] chan8_raw
 *   [16]    target_system
 *   [17]    target_component
 *   [18..19] chan9_raw  (MAVLink 2 extension)
 *   ...
 *   [34..35] chan18_raw
 */
#define RC_OVERRIDE_PAYLOAD_LEN     38

/* CRC-16/MCRF4XX with MAVLink extra byte (crc_extra for msg 70 = 124) */
#define MAVLINK_CRC_EXTRA_RC_OVERRIDE 124

static inline uint16_t mavlink_crc_accumulate(uint8_t b, uint16_t crc)
{
    uint8_t tmp = b ^ (uint8_t)(crc & 0xFF);
    tmp ^= (tmp << 4);
    return (uint16_t)((crc >> 8) ^ ((uint16_t)tmp << 8) ^ ((uint16_t)tmp << 3) ^ (tmp >> 4));
}

static inline uint16_t mavlink_crc16(const uint8_t *buf, int len, uint8_t extra)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++)
        crc = mavlink_crc_accumulate(buf[i], crc);
    crc = mavlink_crc_accumulate(extra, crc);
    return crc;
}

typedef struct {
    int      fd;
    struct sockaddr_in dest;
    uint8_t  seq;
} mavlink_udp_t;

static inline int mavlink_udp_init(mavlink_udp_t *m,
                                    const char *host, uint16_t port)
{
    m->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m->fd < 0) return -1;
    m->dest.sin_family      = AF_INET;
    m->dest.sin_port        = htons(port);
    m->dest.sin_addr.s_addr = inet_addr(host);
    m->seq = 0;
    return 0;
}

/*
 * Send RC_CHANNELS_OVERRIDE with up to 16 PWM values (µs, 1000-2000).
 * Channels beyond 16 are set to UINT16_MAX (ignored by autopilot).
 */
static inline int mavlink_send_rc_override(mavlink_udp_t *m,
                                            const uint16_t *chan_us,
                                            int n_chan)
{
    /* Build payload: 18 channels (8 + 10 extensions) + 2 target bytes = 38 */
    uint8_t payload[RC_OVERRIDE_PAYLOAD_LEN];
    memset(payload, 0xFF, sizeof(payload)); /* UINT16_MAX = ignored */

    int ch = (n_chan < 16) ? n_chan : 16;

    /* chan1..chan8 at bytes 0..15 */
    for (int i = 0; i < ch && i < 8; i++) {
        payload[i * 2]     = (uint8_t)(chan_us[i] & 0xFF);
        payload[i * 2 + 1] = (uint8_t)(chan_us[i] >> 8);
    }
    /* target_system=1, target_component=1 at bytes 16..17 */
    payload[16] = 1;
    payload[17] = 1;
    /* chan9..chan16 at bytes 18..33 */
    for (int i = 8; i < ch; i++) {
        int off = 18 + (i - 8) * 2;
        payload[off]     = (uint8_t)(chan_us[i] & 0xFF);
        payload[off + 1] = (uint8_t)(chan_us[i] >> 8);
    }

    /*
     * MAVLink v2 frame:
     *  [0]      STX   0xFD
     *  [1]      LEN   payload length
     *  [2]      INC   incompat flags (0)
     *  [3]      CMP   compat flags (0)
     *  [4]      SEQ
     *  [5]      SYS
     *  [6]      COMP
     *  [7..9]   MSGID (3 bytes LE)
     *  [10..10+LEN-1] PAYLOAD
     *  [10+LEN..10+LEN+1] CRC (LE)
     */
    uint8_t frame[10 + RC_OVERRIDE_PAYLOAD_LEN + 2];
    frame[0] = MAVLINK_STX_V2;
    frame[1] = RC_OVERRIDE_PAYLOAD_LEN;
    frame[2] = 0; /* incompat */
    frame[3] = 0; /* compat */
    frame[4] = m->seq++;
    frame[5] = MAVLINK_SYS_ID;
    frame[6] = MAVLINK_COMP_ID;
    frame[7] = (uint8_t)(MAVLINK_MSG_ID_RC_OVERRIDE & 0xFF);
    frame[8] = (uint8_t)((MAVLINK_MSG_ID_RC_OVERRIDE >> 8) & 0xFF);
    frame[9] = (uint8_t)((MAVLINK_MSG_ID_RC_OVERRIDE >> 16) & 0xFF);
    memcpy(&frame[10], payload, RC_OVERRIDE_PAYLOAD_LEN);

    /* CRC over bytes [1]..[9+LEN] (from LEN field, not STX) */
    uint16_t crc = mavlink_crc16(&frame[1], 9 + RC_OVERRIDE_PAYLOAD_LEN,
                                  MAVLINK_CRC_EXTRA_RC_OVERRIDE);
    int crc_off = 10 + RC_OVERRIDE_PAYLOAD_LEN;
    frame[crc_off]     = (uint8_t)(crc & 0xFF);
    frame[crc_off + 1] = (uint8_t)(crc >> 8);

    int total = 10 + RC_OVERRIDE_PAYLOAD_LEN + 2;
    return (int)sendto(m->fd, frame, (size_t)total, 0,
                       (struct sockaddr *)&m->dest, sizeof(m->dest));
}

#endif /* MAVLINK_BRIDGE_H */
