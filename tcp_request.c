#include "dnscrypt.h"
#include "block.h"

static void
tcp_request_kill(TCPRequest *const tcp_request)
{
    if (tcp_request == NULL || tcp_request->status.is_dying) {
        return;
    }
    tcp_request->status.is_dying = 1;
    struct context *c;

    if (tcp_request->timeout_timer != NULL) {
        event_free(tcp_request->timeout_timer);
        tcp_request->timeout_timer = NULL;
    }
    if (tcp_request->client_proxy_bev != NULL) {
        bufferevent_free(tcp_request->client_proxy_bev);
        tcp_request->client_proxy_bev = NULL;
    }
    if (tcp_request->proxy_resolver_bev != NULL) {
        bufferevent_free(tcp_request->proxy_resolver_bev);
        tcp_request->proxy_resolver_bev = NULL;
    }
    if (tcp_request->proxy_resolver_query_evbuf != NULL) {
        evbuffer_free(tcp_request->proxy_resolver_query_evbuf);
        tcp_request->proxy_resolver_query_evbuf = NULL;
    }
    c = tcp_request->context;
    if (tcp_request->status.is_in_queue != 0) {
        debug_assert(!TAILQ_EMPTY(&c->tcp_request_queue));
        TAILQ_REMOVE(&c->tcp_request_queue, tcp_request, queue);
        debug_assert(c->connections > 0U);
        c->connections--;
    }
    tcp_request->context = NULL;
    free(tcp_request);
}

static void
tcp_tune(evutil_socket_t handle)
{
    if (handle == -1) {
        return;
    }

    setsockopt(handle, IPPROTO_IP, IP_TOS, (void *) (int []) {
               0x70}, sizeof(int));
#ifdef TCP_QUICKACK
    setsockopt(handle, IPPROTO_TCP, TCP_QUICKACK, (void *)(int[]) {
               1}, sizeof(int));
#else
    setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)(int[]) {
               1}, sizeof(int));
#endif
#if defined(__linux__) && defined(SO_REUSEPORT)
    setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, (void *)(int[]) {
               1}, sizeof(int));
#endif
}

static void
timeout_timer_cb(evutil_socket_t timeout_timer_handle, short ev_flags,
                 void *const tcp_request_)
{
    TCPRequest *const tcp_request = tcp_request_;

    (void)ev_flags;
    (void)timeout_timer_handle;
    logger(LOG_DEBUG, "resolver timeout (TCP)");
    tcp_request_kill(tcp_request);
}

int
tcp_listener_kill_oldest_request(struct context *c)
{
    if (TAILQ_EMPTY(&c->tcp_request_queue)) {
        return -1;
    }
    tcp_request_kill(TAILQ_FIRST(&c->tcp_request_queue));

    return 0;
}


/**
 * Return 0 if served.
 */
static int
self_serve_cert_file(struct context *c, struct dns_header *header,
                     size_t dns_query_len, size_t max_len, TCPRequest *tcp_request)
{
    uint8_t dns_query_len_buf[2];
    if (dnscrypt_self_serve_cert_file(c, header, &dns_query_len, max_len) == 0) {
        dns_query_len_buf[0] = (dns_query_len >> 8) & 0xff;
        dns_query_len_buf[1] = dns_query_len & 0xff;
        if (bufferevent_write(tcp_request->client_proxy_bev,
                        dns_query_len_buf, (size_t) 2U) != 0 ||
            bufferevent_write(tcp_request->client_proxy_bev, (void *)header,
                            (size_t)dns_query_len) != 0) {
            tcp_request_kill(tcp_request);
            return -1;
        }
        bufferevent_enable(tcp_request->client_proxy_bev, EV_WRITE);
        bufferevent_free(tcp_request->proxy_resolver_bev);
        tcp_request->proxy_resolver_bev = NULL;
        return 0;
    }
    return -1;
}

static void
client_proxy_read_cb(struct bufferevent *const client_proxy_bev,
                     void *const tcp_request_)
{
    const size_t sizeof_dns_query = DNS_MAX_PACKET_SIZE_TCP - 2U;
    static uint8_t *dns_query = NULL;
    uint8_t dns_query_len_buf[2];
    uint8_t dns_curved_query_len_buf[2];
    TCPRequest *tcp_request = tcp_request_;
    struct context *c = tcp_request->context;
    struct evbuffer *input = bufferevent_get_input(client_proxy_bev);
    size_t available_size;
    size_t dns_query_len;
    size_t max_query_size;

    if (dns_query == NULL && (dns_query = sodium_malloc(sizeof_dns_query)) == NULL) {
        return;
    }
    if (tcp_request->status.has_dns_query_len == 0) {
        debug_assert(evbuffer_get_length(input) >= (size_t) 2U);
        evbuffer_remove(input, dns_query_len_buf, sizeof dns_query_len_buf);
        tcp_request->dns_query_len = (size_t)
            ((dns_query_len_buf[0] << 8) | dns_query_len_buf[1]);
        tcp_request->status.has_dns_query_len = 1;
    }
    debug_assert(tcp_request->status.has_dns_query_len != 0);
    dns_query_len = tcp_request->dns_query_len;
    if (dns_query_len < (size_t) DNS_HEADER_SIZE) {
        logger(LOG_DEBUG, "Short query received");
        tcp_request_kill(tcp_request);
        return;
    }
    available_size = evbuffer_get_length(input);
    if (available_size < dns_query_len) {
        bufferevent_setwatermark(tcp_request->client_proxy_bev,
                                 EV_READ, dns_query_len, dns_query_len);
        return;
    }
    debug_assert(available_size >= dns_query_len);
    bufferevent_disable(tcp_request->client_proxy_bev, EV_READ);
    debug_assert(tcp_request->proxy_resolver_query_evbuf == NULL);
    if ((tcp_request->proxy_resolver_query_evbuf = evbuffer_new()) == NULL) {
        tcp_request_kill(tcp_request);
        return;
    }
    if ((ssize_t)
        evbuffer_remove_buffer(input, tcp_request->proxy_resolver_query_evbuf,
                               dns_query_len) != (ssize_t) dns_query_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    debug_assert(dns_query_len <= sizeof_dns_query);
    if ((ssize_t) evbuffer_remove(tcp_request->proxy_resolver_query_evbuf,
                                  dns_query, dns_query_len)
        != (ssize_t) dns_query_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    max_query_size = sizeof_dns_query;
    debug_assert(max_query_size < DNS_MAX_PACKET_SIZE_TCP);
    debug_assert(SIZE_MAX - DNSCRYPT_MAX_PADDING - DNSCRYPT_QUERY_HEADER_SIZE
              > dns_query_len);
    size_t max_len =
        dns_query_len + DNSCRYPT_MAX_PADDING + DNSCRYPT_QUERY_HEADER_SIZE;
    if (max_len > max_query_size) {
        max_len = max_query_size;
    }
    if (dns_query_len + DNSCRYPT_QUERY_HEADER_SIZE > max_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    debug_assert(max_len <= DNS_MAX_PACKET_SIZE_TCP - 2U);
    debug_assert(max_len <= sizeof_dns_query);
    debug_assert(dns_query_len <= max_len);

    // decrypt if encrypted
    struct dnscrypt_query_header *dnscrypt_header =
        (struct dnscrypt_query_header *)dns_query;
    debug_assert(sizeof c->keypairs[0].crypt_publickey >= DNSCRYPT_MAGIC_HEADER_LEN);
    if ((tcp_request->cert =
         find_cert(c, dnscrypt_header->magic_query, dns_query_len)) == NULL) {
        tcp_request->is_dnscrypted = false;
    } else {
        if (dnscrypt_server_uncurve(c, tcp_request->cert,
                                    tcp_request->client_nonce,
                                    tcp_request->nmkey, dns_query,
                                    &dns_query_len) != 0 || dns_query_len < DNS_HEADER_SIZE) {
            logger(LOG_DEBUG, "Received a suspicious query from the client");
            tcp_request_kill(tcp_request);
            return;
        }
        tcp_request->is_dnscrypted = true;
    }

    struct dns_header *header = (struct dns_header *)dns_query;
    // self serve signed certificate for provider name?
    if (!tcp_request->is_dnscrypted) {
        if (self_serve_cert_file(c, header, dns_query_len, sizeof_dns_query, tcp_request) == 0)
            return;
        if (!c->allow_not_dnscrypted) {
            logger(LOG_DEBUG, "Unauthenticated query received over TCP");
            tcp_request_kill(tcp_request);
            return;
        }
    }

    tcp_request->is_blocked = is_blocked(c, header, dns_query_len);

    dns_curved_query_len_buf[0] = (dns_query_len >> 8) & 0xff;
    dns_curved_query_len_buf[1] = dns_query_len & 0xff;
    if (bufferevent_write(tcp_request->proxy_resolver_bev,
                          dns_curved_query_len_buf, (size_t) 2U) != 0 ||
        bufferevent_write(tcp_request->proxy_resolver_bev, dns_query,
                          (size_t) dns_query_len) != 0) {
        tcp_request_kill(tcp_request);
        return;
    }

    bufferevent_enable(tcp_request->proxy_resolver_bev, EV_READ);
}

static void
client_proxy_event_cb(struct bufferevent *const client_proxy_bev,
                      const short events, void *const tcp_request_)
{
    TCPRequest *const tcp_request = tcp_request_;

    (void)client_proxy_bev;
    (void)events;
    tcp_request_kill(tcp_request);
}

static void
client_proxy_write_cb(struct bufferevent *const client_proxy_bev,
                      void *const tcp_request_)
{
    TCPRequest *const tcp_request = tcp_request_;

    (void)client_proxy_bev;
    tcp_request_kill(tcp_request);
}

static void
proxy_resolver_event_cb(struct bufferevent *const proxy_resolver_bev,
                        const short events, void *const tcp_request_)
{
    TCPRequest *const tcp_request = tcp_request_;

    (void)proxy_resolver_bev;
    if ((events & BEV_EVENT_ERROR) != 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    if ((events & BEV_EVENT_CONNECTED) == 0) {
        tcp_tune(bufferevent_getfd(proxy_resolver_bev));
        return;
    }
}

static void
resolver_proxy_read_cb(struct bufferevent *const proxy_resolver_bev,
                       void *const tcp_request_)
{
    uint8_t dns_reply_len_buf[2];
    uint8_t dns_curved_reply_len_buf[2];
    uint8_t *dns_reply_bev;
    TCPRequest *tcp_request = tcp_request_;
    struct context *c = tcp_request->context;
    struct evbuffer *input = bufferevent_get_input(proxy_resolver_bev);
    size_t available_size;
    const size_t sizeof_dns_reply = DNS_MAX_PACKET_SIZE_TCP - 2U;
    static uint8_t *dns_reply = NULL;
    size_t dns_reply_len;

    if (dns_reply == NULL && (dns_reply = sodium_malloc(sizeof_dns_reply)) == NULL) {
        tcp_request_kill(tcp_request);
        return;
    }
    logger(LOG_DEBUG, "Resolver read callback.");
    if (tcp_request->status.has_dns_reply_len == 0) {
        debug_assert(evbuffer_get_length(input) >= (size_t) 2U);
        evbuffer_remove(input, dns_reply_len_buf, sizeof dns_reply_len_buf);
        tcp_request->dns_reply_len = (size_t)
            ((dns_reply_len_buf[0] << 8) | dns_reply_len_buf[1]);
        tcp_request->status.has_dns_reply_len = 1;
    }
    debug_assert(tcp_request->status.has_dns_reply_len != 0);
    dns_reply_len = tcp_request->dns_reply_len;
    if (dns_reply_len < (size_t) DNS_HEADER_SIZE) {
        logger(LOG_WARNING, "Short reply received");
        tcp_request_kill(tcp_request);
        return;
    }
    available_size = evbuffer_get_length(input);
    if (available_size < dns_reply_len) {
        bufferevent_setwatermark(tcp_request->proxy_resolver_bev,
                                 EV_READ, dns_reply_len, dns_reply_len);
        return;
    }
    debug_assert(available_size >= dns_reply_len);
    dns_reply_bev = evbuffer_pullup(input, (ssize_t) dns_reply_len);
    if (dns_reply_bev == NULL) {
        tcp_request_kill(tcp_request);
        return;
