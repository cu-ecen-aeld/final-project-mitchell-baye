#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "dfplayer-ctl.h"

static int configure_serial(int fd)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) != 0)
    {
        perror("tcgetattr");
        return -1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_cflag |= CLOCAL | CREAD;

    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("tcsettattr");
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    return 0;
}

int dfp_open(dfp_handle_t *h, const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (configure_serial(fd) != 0)
    {
        close(fd);
        return -1;
    }

    h->fd = fd;
    return 0;
}

void dfp_close(dfp_handle_t *h)
{
    if (h->fd >= 0)
    {
        close(h->fd);
        h->fd = -1;
    }
}

/*
 * Checksum = 0 - (VER + LEN + CMD + FEEDBACK + PARAM_HI + PARAM_LO)
 */
static uint16_t dfp_checksum(uint8_t cmd, uint8_t feedback, uint8_t param_hi, uint8_t param_lo)
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
    if (written != (ssize_t)sizeof(frame))
    {
        fprintf(stderr, "dfp_send_command: short write (%zd of %zu bytes): %s\n", 
                written, sizeof(frame), strerror(errno));
        return -1;
    }

    tcdrain(h->fd);
    usleep(100000);

    return 0;
}

int dfp_play(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_PLAY, 0); }
int dfp_pause(dfp_handle_t *h)                      { return dfp_send_command(h, DFP_CMD_PAUSE, 0); }
int dfp_stop(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_STOP, 0); }
int dfp_next(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_NEXT, 0); }
int dfp_prev(dfp_handle_t *h)                       { return dfp_send_command(h, DFP_CMD_PREV, 0); }
int dfp_play_track(dfp_handle_t *h, uint16_t track) { return dfp_send_command(h, DFP_CMD_PLAY_TRACK, track); }
int dfp_set_volume(dfp_handle_t *h, uint8_t volume) { return dfp_send_command(h, DFP_CMD_SET_VOLUME, volume); }
int dfp_volume_up(dfp_handle_t *h)                  { return dfp_send_command(h, DFP_CMD_VOLUME_UP, 0); }
int dfp_volume_down(dfp_handle_t *h)                { return dfp_send_command(h, DFP_CMD_VOLUME_DOWN, 0); }

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-d device] <command> [args}\n"
            "\n"
            "Options:\n"
            "  -d device    UART device (default: /dev/ttyAMA0)\n"
            "\n"
            "Commands:\n"
            "  play             Resume/start playback\n"
            "  pause            Pause playback\n"
            "  stop             Stop playback\n"
            "  next             Skip to next track\n"
            "  prev             Skip to previous track\n"
            "  track <n>        Play track number n (1-based)\n"
            "  volume <0-30>    Set volume level\n"
            "  volup            Increase volume by one step\n"
            "  voldown          Decrease volume by one step\n",
            prog);
}

int main(int argc, char *argv[])
{
    const char *device = "/dev/ttyAMA0";
    int argi = 1;

    if (argc >= 3 && strcmp(argv[1], "-d") == 0) 
    {
        device = argv[2];
        argi = 3;
    }

    if (argi >= argc)
    {
        usage(argv[0]);
        return 1;
    }

    const char *command = argv[argi];

    dfp_handle_t h;
    if (dfp_open(&h, device) != 0)
    {
        fprintf(stderr, "Failed to open %s\n", device);
        return 1;
    }

    int rc = 0;

    if (strcmp(command, "play") == 0) 
        rc = dfp_play(&h);
    else if (strcmp(command, "pause") == 0)
        rc = dfp_pause(&h);
    else if (strcmp(command, "stop") == 0)
        rc = dfp_stop(&h);
    else if (strcmp(command, "next") == 0)
        rc = dfp_next(&h);
    else if (strcmp(command, "prev") == 0)
        rc = dfp_prev(&h);
    else if (strcmp(command, "track") == 0)
    {
        if (argi + 1 >= argc)
        {
            fprintf(stderr, "track requires a track number\n");
            rc = -1;
        }
        else 
        {
            long track = strtol(argv[argi + 1], NULL, 10);
            rc = dfp_play_track(&h, (uint8_t)track);
        }
    }
    else if (strcmp(command, "volume") == 0)
    {
        if (argi +1 >= argc)
        {
            fprintf(stderr, "volume requires a level 0-30\n");
            rc = -1;
        }
        else
        {
            long vol = strtol(argv[argi + 1], NULL, 10);
            if (vol < 0 || vol > 30)
            {
                fprintf(stderr, "volume must be between 0 and 30\n");
                rc = -1;
            }
            else
                rc = dfp_set_volume(&h, (uint8_t)vol);
        }
    }
    else if (strcmp(command, "volup") == 0)
        rc = dfp_volume_up(&h);
    else if (strcmp(command, "voldown") == 0)
        rc = dfp_volume_down(&h);
    else
    {
        usage(argv[0]);
        rc = -1;
    }
    
    dfp_close(&h);
    return rc == 0 ? 0 : 1;
}

