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
tcp_l