#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>

#include <aws/nitro_enclaves/kms.h>
#include <aws/nitro_enclaves/nitro_enclaves.h>
#include <aws/nitro_enclaves/attestation.h>

#include <openssl/bytestring.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

#include "gateway_client.h"

struct aws_allocator	*allocator;
static struct aws_rsa_keypair	*keypair;

static void
get_attsdoc(struct aws_byte_buf *attsdoc)
{
	int	rc;

	rc = aws_attestation_request(allocator, keypair, attsdoc);
	if (rc != AWS_OP_SUCCESS) {
		perror("Failed to request attestation");
		exit(1);
	}
}

static void
run_service_offload(EVP_PKEY *pkey)
{
	EVP_PKEY	*pubkey_iiot;

	pubkey_iiot = recv_iiot_pubkey();

	while (1) {
		static int	n_response;
		char	msg[512];

		snprintf(msg, sizeof(msg), "offload response: %d\n", n_response++);
		response_offload(pkey, pubkey_iiot, msg);
	}
}

int
main(void)
{
	struct aws_byte_buf	buf_attsdoc;

	aws_nitro_enclaves_library_init(NULL);

	allocator = aws_nitro_enclaves_get_allocator();

	keypair = aws_attestation_rsa_keypair_new(allocator, AWS_RSA_2048);
	if (keypair == NULL) {
		perror("Could not create keypair");
		exit(1);
	}

	connect_gateway(NULL);
	get_attsdoc(&buf_attsdoc);
	send_attsdoc(&buf_attsdoc);

	run_service_offload((EVP_PKEY *)keypair->key_impl);

	return 0;
}
