#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cbor.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <aws/common/byte_buf.h>
#define ROOT_CA_CERT_PATH "root.pem"

#undef DEBUG

EVP_PKEY *
validate_attsdoc(struct aws_byte_buf *attsdoc)
{
	struct cbor_load_result	result;
	cbor_item_t *item = cbor_load(attsdoc->buffer, attsdoc->len, &result);
	if (result.error.code != CBOR_ERR_NONE) {
		fprintf(stderr, "failed to decode CBOR: %d\n", result.error.code);
		exit(2);
	}

#ifdef DEBUG
	// check CBOR type
	printf("Top-level CBOR item type: ");
	if (cbor_isa_tag(item)) {
		printf("Tag with value: %llu\n", cbor_tag_value(item));
	} else if (cbor_isa_array(item)) {
		printf("Array with %zu items\n", cbor_array_size(item));
	} else if (cbor_isa_map(item)) {
		printf("Map with %zu pairs\n", cbor_map_size(item));
	} else {
		printf("Other type\n");
	}
#endif
	
	cbor_item_t *cose_sign1 = NULL;
    
	// check COSE_Sign1 structure (tag check)
	if (cbor_isa_tag(item)) {
		printf("Found CBOR tag: %llu\n", cbor_tag_value(item));
		cose_sign1 = cbor_tag_item(item);
	} else if (cbor_isa_array(item) && cbor_array_size(item) == 4) {
		printf("Found untagged array with 4 items, treating as COSE_Sign1\n");
		cose_sign1 = item;
	} else {
		printf("Not a tagged COSE_Sign1, treating as raw attestation document\n");
        
		fprintf(stderr, "Cannot handle raw attestation document directly, please check format\n");
		cbor_decref(&item);
		exit(2);
	}
    
	// validate cose_sign1 structure
	if (!cbor_isa_array(cose_sign1) || cbor_array_size(cose_sign1) != 4) {
		fprintf(stderr, "Invalid COSE_Sign1 structure (expected array of 4 items)\n");
		cbor_decref(&item);
		exit(2);
	}
    
	// protected header, unprotected header, payload, signature
	cbor_item_t *protected_header = cbor_array_get(cose_sign1, 0);
	cbor_item_t *unprotected_header = cbor_array_get(cose_sign1, 1);
	cbor_item_t *payload = cbor_array_get(cose_sign1, 2);
	cbor_item_t *signature = cbor_array_get(cose_sign1, 3);
    
	if (!cbor_isa_bytestring(protected_header)) {
		fprintf(stderr, "Protected header is not a bytestring\n");
		cbor_decref(&item);
		exit(2);
	}
    
	// Payload is bytestring or null
	if (!cbor_isa_bytestring(payload) && !cbor_is_null(payload)) {
		fprintf(stderr, "Payload is neither bytestring nor null\n");
		cbor_decref(&item);
		exit(2);
	}
    
	if (!cbor_isa_bytestring(signature)) {
		fprintf(stderr, "Signature is not a bytestring\n");
		cbor_decref(&item);
		exit(2);
	}
    
	// parsing protected header
	unsigned char *protected_bytes = cbor_bytestring_handle(protected_header);
	size_t protected_len = cbor_bytestring_length(protected_header);
    
	if (protected_len == 0) {
		fprintf(stderr, "Protected header is empty\n");
		cbor_decref(&item);
		exit(2);
	}
    
	struct cbor_load_result ph_result;
	cbor_item_t *ph_item = cbor_load(protected_bytes, protected_len, &ph_result);
	if (ph_result.error.code != CBOR_ERR_NONE) {
		fprintf(stderr, "failed to decode protected header CBOR: %d\n", ph_result.error.code);
		cbor_decref(&item);
		exit(2);
	}
    
	// find algorithm ID
	int alg_id = -1;
	if (cbor_isa_map(ph_item)) {
		size_t ph_map_size = cbor_map_size(ph_item);
		struct cbor_pair *ph_pairs = cbor_map_handle(ph_item);
		
		for (size_t i = 0; i < ph_map_size; i++) {
			if (cbor_isa_uint(ph_pairs[i].key) && 
			    cbor_get_int(ph_pairs[i].key) == 1) {
				if (cbor_isa_negint(ph_pairs[i].value)) {
					alg_id = -1 - cbor_get_int(ph_pairs[i].value);
				} else if (cbor_isa_uint(ph_pairs[i].value)) {
					alg_id = cbor_get_int(ph_pairs[i].value);
				}
				printf("Signature algorithm ID: %d\n", alg_id);
				break;
			}
		}
	}
	cbor_decref(&ph_item);
    
	// payload
	unsigned char *attestation_doc = NULL;
	size_t attestation_doc_len = 0;
    
	if (cbor_isa_bytestring(payload)) {
		attestation_doc = cbor_bytestring_handle(payload);
		attestation_doc_len = cbor_bytestring_length(payload);
		printf("Attestation document size: %zu bytes\n", attestation_doc_len);
	} else {
		fprintf(stderr, "No payload found in COSE_Sign1\n");
		cbor_decref(&item);
		exit(2);
	}
    
	unsigned char *sig = cbor_bytestring_handle(signature);
	size_t sig_len = cbor_bytestring_length(signature);
	printf("Signature size: %zu bytes\n", sig_len);
	
	// 1. extract certificate from attestation document
	struct cbor_load_result ad_result;
	cbor_item_t *ad_item = cbor_load(attestation_doc, attestation_doc_len, &ad_result);
	if (ad_result.error.code != CBOR_ERR_NONE) {
		fprintf(stderr, "failed to decode attestation document CBOR: %d (position: %zu)\n", 
			ad_result.error.code, ad_result.error.position);
        
#ifdef DEBUG
		printf("First 16 bytes of attestation document: ");
		for (int i = 0; i < (attestation_doc_len > 16 ? 16 : attestation_doc_len); i++) {
			printf("%02x ", attestation_doc[i]);
		}
		printf("\n");
#endif        
		cbor_decref(&item);
		exit(2);
	}
    
	unsigned char *cert_data = NULL;
	size_t cert_len = 0;
    
	if (!cbor_isa_map(ad_item)) {
		fprintf(stderr, "Attestation document is not a map (actual type: %d)\n", 
			cbor_typeof(ad_item));
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	size_t map_size = cbor_map_size(ad_item);
	struct cbor_pair *pairs = cbor_map_handle(ad_item);
    
	printf("Attestation document map contains %zu key-value pairs\n", map_size);
    
#ifdef DEBUG
	for (size_t i = 0; i < map_size; i++) {
		if (cbor_isa_string(pairs[i].key)) {
			char *key_str = malloc(cbor_string_length(pairs[i].key) + 1);
			memcpy(key_str, cbor_string_handle(pairs[i].key), cbor_string_length(pairs[i].key));
			key_str[cbor_string_length(pairs[i].key)] = '\0';
			printf("Key: %s\n", key_str);
			free(key_str);
		} else {
			printf("Key: (not a string)\n");
		}
	}
#endif

	// search certificate
	for (size_t i = 0; i < map_size; i++) {
		if (cbor_isa_string(pairs[i].key)) {
			char *key_str = malloc(cbor_string_length(pairs[i].key) + 1);
			memcpy(key_str, cbor_string_handle(pairs[i].key), cbor_string_length(pairs[i].key));
			key_str[cbor_string_length(pairs[i].key)] = '\0';
            
			if ((strcmp(key_str, "certificate") == 0 || strcmp(key_str, "cert") == 0) && 
			    cbor_isa_bytestring(pairs[i].value)) {
				cert_data = cbor_bytestring_handle(pairs[i].value);
				cert_len = cbor_bytestring_length(pairs[i].value);
				printf("Found certificate with key '%s', length: %zu bytes\n", key_str, cert_len);
				free(key_str);
				break;
			}
			free(key_str);
		}
	}
    
	if (!cert_data || cert_len == 0) {
		fprintf(stderr, "Certificate not found in attestation document\n");
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	// 2. extract public key
	const unsigned char *cert_temp = cert_data;
	X509 *signer_cert = d2i_X509(NULL, &cert_temp, cert_len);
    
	if (!signer_cert) {
		fprintf(stderr, "failed to parse certificate: %s\n", 
			ERR_error_string(ERR_get_error(), NULL));
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	EVP_PKEY *pubkey = X509_get_pubkey(signer_cert);
	if (!pubkey) {
		fprintf(stderr, "failed to extract public key: %s\n", 
			ERR_error_string(ERR_get_error(), NULL));
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	int key_type = EVP_PKEY_id(pubkey);
#ifdef DEBUG
	printf("Public key type: %d (%s)\n", key_type, 
	       (key_type == EVP_PKEY_EC) ? "EC" : 
	       (key_type == EVP_PKEY_RSA) ? "RSA" : "Unknown");
#endif

	// 3. prepare
	EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
	if (!md_ctx) {
		fprintf(stderr, "failed to create EVP_MD_CTX\n");
		EVP_PKEY_free(pubkey);
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	const EVP_MD *md = NULL;
	switch (alg_id) {
        case -7:  // ES256
		md = EVP_sha256();
		break;
        case -35: // ES384
		md = EVP_sha384();
		break;
        case -36: // ES512
		md = EVP_sha512();
		break;
        default:
		printf("Unknown algorithm ID: %d, defaulting to SHA-384\n", alg_id);
		md = EVP_sha384(); // default value
		break;
	}
    
	if (EVP_DigestVerifyInit(md_ctx, NULL, md, NULL, pubkey) != 1) {
		fprintf(stderr, "failed to EVP_DigestVerifyInit: %s\n", 
			ERR_error_string(ERR_get_error(), NULL));
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_free(pubkey);
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	// 4. COSE_Sign1 (Sig_structure)
	cbor_item_t *sig_array = cbor_new_definite_array(4);
	_Bool	unused;

	unused = cbor_array_push(sig_array, cbor_build_string("Signature1"));
	unused = cbor_array_push(sig_array, cbor_build_bytestring(protected_bytes, protected_len));
	unused = cbor_array_push(sig_array, cbor_build_bytestring(NULL, 0)); // empty external_aad
	unused = cbor_array_push(sig_array, cbor_build_bytestring(attestation_doc, attestation_doc_len));
    
	unsigned char *sig_structure_buf = NULL;
	size_t sig_structure_len = 0;
    
	size_t estimate_size = protected_len + attestation_doc_len + 100;
	sig_structure_buf = malloc(estimate_size);
	if (!sig_structure_buf) {
		fprintf(stderr, "failed to allocate memory for sig_structure\n");
		cbor_decref(&sig_array);
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_free(pubkey);
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	sig_structure_len = cbor_serialize(sig_array, sig_structure_buf, estimate_size);
	if (sig_structure_len == 0 || sig_structure_len == SIZE_MAX) {
		fprintf(stderr, "failed to serialize sig_structure\n");
		free(sig_structure_buf);
		cbor_decref(&sig_array);
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_free(pubkey);
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	cbor_decref(&sig_array);
    
#ifdef DEBUG
	printf("Sig_structure length: %zu bytes\n", sig_structure_len);
	printf("Signature length: %zu bytes\n", sig_len);
#endif
	// 5. validate signature
	if (EVP_DigestVerifyUpdate(md_ctx, sig_structure_buf, sig_structure_len) != 1) {
		fprintf(stderr, "failed to EVP_DigestVerifyUpdate: %s\n", 
			ERR_error_string(ERR_get_error(), NULL));
		free(sig_structure_buf);
		EVP_MD_CTX_free(md_ctx);
		EVP_PKEY_free(pubkey);
		X509_free(signer_cert);
		cbor_decref(&ad_item);
		cbor_decref(&item);
		exit(2);
	}
    
	// check ECDSA signature format (DER vs raw) - Nitro is generally raw format?
	printf("First few bytes of signature: ");
	for (int i = 0; i < (sig_len > 8 ? 8 : sig_len); i++) {
		printf("%02x ", sig[i]);
	}
	printf("\n");
    
	// if ECDSA is raw format, then convert DER
	unsigned char *der_sig = NULL;
	size_t der_sig_len = 0;
    
	if (key_type == EVP_PKEY_EC && (sig_len == 64 || sig_len == 96 || sig_len == 132)) {
		// maybe raw signature (r||s format)
		// need to convert DER format
		printf("Converting raw ECDSA signature to DER format...\n");
        
		ECDSA_SIG *ecdsa_sig = ECDSA_SIG_new();
		if (!ecdsa_sig) {
			fprintf(stderr, "Failed to create ECDSA_SIG\n");
			free(sig_structure_buf);
			EVP_MD_CTX_free(md_ctx);
			EVP_PKEY_free(pubkey);
			X509_free(signer_cert);
			cbor_decref(&ad_item);
			cbor_decref(&item);
			exit(2);
		}
        
		BIGNUM *r = BN_new();
		BIGNUM *s = BN_new();
		if (!r || !s) {
			fprintf(stderr, "Failed to create BIGNUMs\n");
			ECDSA_SIG_free(ecdsa_sig);
			free(sig_structure_buf);
			EVP_MD_CTX_free(md_ctx);
			EVP_PKEY_free(pubkey);
			X509_free(signer_cert);
			cbor_decref(&ad_item);
			cbor_decref(&item);
			exit(2);
		}
        
		BN_bin2bn(sig, sig_len/2, r);
		BN_bin2bn(sig + sig_len/2, sig_len/2, s);
        
		if (ECDSA_SIG_set0(ecdsa_sig, r, s) != 1) {
			fprintf(stderr, "Failed to set r and s in ECDSA_SIG\n");
			BN_free(r);
			BN_free(s);
			ECDSA_SIG_free(ecdsa_sig);
			free(sig_structure_buf);
			EVP_MD_CTX_free(md_ctx);
			EVP_PKEY_free(pubkey);
			X509_free(signer_cert);
			cbor_decref(&ad_item);
			cbor_decref(&item);
			exit(2);
		}
        
		int der_len = i2d_ECDSA_SIG(ecdsa_sig, NULL);
		if (der_len <= 0) {
			fprintf(stderr, "Failed to calculate DER length\n");
			ECDSA_SIG_free(ecdsa_sig);
			free(sig_structure_buf);
			EVP_MD_CTX_free(md_ctx);
			EVP_PKEY_free(pubkey);
			X509_free(signer_cert);
			cbor_decref(&ad_item);
			cbor_decref(&item);
			exit(2);
		}
        
		der_sig = malloc(der_len);
		if (!der_sig) {
			fprintf(stderr, "Failed to allocate DER signature buffer\n");
			ECDSA_SIG_free(ecdsa_sig);
			free(sig_structure_buf);
			EVP_MD_CTX_free(md_ctx);
			EVP_PKEY_free(pubkey);
			X509_free(signer_cert);
			cbor_decref(&ad_item);
			cbor_decref(&item);
			exit(2);
		}
        
		unsigned char *p = der_sig;
		der_sig_len = i2d_ECDSA_SIG(ecdsa_sig, &p);
		ECDSA_SIG_free(ecdsa_sig);
        
		// DER signature
		printf("Converted to DER signature, length: %zu bytes\n", der_sig_len);
	} else {
		// using existing signature
		der_sig = sig;
		der_sig_len = sig_len;
	}
    
	// validate signature
	int verify_result = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
	if (verify_result != 1) {
		fprintf(stderr, "Failed to validate signature: %d (error: %s)\n", 
			verify_result, ERR_error_string(ERR_get_error(), NULL));
        
		printf("Trying with different hash algorithm...\n");
		EVP_MD_CTX_free(md_ctx);
		md_ctx = EVP_MD_CTX_new();
        
		// try with SHA-256
		if (md != EVP_sha256() && 
		    EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, pubkey) == 1 &&
		    EVP_DigestVerifyUpdate(md_ctx, sig_structure_buf, sig_structure_len) == 1) {
            
			verify_result = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
			if (verify_result == 1) {
				printf("Success with SHA-256!\n");
			}
		}
        
		// try with SHA-384
		if (verify_result != 1) {
			EVP_MD_CTX_free(md_ctx);
			md_ctx = EVP_MD_CTX_new();
            
			if (md != EVP_sha384() && 
			    EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha384(), NULL, pubkey) == 1 &&
			    EVP_DigestVerifyUpdate(md_ctx, sig_structure_buf, sig_structure_len) == 1) {
                
				verify_result = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
				if (verify_result == 1) {
					printf("Success with SHA-384!\n");
				}
			}
		}
        
		// try with SHA-512
		if (verify_result != 1) {
			EVP_MD_CTX_free(md_ctx);
			md_ctx = EVP_MD_CTX_new();
            
			if (md != EVP_sha512() && 
			    EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha512(), NULL, pubkey) == 1 &&
			    EVP_DigestVerifyUpdate(md_ctx, sig_structure_buf, sig_structure_len) == 1) {
                
				verify_result = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
				if (verify_result == 1) {
					printf("Success with SHA-512!\n");
				}
			}
		}
        
		//if failed, try with raw byte instead
		if (verify_result != 1) {
			printf("Trying direct verification with attestation document...\n");
			EVP_MD_CTX_free(md_ctx);
			md_ctx = EVP_MD_CTX_new();
            
			if (EVP_DigestVerifyInit(md_ctx, NULL, md, NULL, pubkey) == 1 &&
			    EVP_DigestVerifyUpdate(md_ctx, attestation_doc, attestation_doc_len) == 1) {
                
				verify_result = EVP_DigestVerifyFinal(md_ctx, der_sig, der_sig_len);
				if (verify_result == 1) {
					printf("Direct verification successful!\n");
				} else {
					fprintf(stderr, "All verification attempts failed: %s\n", 
						ERR_error_string(ERR_get_error(), NULL));
				}
			}
			exit(2);
		}
	} else {
		printf("Signature validation successful!\n");
	}

	EVP_PKEY *enclave_pubkey = NULL;
	// search public key
	for (size_t i = 0; i < map_size; i++) {
		if (cbor_isa_string(pairs[i].key)) {
			char *key_str = malloc(cbor_string_length(pairs[i].key) + 1);
			memcpy(key_str, cbor_string_handle(pairs[i].key), cbor_string_length(pairs[i].key));
			key_str[cbor_string_length(pairs[i].key)] = '\0';
			if (strcmp(key_str, "public_key") == 0 && cbor_isa_bytestring(pairs[i].value)) {
				unsigned char *enclave_pubkey_data = NULL;
				size_t enclave_pubkey_len = 0;

				enclave_pubkey_data = cbor_bytestring_handle(pairs[i].value);
				enclave_pubkey_len = cbor_bytestring_length(pairs[i].value);
				printf("Found enclave public key, length: %zu bytes\n", enclave_pubkey_len);
				free(key_str);

				const unsigned char *p = enclave_pubkey_data;
				enclave_pubkey = d2i_PUBKEY(NULL, &p, enclave_pubkey_len);
				break;
			}
			free(key_str);
		}
	}

	// clean up resources
	if (der_sig != sig) {
		free(der_sig);
	}
	free(sig_structure_buf);
	EVP_MD_CTX_free(md_ctx);
	X509_free(signer_cert);
	EVP_PKEY_free(pubkey);
	cbor_decref(&ad_item);
	cbor_decref(&item);

	return enclave_pubkey;
}
