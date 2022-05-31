#include "dnscrypt.h"

static int
_skip_name(const uint8_t *const dns_packet, const size_t dns_packet_len,
           size_t * const offset_