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
        tcp_request->client_prox