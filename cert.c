#include "dnscrypt.h"

struct SignedCert *
cert_build_cert(const uint8_t *crypt_publickey, int cert_file_expire_seconds,
                int use_xchacha20)
{
    struct SignedCert *signed_cert = malloc(sizeof(struct SignedCert));
    if (!signed_cert)
        return NULL;

    memcpy(signed_cert->magic_cert, CERT_MAGIC_CERT, 4);
    signed_cert->version_major[0] = 0;
    if (use_xchacha20) {
        signed_cert->version_major[1] = 2;
    } else {
        signed_cert->version_major[1] = 1;
    }
    signed_cert->version_minor[0] = 0;
    