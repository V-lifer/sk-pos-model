#include "dnscrypt.h"

struct SignedCert *
cert_build_cert(const uint8_t *crypt_publickey, int cert_file_expire_seconds,
                int use_xchacha20)
{
    struct SignedCert *signed_cert = malloc(sizeof(struct SignedCert));
    if (!signed_cert)
        return NULL;

    memcpy(signed_cert->magic_cert, CERT_MAGIC_CERT, 4);
    signed_cert->ver