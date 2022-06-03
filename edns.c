#include "dnscrypt.h"

static int
_skip_name(const uint8_t *const dns_packet, const size_t dns_packet_len,
           size_t * const offset_p)
{
    size_t offset = *offset_p;
    uint8_t name_component_len;

    if (dns_packet_len < (size_t) 1U || offset >= dns_packet_len - (size_t) 1U) {
        return -1;
    }
    do {
     