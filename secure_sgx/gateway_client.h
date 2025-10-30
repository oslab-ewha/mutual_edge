#ifndef _GATEWAY_CLIENT_H_
#define _GATEWAY_CLIENT_H_

#include <aws/common/byte_buf.h>
#include <aws/common/string.h>
#include <openssl/evp.h>

void connect_gateway(const char *gateway_ip);
void send_attsdoc(struct aws_byte_buf *attsdoc);
void recv_attsdoc(struct aws_byte_buf *attsdoc);

void send_iiot_pubkey(EVP_PKEY *pkey);
EVP_PKEY *recv_iiot_pubkey(void);

struct aws_string *request_offload(EVP_PKEY *pkey, EVP_PKEY *pubkey_enclave, struct aws_string *data_in);
void response_offload(EVP_PKEY *pkey, EVP_PKEY *pubkey_iiot, const char *response_msg);

#endif
