#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "transmit.h"

#define VSOCK_PORT     7000
#define IOT_PORT       8000
#define BUFFER_SIZE    8192

static char	attsdoc_buf[8192];
static ssize_t	attsdoc_len;

static void
send_enclave_attsdoc(int fd)
{
	ssize_t	sent;

	send_len_data(fd, attsdoc_buf, attsdoc_len);

	printf("Gateway: send attestation document: len: %d\n", attsdoc_len);
}

static void
recv_enclave_attsdoc(int fd)
{
	attsdoc_len = sizeof(attsdoc_buf);
	recv_len_data(fd, attsdoc_buf, &attsdoc_len);

	printf("Gateway: received attestation document: %ld bytes from Enclave\n", attsdoc_len);
}

static int
accept_enclave(void)
{
	struct sockaddr_vm enclave_addr = {
		.svm_family = AF_VSOCK,
		.svm_port = htons(VSOCK_PORT),
		.svm_cid = VMADDR_CID_ANY
	};
	int	vsock_fd;

	printf("Gateway: waiting for Enclave connection on vsock...\n");

	vsock_fd = socket(AF_VSOCK, SOCK_STREAM, 0);
	bind(vsock_fd, (struct sockaddr*)&enclave_addr, sizeof(enclave_addr));
	listen(vsock_fd, 1);

	int	enclave_fd = accept(vsock_fd, NULL, NULL);
	printf("Gateway: Enclave connected via vsock: %d.\n", enclave_fd);

	recv_enclave_attsdoc(enclave_fd);

	return enclave_fd;
}

static int
accept_iiot_device(void)
{
	struct sockaddr_in iot_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(IOT_PORT),
		.sin_addr.s_addr = INADDR_ANY
	};
	int	iot_sock;

	iot_sock = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;
	setsockopt(iot_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	bind(iot_sock, (struct sockaddr*)&iot_addr, sizeof(iot_addr));
	listen(iot_sock, 1);
	printf("Gateway: waiting for IoT connection...\n");

	int	iiot_fd = accept(iot_sock, NULL, NULL);
	printf("Gateway: IoT device connected.\n");

	send_enclave_attsdoc(iiot_fd);

	return iiot_fd;
}

static void
relay_iiot_pubkey(int enclave_fd, int iiot_fd)
{
	char	buf[1024];
	ssize_t	len, sent;

	len = sizeof(buf);
	recv_len_data(iiot_fd, buf, &len);
	printf("Gateway: recv IIoT pubkey\n");
	send_len_data(enclave_fd, buf, len);
	printf("Gateway: sent IIoT pubkey\n");
}

static void
do_relay_offload(int enclave_fd, int iiot_fd)
{
	while (1) {
		char	buf[1024];
		ssize_t	len, sent;
		
		len = sizeof(buf);
		recv_len_data(iiot_fd, buf, &len);

		send_len_data(enclave_fd, buf, len);

		len = sizeof(buf);
		recv_len_data(enclave_fd, buf, &len);

		send_len_data(iiot_fd, buf, len);
	}
}

int
main(void)
{
	int	enclave_fd, iiot_fd;

	enclave_fd = accept_enclave();
	iiot_fd = accept_iiot_device();

	relay_iiot_pubkey(enclave_fd, iiot_fd);
	do_relay_offload(enclave_fd, iiot_fd);

	return 0;
}
