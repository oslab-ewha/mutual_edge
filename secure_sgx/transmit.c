#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void
send_len_data(int fd, char *data, ssize_t size)
{
	ssize_t	sent, total_sent = 0;

	int len = (int)size;
	char *plen = (char *)&len;
	while (total_sent < sizeof(int)) {
		sent = write(fd, plen + total_sent, sizeof(int) - total_sent);
		if (sent <= 0) {
			perror("send_len_data: failed to send length");
			exit(3);
		}
		total_sent += sent;
	}
	
	total_sent = 0;
	while (total_sent < size) {
		sent = write(fd, data + total_sent, size - total_sent);
		if (sent <= 0) {
			perror("send_len_data: failed to send data");
			exit(3);
		}
		total_sent += sent;
	}
}

void
recv_len_data(int fd, char *data, ssize_t *psize)
{
	ssize_t recvd = 0;
	ssize_t total_recvd = 0;
	int len = 0;
	char *plen = (char *)&len;

	while (total_recvd < sizeof(int)) {
		recvd = read(fd, plen + total_recvd, sizeof(int) - total_recvd);
		if (recvd <= 0) {
			perror("recv_len_data: failed to receive length");
			exit(3);
		}
		total_recvd += recvd;
	}

	*psize = len;

	total_recvd = 0;
	while (total_recvd < len) {
		recvd = read(fd, data + total_recvd, len - total_recvd);
		if (recvd <= 0) {
			perror("recv_len_data: failed to receive data");
			exit(3);
		}
		total_recvd += recvd;
	}
}
