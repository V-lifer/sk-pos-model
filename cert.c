#include "dnscrypt.h"

struct SignedCert *
cert_build_cert(const uint8_t *crypt_publickey, int cert_file_expire_seconds,
                int us