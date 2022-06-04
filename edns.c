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
        name_component_len = dns_packet[offset];
        if ((name_component_len & 0xC0) == 0xC0) {
            name_component_len = 1U;
        }
        if (name_component_len >= dns_packet_len - offset - 1U) {
            return -1;
        }
        offset += name_component_len + 1U;
    } while (name_component_len != 0U);
    if (offset >= dns_packet_len) {
        return -1;
    }
    *offset_p = offset;

    return 0;
}

#define DNS_QTYPE_PLUS_QCLASS_LEN 4U

static ssize_t
edns_get_payload_size(const uint8_t *const dns_packet,
                      const size_t dns