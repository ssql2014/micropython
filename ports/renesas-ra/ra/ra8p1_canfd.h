#ifndef RA8P1_CANFD_H
#define RA8P1_CANFD_H

#include <stdint.h>
#include <stdbool.h>

int  ra8p1_canfd_init(uint32_t baudrate);
void ra8p1_canfd_deinit(void);
int  ra8p1_canfd_send(uint32_t id, bool ext, bool fd,
                      const uint8_t *data, uint8_t dlc, uint32_t timeout_ms);
int  ra8p1_canfd_recv(uint32_t *id, bool *ext, bool *fd,
                      uint8_t *data, uint8_t *dlc, uint32_t timeout_ms);
bool ra8p1_canfd_is_open(void);

#endif /* RA8P1_CANFD_H */
