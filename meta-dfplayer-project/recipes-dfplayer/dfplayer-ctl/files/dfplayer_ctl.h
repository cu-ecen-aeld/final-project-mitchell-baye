#ifndef DFPLAYER_CTL_H
#define DFPLAYER_CTL_H

#include <stdint.h>

/* Frame layout: START VER LEN CMD FEEDBACK PARAM_HI PARAM_LO CHK_HI CHK_LO END */
#define DFP_START_BYTE 0x7E
#define DFP_VERSION 0xFF
#define DFP_LEN 0x06
#define DFP_END_BYTE 0xEF
#define DFP_FRAME_SIZE 10

#define DFP_FEEDBACK_OFF 0x00
#define DFP_FEEDBACK_ON 0x01

/* Command bytes */
#define DFP_CMD_NEXT 0x01
#define DFP_CMD_PREV 0x02
#define DFP_CMD_PLAY_TRACK 0x03
#define DFP_CMD_VOLUME_UP 0x04
#define DFP_CMD_VOLUME_DOWN 0x05
#define DFP_CMD_SET_VOLUME 0x06
#define DFP_CMD_PLAY 0x0D
#define DFP_CMD_PAUSE 0x0E
#define DFP_CMD_STOP 0x16
#define DFP_CMD_RESET 0x0C
#define DFP_CMD_QUERY_VOLUME 0x43
#define DFP_CMD_QUERY_STATUS 0x42

typedef struct {
    int fd;
} dfp_handle_t;

int dfp_open(dfp_handle_t *h, const char *device);
void dfp_close(dfp_handle_t *h);
int dfp_send_command(dfp_handle_t *h, uint8_t cmd, uint16_t param);

int dfp_play(dfp_handle_t *h);
int dfp_pause(dfp_handle_t *h);
int dfp_stop(dfp_handle_t *h);
int dfp_next(dfp_handle_t *h);
int dfp_prev(dfp_handle_t *h);
int dfp_play_track(dfp_handle_t *h, uint16_t track);
int dfp_set_volume(dfp_handle_t *h, uint8_t volume);
int dfp_volume_up(dfp_handle_t *h);
int dfp_volume_down(dfp_handle_t *h);

#endif /* DFPLAYER_CTL_H */
