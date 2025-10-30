#ifndef _TRANSMIT_H_
#define _TRANSMIT_H_

void send_len_data(int fd, char *data, ssize_t size);
void recv_len_data(int fd, char *data, ssize_t *psize);

#endif
