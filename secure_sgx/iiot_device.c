#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <aws/common/byte_buf.h>
#include <aws/common/string.h>
#include <openssl/evp.h>

#include "gateway_client.h"

static char	*gateway_ip = "127.0.0.1";

struct aws_allocator	*allocator;
extern EVP_PKEY *validate_attsdoc(struct aws_byte_buf *attsdoc);

static void
usage(void)
{
        fprintf(stdout,
"Usage: iiot_device <options> <config path>\n"
" <options>\n"
"      -h: this message\n"
"      -g <ip>: gateway IP\n"
        );
}

static void
parse_args(int argc, char *argv[])
{
	int	c;

	while ((c = getopt(argc, argv, "g:h")) != -1) {
		switch (c) {
		case 'g':
			gateway_ip = strdup(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		default:
			fprintf(stderr, "invalid option\n");
			usage();
			exit(1);
		}
	}
}

static int
need_offloading(void)
{
	return rand() % 2;
}

static void
run_iiot_emulation(EVP_PKEY *pkey, EVP_PKEY *pubkey_enclave)
{
	while (1) {
		/* execute IIOT logic */
		if (need_offloading()) {
			static int	n_offloads;
			struct aws_string	*data_in, *data_out;
			char	buf[1024];

			snprintf(buf, sizeof(buf), "task offloading to TEE: %d\n", n_offloads++);
			data_in = aws_string_new_from_c_str(allocator, buf);
			data_out = request_offload(pkey, pubkey_enclave, data_in);
			aws_string_destroy(data_in);
			printf("TEE response: %s\n", aws_string_c_str(data_out));
			aws_string_destroy(data_out);

			sleep(1);
		}
		else {
			printf("run IIoT tasks locally\n");
			sleep(2);
		}
		printf("sleep until task period ends(3 secs)\n");
		sleep(3);
	}
}

static EVP_PKEY *
generate_rsa_keypair(void)
{
	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
	EVP_PKEY *pkey = NULL;

	EVP_PKEY_keygen_init(ctx);
	EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
	EVP_PKEY_keygen(ctx, &pkey);
	EVP_PKEY_CTX_free(ctx);
	return pkey;
}

int
main(int argc, char **argv)
{
	struct aws_byte_buf	attsdoc;
	EVP_PKEY	*mykey, *pubkey_enclave;

	parse_args(argc, argv);

	allocator = aws_default_allocator();

	connect_gateway(gateway_ip);
	recv_attsdoc(&attsdoc);

	pubkey_enclave = validate_attsdoc(&attsdoc);
	mykey = generate_rsa_keypair();

	send_iiot_pubkey(mykey);

	run_iiot_emulation(mykey, pubkey_enclave);

	return 0;
}
