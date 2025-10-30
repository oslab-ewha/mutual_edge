#define GATEWAY_PORT_FOR_ENCLAVE 7000
#define GATEWAY_PORT_FOR_IIOT_DEVICE 8000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/vm_sockets.h>

#include <aws/common/string.h>
#include <aws/nitro_enclaves/attestation.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include "transmit.h"

extern struct aws_allocator	*allocator;

static int	gateway_fd;

void
connect_gateway(const char *gateway_ip)
{
	int	fd;
	int	rc;

	if (gateway_ip == NULL) {
		struct sockaddr_vm	svm = {
			.svm_family = AF_VSOCK,
			.svm_cid = VMADDR_CID_HOST,
			.svm_port = htons(GATEWAY_PORT_FOR_ENCLAVE)
		};
		fd = socket(AF_VSOCK, SOCK_STREAM, 0);
		rc = connect(fd, (struct sockaddr *)&svm, sizeof(svm));
	}
	else {
		struct sockaddr_in	iot_addr = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = inet_addr(gateway_ip),
			.sin_port = htons(GATEWAY_PORT_FOR_IIOT_DEVICE)
		};
		fd = socket(AF_INET, SOCK_STREAM, 0);
		rc = connect(fd, (struct sockaddr *)&iot_addr, sizeof(iot_addr));
	}
	if (rc < 0) {
		perror("cannot connect");
		close(fd);
		exit(1);
	}
	printf("gateway connected\n");
	gateway_fd = fd;
}

void
send_attsdoc(struct aws_byte_buf *attsdoc)
{
	send_len_data(gateway_fd, attsdoc->buffer, attsdoc->len);
	printf("attestation document sent: len: %d\n", attsdoc->len);
}

void
recv_attsdoc(struct aws_byte_buf *attsdoc)
{
	char	buffer[8192];
	ssize_t	len = sizeof(buffer);

	recv_len_data(gateway_fd, buffer, &len);

	aws_byte_buf_init(attsdoc, allocator, len);
	aws_byte_buf_write(attsdoc, buffer, len);

	printf("Gateway: received attestation document: %ld bytes from Enclave\n", len);
}

void
send_iiot_pubkey(EVP_PKEY *pkey)
{
	BIO *bio = BIO_new(BIO_s_mem());
	PEM_write_bio_PUBKEY(bio, pkey);
	char *data;
	long len = BIO_get_mem_data(bio, &data);

	send_len_data(gateway_fd, data, len);

	BIO_free(bio);

	printf("sent IIoT public key\n");
}

EVP_PKEY *
recv_iiot_pubkey(void)
{
	char	buf[4096];
	ssize_t	len;

	len = sizeof(buf);
	recv_len_data(gateway_fd, buf, &len);

	BIO *bio = BIO_new_mem_buf(buf, len);
	EVP_PKEY *pubkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
	BIO_free(bio);

	printf("recv IIoT public key\n");

	return pubkey;
}

struct aws_string *
request_offload(EVP_PKEY *pkey, EVP_PKEY *pubkey_enclave, struct aws_string *data_in)
{
	EVP_PKEY_CTX	*ctx;
	struct aws_string	*data_out;
	unsigned char encrypted[512] = {0}, decrypted[512] = {0};
	size_t encrypted_len = 0, decrypted_len = 0;

	ERR_print_errors_fp(stderr);

	ctx = EVP_PKEY_CTX_new(pubkey_enclave, NULL);
	EVP_PKEY_encrypt_init(ctx);

	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_encrypt(ctx, NULL, &encrypted_len, aws_string_c_str(data_in), data_in->len);
	EVP_PKEY_encrypt(ctx, encrypted, &encrypted_len, aws_string_c_str(data_in), data_in->len);

	EVP_PKEY_CTX_free(ctx);
	send_len_data(gateway_fd, encrypted, encrypted_len);
	encrypted_len = sizeof(encrypted);
	recv_len_data(gateway_fd, encrypted, &encrypted_len);

	ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_decrypt_init(ctx);
	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_decrypt(ctx, NULL, &decrypted_len, encrypted, encrypted_len);
	EVP_PKEY_decrypt(ctx, decrypted, &decrypted_len, encrypted, encrypted_len);

	data_out = aws_string_new_from_c_str(allocator, decrypted);
	EVP_PKEY_CTX_free(ctx);
	
	return data_out;
}

void
response_offload(EVP_PKEY *pkey, EVP_PKEY *pubkey_iiot, const char *response_msg)
{
	EVP_PKEY_CTX	*ctx;
	struct aws_string	*data_out;
	unsigned char encrypted[512] = {0}, decrypted[512] = {0};
	size_t encrypted_len = 0, decrypted_len = 0;

	ERR_print_errors_fp(stderr);

	encrypted_len = sizeof(encrypted);
	recv_len_data(gateway_fd, encrypted, &encrypted_len);
	if (encrypted_len <= 0) {
		printf("failed to receive offload request\n");
		exit(2);
	}
	ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_decrypt_init(ctx);
	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	EVP_PKEY_decrypt(ctx, NULL, &decrypted_len, encrypted, encrypted_len);
	EVP_PKEY_decrypt(ctx, decrypted, &decrypted_len, encrypted, encrypted_len);

	EVP_PKEY_CTX_free(ctx);

	printf("offload data: %s\n", decrypted);

	ctx = EVP_PKEY_CTX_new(pubkey_iiot, NULL);
	EVP_PKEY_encrypt_init(ctx);

	EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
	decrypted_len = strlen(response_msg);
	EVP_PKEY_encrypt(ctx, NULL, &encrypted_len, response_msg, decrypted_len);
	EVP_PKEY_encrypt(ctx, encrypted, &encrypted_len, response_msg, decrypted_len);
	EVP_PKEY_CTX_free(ctx);

	send_len_data(gateway_fd, encrypted, encrypted_len);
}
