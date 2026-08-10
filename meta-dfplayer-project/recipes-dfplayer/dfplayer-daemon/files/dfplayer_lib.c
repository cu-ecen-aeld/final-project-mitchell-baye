#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "dfplayer_ctl.h"

static int configure_serial(int fd)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_cflag |= CLOCAL | CREAD;

    tty.c_lflag = 0;   /* raw mode: no canonical, no echo, no signals */
    tty.c_iflag = 0;   /* no input processing (no XON/XOFF, no CR/NL translation) */
    tty.c_oflag = 0;   /* no output processing */

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10; /* 1.0s read timeout */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    return 0;
}

int dfp_open(dfp_handle_t *h, const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (configure_serial(fd) != 0) {
        close(fd);
        return -1;
    }

    h->fd = fd;
    return 0;
}

void dfp_close(dfp_handle_t *h)
{
    if (h->fd >= 0) {
        close(h->fd);
        h->fd = -1;
    }
}

/*
 * Checksum = 0 - (VER + LEN + CMD + FEEDBACK + PARAM_HI + PARAM_LO)
 * i.e. two's-complement of the sum of bytes 1..6 (0-indexed from VER).
 */
static uint16_t dfp_checksum(uint8_t cmd, uint8_t feedback,
                              uint8_t param_hi, uint8_t param_lo)
{
    uint16_t sum = DFP_VERSION + DFP_LEN + cmd + feedback + param_hi + param_lo;
    return (uint16_t)(0 - sum);
}

int dfp_send_command(dfp_handle_t *h, uint8_t cmd, uint16_t param)
{
    uint8_t param_hi = (uint8_t)(param >> 8);
    uint8_t param_lo = (uint8_t)(param & 0xFF);
    uint16_t checksum = dfp_checksum(cmd, DFP_FEEDBACK_OFF, param_hi, param_lo);

    uint8_t frame[DFP_FRAME_SIZE] = {
        DFP_START_BYTE,
        DFP_VERSION,
        DFP_LEN,
        cmd,
        DFP_FEEDBACK_OFF,
        param_hi,
        param_lo,
        (uint8_t)(checksum >> 8),
        (uint8_t)(checksum & 0xFF),
        DFP_END_BYTE
    };

    ssize_t written = write(h->fd, frame, sizeof(frame));
    if (written != (ssize_t)sizeof(frame)) {
        fprintf(stderr, "dfp_send_command: short write (%zd of %zu bytes): %s\n",
                written, sizeof(frame), strerror(errno));
        return -1;
    }

    /* DFPlayer needs a short settle time between commands */
    tcdrain(h->fd);
    usleep(100000); /* 100ms */

    return 0;
}

int dfp_play(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_PLAY, 0); }
int dfp_pause(dfp_handle_t *h)                      { return dfp_send_command(h, DFP_CMD_PAUSE, 0); }
int dfp_stop(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_STOP, 0); }
int dfp_next(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_NEXT, 0); }
int dfp_prev(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_PREV, 0); }
int dfp_repeat(dfp_handle_t *h, uint8_t status)     { return dfp_send_command(h, DFP_CMD_REPEAT, status); }
int dfp_play_track(dfp_handle_t *h, uint16_t track) { return dfp_send_command(h, DFP_CMD_PLAY_TRACK, track); }
int dfp_set_volume(dfp_handle_t *h, uint8_t volume) { return dfp_send_command(h, DFP_CMD_SET_VOLUME, volume); }
int dfp_volume_up(dfp_handle_t *h)                  { return dfp_send_command(h, DFP_CMD_VOLUME_UP, 0); }
int dfp_volume_down(dfp_handle_t *h)                { return dfp_send_command(h, DFP_CMD_VOLUME_DOWN, 0); }
