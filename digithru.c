/* SPDX-License-Identifier: GPL-2.0-or-later */
/* DigiThru 0.2 for the Digitakt mk1 (OS 1.53): pitch bend, aftertouch and the
 * mod wheel from MIDI IN go on to the active MIDI track's channel.
 *
 * The stock firmware drops them: its MIDI task sends each message it receives
 * to a table of handlers by status (0x401b8a54, one pointer per high nibble),
 * and the entries for poly aftertouch (0xA0), channel pressure (0xD0) and pitch
 * bend (0xE0) point at a bare rts. DigiThru points those three entries at its
 * own handlers ("ptr" sites). The control change entry (0xB0) it wraps: the
 * stock CC handler takes CC 1 and 33 (mod wheel MSB, LSB) for the track's own
 * 14-bit mod wheel (0x400d685a) before any CC router or core's ev_midi_cc sees
 * them, so DigiThru forwards them first and then calls the stock handler.
 *
 * A message is forwarded when it comes on the AUTO channel, MIDI CONFIG
 * receives notes, and the active track is a MIDI track (9-16) whose CHAN is on.
 * It goes out on that track's channel through the sequencer's own output
 * routine, so it follows PORT CONFIG > OUTPUT TO like the track's notes.
 * When the active track changes, the track left gets pitch bend centre and
 * pressure 0, so no note on the synth stays bent. */

typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

/* ---- OS 1.53 (mod.json cflags) ---------------------------------------- */
#ifndef SEQ_SEND_AT
#error "SEQ_SEND_AT: the sequencer's MIDI output routine (mod.json cflags)"
#endif
/* 0x400e076a send(len, buf): to DIN and/or USB per OUTPUT TO (0x421c8560);
 * the DIN send copies the whole message with interrupts masked. */
typedef void (*seq_send_t)(int32_t len, const uint8_t *buf);
#define SEQ_SEND ((seq_send_t)SEQ_SEND_AT)
/* 0x400e07b0 chan(mt): MIDI track mt (0-7)'s channel 0-15, or -1 (CHAN off) */
typedef int32_t (*mtrk_chan_t)(int32_t mt);
#define MTRK_CHAN ((mtrk_chan_t)MTRK_CHAN_AT)
#define ACTIVE_TRACK (*(volatile int32_t *)ACTIVE_TRACK_AT) /* 0-7 audio, 8-15 MIDI */
#define AUTO_CHAN    (*(volatile int32_t *)AUTO_CHAN_AT)    /* 0-15 */
#define RX_NOTES     (*(volatile int32_t *)RX_NOTES_AT)     /* MIDI CONFIG: receive notes */
#define RX_CC        (*(volatile int32_t *)RX_CC_AT)        /* MIDI CONFIG: receive CC */
/* 0x400d4a90 the stock CC handler, f(msg, len, flags) */
typedef void (*cc_handler_t)(const uint8_t *msg, int32_t len, int32_t flags);
#define STOCK_CC ((cc_handler_t)STOCK_CC_AT)

/* state, written by the MIDI task (handlers) and the UI task (tick) */
static volatile int32_t dt_bent_ch = -1;     /* a channel left with PB/pressure off centre */
static volatile int32_t dt_bent_track = -1;  /* the track that channel belonged to */
static volatile int32_t dt_pb_off, dt_at_off;

static int32_t dt_target(const uint8_t *msg)
{
    int32_t t;
    if ((msg[0] & 15) != AUTO_CHAN || !RX_NOTES)
        return -1;
    t = ACTIVE_TRACK;
    if (t < 8 || t > 15)
        return -1;
    return MTRK_CHAN(t - 8);
}

static void dt_send(int32_t len, uint8_t b0, uint8_t b1, uint8_t b2)
{
    uint8_t m[3];
    m[0] = b0; m[1] = b1 & 127; m[2] = b2 & 127;
    SEQ_SEND(len, m);
}

static void dt_mark(int32_t ch)
{
    dt_bent_ch = ch;
    dt_bent_track = ACTIVE_TRACK;
}

/* table handlers: f(msg, len, flags); msg[0] the status */
void dt_pitchbend(const uint8_t *msg, int32_t len, int32_t flags)
{
    int32_t ch = dt_target(msg);
    (void)len; (void)flags;
    if (ch < 0 || ch > 15)
        return;
    dt_send(3, 0xE0 | ch, msg[1], msg[2]);
    dt_pb_off = !(msg[1] == 0 && msg[2] == 64);
    dt_mark(ch);
}

void dt_chanpress(const uint8_t *msg, int32_t len, int32_t flags)
{
    int32_t ch = dt_target(msg);
    (void)len; (void)flags;
    if (ch < 0 || ch > 15)
        return;
    dt_send(2, 0xD0 | ch, msg[1], 0);
    dt_at_off = msg[1] != 0;
    dt_mark(ch);
}

void dt_polypress(const uint8_t *msg, int32_t len, int32_t flags)
{
    int32_t ch = dt_target(msg);
    (void)len; (void)flags;
    if (ch < 0 || ch > 15)
        return;
    dt_send(3, 0xA0 | ch, msg[1], msg[2]);
}

/* the CC table entry: the mod wheel (CC 1, and its LSB CC 33) on the auto
 * channel goes to the active MIDI track first; every CC then goes on to stock */
void dt_control(const uint8_t *msg, int32_t len, int32_t flags)
{
    int32_t t, ch;
    if ((msg[1] == 1 || msg[1] == 33) && (msg[0] & 15) == AUTO_CHAN && RX_CC) {
        t = ACTIVE_TRACK;
        if (t >= 8 && t <= 15) {
            ch = MTRK_CHAN(t - 8);
            if (ch >= 0 && ch <= 15)
                dt_send(3, 0xB0 | ch, msg[1], msg[2]);
        }
    }
    STOCK_CC(msg, len, flags);
}

/* ev_tick: the active track changed, so centre what the old one left bent */
void dt_tick(void)
{
    int32_t ch = dt_bent_ch;
    if (ch < 0 || ACTIVE_TRACK == dt_bent_track)
        return;
    dt_bent_ch = -1;
    if (dt_pb_off)
        dt_send(3, 0xE0 | ch, 0, 64);
    if (dt_at_off)
        dt_send(2, 0xD0 | ch, 0, 0);
    dt_pb_off = dt_at_off = 0;
}
