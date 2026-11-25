/*
 * crsf.h - CRSF (Crossfire/ExpressLRS) frame definitions
 *
 * Wire format (half-duplex UART, 420000 or 115200 baud, inverted on some HW):
 *
 *   [SYNC 0xC8] [LEN] [TYPE] [PAYLOAD...] [CRC8]
 *
 *   LEN  = number of bytes from TYPE through CRC8 (payload + 2)
 *   CRC8 = DVB-S2 polynomial 0xD5 over TYPE..last payload byte
 *
 * References:
 *   https://github.com/ExpressLRS/ExpressLRS/wiki/CRSF-Protocol
 *   https://github.com/crsf-wg/crsf/wiki
 */

#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */
#define CRSF_SYNC_BYTE          0xC8
#define CRSF_MAX_FRAME_LEN      64
#define CRSF_HEADER_LEN         2   /* SYNC + LEN */
#define CRSF_TYPE_RC_CHANNELS   0x16
#define CRSF_RC_CHANNEL_COUNT   16

/* RC_CHANNELS_PACKED payload is 22 bytes = 16 × 11 bits */
#define CRSF_RC_PAYLOAD_LEN     22

/* Raw CRSF channel range: 172–1811, centre 992 */
#define CRSF_CHAN_MIN           172
#define CRSF_CHAN_MID           992
#define CRSF_CHAN_MAX           1811

/* -------------------------------------------------------------------------
 * Types
 * ---------------------------------------------------------------------- */
typedef struct {
    uint16_t chan[CRSF_RC_CHANNEL_COUNT];
} crsf_channels_t;

typedef struct {
    uint8_t buf[CRSF_MAX_FRAME_LEN + CRSF_HEADER_LEN];
    int     len;    /* bytes currently in buf */
} crsf_parser_t;

/* -------------------------------------------------------------------------
 * CRC-8 DVB-S2 (poly 0xD5)
 * ---------------------------------------------------------------------- */
static inline uint8_t crsf_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5) : (uint8_t)(crc << 1);
    }
    return crc;
}

/* -------------------------------------------------------------------------
 * Unpack 16 × 11-bit channels from 22-byte CRSF payload.
 * Returns true on success (valid CRC and length).
 * ---------------------------------------------------------------------- */
static inline bool crsf_parse_rc(const uint8_t *frame, int frame_len,
                                  crsf_channels_t *out)
{
    /* frame = [SYNC][LEN][TYPE][22 bytes payload][CRC] */
    if (frame_len < (int)(CRSF_HEADER_LEN + 1 + CRSF_RC_PAYLOAD_LEN + 1))
        return false;

    uint8_t len  = frame[1];
    uint8_t type = frame[2];
    if (type != CRSF_TYPE_RC_CHANNELS)
        return false;
    if (len < CRSF_RC_PAYLOAD_LEN + 2)
        return false;

    /* CRC covers TYPE + payload */
    uint8_t crc_calc = crsf_crc8(&frame[2], len - 1);
    uint8_t crc_recv = frame[2 + len - 1];
    if (crc_calc != crc_recv)
        return false;

    const uint8_t *p = &frame[3]; /* start of 22-byte payload */

    /* 11-bit little-endian packing, 16 channels */
    out->chan[0]  = ((uint16_t)p[0]        | ((uint16_t)p[1]  << 8))  & 0x07FF;
    out->chan[1]  = ((uint16_t)p[1]  >> 3  | ((uint16_t)p[2]  << 5))  & 0x07FF;
    out->chan[2]  = ((uint16_t)p[2]  >> 6  | ((uint16_t)p[3]  << 2)
                                           | ((uint16_t)p[4]  << 10)) & 0x07FF;
    out->chan[3]  = ((uint16_t)p[4]  >> 1  | ((uint16_t)p[5]  << 7))  & 0x07FF;
    out->chan[4]  = ((uint16_t)p[5]  >> 4  | ((uint16_t)p[6]  << 4))  & 0x07FF;
    out->chan[5]  = ((uint16_t)p[6]  >> 7  | ((uint16_t)p[7]  << 1)
                                           | ((uint16_t)p[8]  << 9))  & 0x07FF;
    out->chan[6]  = ((uint16_t)p[8]  >> 2  | ((uint16_t)p[9]  << 6))  & 0x07FF;
    out->chan[7]  = ((uint16_t)p[9]  >> 5  | ((uint16_t)p[10] << 3))  & 0x07FF;
    out->chan[8]  = ((uint16_t)p[11]       | ((uint16_t)p[12] << 8))  & 0x07FF;
    out->chan[9]  = ((uint16_t)p[12] >> 3  | ((uint16_t)p[13] << 5))  & 0x07FF;
    out->chan[10] = ((uint16_t)p[13] >> 6  | ((uint16_t)p[14] << 2)
                                           | ((uint16_t)p[15] << 10)) & 0x07FF;
    out->chan[11] = ((uint16_t)p[15] >> 1  | ((uint16_t)p[16] << 7))  & 0x07FF;
    out->chan[12] = ((uint16_t)p[16] >> 4  | ((uint16_t)p[17] << 4))  & 0x07FF;
    out->chan[13] = ((uint16_t)p[17] >> 7  | ((uint16_t)p[18] << 1)
                                           | ((uint16_t)p[19] << 9))  & 0x07FF;
    out->chan[14] = ((uint16_t)p[19] >> 2  | ((uint16_t)p[20] << 6))  & 0x07FF;
    out->chan[15] = ((uint16_t)p[20] >> 5  | ((uint16_t)p[21] << 3))  & 0x07FF;

    return true;
}

/* Map CRSF raw value (172-1811) to MAVLink PWM microseconds (1000-2000) */
static inline uint16_t crsf_to_us(uint16_t v)
{
    if (v <= CRSF_CHAN_MIN) return 1000;
    if (v >= CRSF_CHAN_MAX) return 2000;
    return (uint16_t)(1000 + (v - CRSF_CHAN_MIN) * 1000 / (CRSF_CHAN_MAX - CRSF_CHAN_MIN));
}

#endif /* CRSF_H */
