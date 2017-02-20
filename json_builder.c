#define _GNU_SOURCE

#include "json_builder.h"
#include <jansson.h>
#include <netdb.h>
#include "init.h"
#include "lib.h"
#include "logger.h"
#include "string_builders.h"

/* Save reference to pointer with shorter name */
typedef int (*add_type)(json_t *o, const char *k, json_t *v);
static add_type add = &json_object_set_new;

static json_t *build_addr(const TcpAddr *addr) {
        json_t *json_addr = json_object();
        if (!json_addr) goto error;

        add(json_addr, "ip", json_string(addr->ip));
        add(json_addr, "hostname", json_string(addr->hostname));
        add(json_addr, "port", json_string(addr->port));
        add(json_addr, "service", json_string(addr->service));

        return json_addr;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_send_flags(const TcpSendFlags *flags) {
        json_t *json_flags = json_object();
        if (!json_flags) goto error;

        add(json_flags, "msg_confirm", json_boolean(flags->msg_confirm));
        add(json_flags, "msg_dontroute", json_boolean(flags->msg_dontroute));
        add(json_flags, "msg_dontwait", json_boolean(flags->msg_dontwait));
        add(json_flags, "msg_eor", json_boolean(flags->msg_eor));
        add(json_flags, "msg_more", json_boolean(flags->msg_more));
        add(json_flags, "msg_nosignal", json_boolean(flags->msg_nosignal));
        add(json_flags, "msg_oob", json_boolean(flags->msg_oob));

        return json_flags;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_recv_flags(const TcpRecvFlags *flags) {
        json_t *json_flags = json_object();
        if (!json_flags) goto error;

        add(json_flags, "msg_cmsg_cloexec",
            json_boolean(flags->msg_cmsg_cloexec));
        add(json_flags, "msg_dontwait", json_boolean(flags->msg_dontwait));
        add(json_flags, "msg_errqueue", json_boolean(flags->msg_errqueue));
        add(json_flags, "msg_oob", json_boolean(flags->msg_oob));
        add(json_flags, "msg_peek", json_boolean(flags->msg_peek));
        add(json_flags, "msg_trunc", json_boolean(flags->msg_trunc));
        add(json_flags, "msg_waitall", json_boolean(flags->msg_waitall));

        return json_flags;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_timeout(const TcpTimeout *timeout) {
        json_t *json_timeout = json_object();
        if (!json_timeout) goto error;

        add(json_timeout, "seconds", json_integer(timeout->seconds));
        add(json_timeout, "nanoseconds", json_integer(timeout->nanoseconds));

        return json_timeout;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_poll_events(const TcpPollEvents *events) {
        json_t *json_events = json_object();
        if (!json_events) goto error;

        add(json_events, "pollin", json_boolean(events->pollin));
        add(json_events, "pollpri", json_boolean(events->pollpri));
        add(json_events, "pollout", json_boolean(events->pollout));
        add(json_events, "pollrdhup", json_boolean(events->pollrdhup));
        add(json_events, "pollerr", json_boolean(events->pollerr));
        add(json_events, "pollhup", json_boolean(events->pollhup));
        add(json_events, "pollnval", json_boolean(events->pollnval));

        return json_events;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_select_events(const TcpSelectEvents *events) {
        json_t *json_events = json_object();
        if (!json_events) goto error;

        add(json_events, "read", json_boolean(events->read));
        add(json_events, "write", json_boolean(events->write));
        add(json_events, "except", json_boolean(events->except));

        return json_events;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_iovec(const TcpIovec *iovec) {
        json_t *json_iovec = json_object();
        if (!json_iovec) goto error;

        add(json_iovec, "iovec_count", json_integer(iovec->iovec_count));
        json_t *iovec_sizes = json_array();
        if (iovec_sizes) {
                for (int i = 0; i < iovec->iovec_count; i++) {
                        json_array_append_new(
                            iovec_sizes, json_integer(iovec->iovec_sizes[i]));
                }
        }
        add(json_iovec, "iovec_sizes", iovec_sizes);

        return json_iovec;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_msghdr(const TcpMsghdr *msg) {
        json_t *json_msghdr = json_object();
        if (!json_msghdr) goto error;

        add(json_msghdr, "control_data", json_boolean(msg->control_data));
        add(json_msghdr, "iovec", build_iovec(&msg->iovec));

        return json_msghdr;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_timeval(const struct timeval *tv) {
        json_t *json_timeval = json_object();
        if (!json_timeval) goto error;

        add(json_timeval, "tv_sec", json_integer(tv->tv_sec));
        add(json_timeval, "tv_usec", json_integer(tv->tv_usec));

        return json_timeval;
error:
        LOG(ERROR, "json_object() failed.");
        LOG_FUNC_FAIL;
        return NULL;
}

static json_t *build_optval(const TcpSockopt *sockopt) {
        switch (sockopt->level) {
                case SOL_SOCKET:
                        switch (sockopt->optname) {
                                case SO_RCVTIMEO:
                                case SO_SNDTIMEO:
                                        return build_timeval(
                                            (struct timeval *)sockopt->optval);
                                        break;
                        }
                        break;
        }
        return NULL;
}

static void build_shared_fields(json_t *json_ev, const TcpEvent *ev) {
        const char *type_str = string_from_tcp_event_type(ev->type);
        add(json_ev, "type", json_string(type_str));

        /* Time stamp */
        json_t *timestamp_json = json_object();
        if (timestamp_json) {
                add(timestamp_json, "sec", json_integer(ev->timestamp.tv_sec));
                add(timestamp_json, "usec",
                    json_integer(ev->timestamp.tv_usec));
        }
        add(json_ev, "timestamp", timestamp_json);

        /* Return value & err string */
        add(json_ev, "return_value", json_integer(ev->return_value));
        add(json_ev, "success", json_boolean(ev->success));
        add(json_ev, "error_str", json_string(ev->error_str));
}

#define DETAILS_FAILURE "json_object() failed. Cannot build event details."

#define BUILD_EV_PRELUDE()                                  \
        json_t *json_ev = json_object();                    \
        if (!json_ev) {                                     \
                LOG(ERROR, "json_object() failed.");        \
                LOG_FUNC_FAIL;                              \
                return NULL;                                \
        }                                                   \
        build_shared_fields(json_ev, (const TcpEvent *)ev); \
        json_t *json_details = json_object();               \
        if (json_details == NULL) {                         \
                LOG(ERROR, DETAILS_FAILURE);                \
                return json_ev;                             \
        }                                                   \
        add(json_ev, "details", json_details);

static json_t *build_tcp_ev_socket(const TcpEvSocket *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "domain", json_string(ev->domain_str));
        add(json_details, "type", json_string(ev->type_str));
        add(json_details, "protocol", json_integer(ev->protocol));
        add(json_details, "sock_cloexec", json_boolean(ev->sock_cloexec));
        add(json_details, "sock_nonblock", json_boolean(ev->sock_nonblock));

        return json_ev;
}

static json_t *build_tcp_ev_bind(const TcpEvBind *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "addr", build_addr(&ev->addr));

        return json_ev;
}

static json_t *build_tcp_ev_connect(const TcpEvConnect *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "addr", build_addr(&ev->addr));

        return json_ev;
}

static json_t *build_tcp_ev_shutdown(const TcpEvShutdown *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "shut_rd", json_boolean(ev->shut_rd));
        add(json_details, "shut_wr", json_boolean(ev->shut_wr));

        return json_ev;
}

static json_t *build_tcp_ev_listen(const TcpEvListen *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "backlog", json_integer(ev->backlog));

        return json_ev;
}

static json_t *build_tcp_ev_accept(const TcpEvAccept *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "addr", build_addr(&ev->addr));

        return json_ev;
}

static json_t *build_tcp_ev_getsockopt(const TcpEvGetsockopt *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "level", json_integer(ev->sockopt.level));
        add(json_details, "level_str", json_string(ev->sockopt.level_str));
        add(json_details, "optname", json_integer(ev->sockopt.optname));
        add(json_details, "optname_str", json_string(ev->sockopt.optname_str));
        add(json_details, "optval", build_optval(&ev->sockopt));

        return json_ev;
}

static json_t *build_tcp_ev_setsockopt(const TcpEvSetsockopt *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "level", json_integer(ev->sockopt.level));
        add(json_details, "level_str", json_string(ev->sockopt.level_str));
        add(json_details, "optname", json_integer(ev->sockopt.optname));
        add(json_details, "optname_str", json_string(ev->sockopt.optname_str));
        add(json_details, "optval", build_optval(&ev->sockopt));

        return json_ev;
}

static json_t *build_tcp_ev_send(const TcpEvSend *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_send_flags(&(ev->flags)));

        return json_ev;
}

static json_t *build_tcp_ev_recv(const TcpEvRecv *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_recv_flags(&(ev->flags)));

        return json_ev;
}

static json_t *build_tcp_ev_sendto(const TcpEvSendto *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        // char *addr_str = alloc_host_str(&(ev->addr));
        // char *port_str = alloc_port_str(&(ev->addr));

        // add(json_details, "addr", json_string(addr_str));
        // add(json_details, "port", json_string(port_str));
        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_send_flags(&(ev->flags)));

        // free(addr_str);
        // free(port_str);
        return json_ev;
}

static json_t *build_tcp_ev_recvfrom(const TcpEvRecvfrom *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        // char *addr_str = alloc_host_str(&(ev->addr));
        // char *port_str = alloc_port_str(&(ev->addr));

        // add(json_details, "addr", json_string(addr_str));
        // add(json_details, "port", json_string(port_str));
        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_recv_flags(&(ev->flags)));

        // free(addr_str);
        // free(port_str);
        return json_ev;
}

static json_t *build_tcp_ev_sendmsg(const TcpEvSendmsg *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_send_flags(&(ev->flags)));
        add(json_details, "msghdr", build_msghdr(&(ev->msghdr)));

        return json_ev;
}

static json_t *build_tcp_ev_recvmsg(const TcpEvRecvmsg *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "flags", build_recv_flags(&(ev->flags)));
        add(json_details, "msghdr", build_msghdr(&(ev->msghdr)));

        return json_ev;
}

#if !defined(__ANDROID__) || __ANDROID_API__ >= 21
static json_t *build_tcp_ev_sendmmsg(const TcpEvSendmmsg *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        return json_ev;
}

static json_t *build_tcp_ev_recvmmsg(const TcpEvRecvmmsg *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        return json_ev;
}
#endif

static json_t *build_tcp_ev_getsockname(const TcpEvGetsockname *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "addr", build_addr(&ev->addr));

        return json_ev;
}

static json_t *build_tcp_ev_write(const TcpEvWrite *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));

        return json_ev;
}

static json_t *build_tcp_ev_read(const TcpEvRead *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));

        return json_ev;
}

static json_t *build_tcp_ev_close(const TcpEvClose *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "detected", json_boolean(ev->detected));

        return json_ev;
}

static json_t *build_tcp_ev_dup(const TcpEvDup *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        return json_ev;
}

static json_t *build_tcp_ev_dup2(const TcpEvDup2 *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "newfd", json_integer(ev->newfd));

        return json_ev;
}

static json_t *build_tcp_ev_dup3(const TcpEvDup3 *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "newfd", json_integer(ev->newfd));
        add(json_details, "o_cloexec", json_boolean(ev->o_cloexec));

        return json_ev;
}

static json_t *build_tcp_ev_writev(const TcpEvWritev *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "iovec", build_iovec(&ev->iovec));

        return json_ev;
}

static json_t *build_tcp_ev_readv(const TcpEvReadv *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));
        add(json_details, "iovec", build_iovec(&ev->iovec));

        return json_ev;
}

static json_t *build_tcp_ev_ioctl(const TcpEvIoctl *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "request", json_integer(ev->request));

        return json_ev;
}

static json_t *build_tcp_ev_sendfile(const TcpEvSendfile *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "bytes", json_integer(ev->bytes));

        return json_ev;
}

static json_t *build_tcp_ev_poll(const TcpEvPoll *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "timeout", build_timeout(&ev->timeout));
        add(json_details, "requested_events",
            build_poll_events(&ev->requested_events));
        add(json_details, "returned_events",
            build_poll_events(&ev->returned_events));

        return json_ev;
}

static json_t *build_tcp_ev_ppoll(const TcpEvPpoll *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "timeout", build_timeout(&ev->timeout));
        add(json_details, "requested_events",
            build_poll_events(&ev->requested_events));
        add(json_details, "returned_events",
            build_poll_events(&ev->returned_events));

        return json_ev;
}

static json_t *build_tcp_ev_select(const TcpEvSelect *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "timeout", build_timeout(&ev->timeout));
        add(json_details, "requested_events",
            build_select_events(&ev->requested_events));
        add(json_details, "returned_events",
            build_select_events(&ev->returned_events));

        return json_ev;
}

static json_t *build_tcp_ev_fcntl(const TcpEvFcntl *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "cmd", json_string(ev->cmd));

        return json_ev;
}

static json_t *build_tcp_ev_pselect(const TcpEvPselect *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details

        add(json_details, "timeout", build_timeout(&ev->timeout));
        add(json_details, "requested_events",
            build_select_events(&ev->requested_events));
        add(json_details, "returned_events",
            build_select_events(&ev->returned_events));

        return json_ev;
}

static json_t *build_tcp_ev_tcp_info(const TcpEvTcpInfo *ev) {
        BUILD_EV_PRELUDE()  // Inst. json_t *json_ev & json_t *json_details
        struct tcp_info i = ev->info;

        add(json_details, "state", json_integer(i.tcpi_state));
        add(json_details, "ca_state", json_integer(i.tcpi_ca_state));
        add(json_details, "retransmits", json_integer(i.tcpi_retransmits));
        add(json_details, "probes", json_integer(i.tcpi_probes));
        add(json_details, "backoff", json_integer(i.tcpi_backoff));
        add(json_details, "options", json_integer(i.tcpi_options));
        add(json_details, "snd_wscale", json_integer(i.tcpi_snd_wscale));
        add(json_details, "rcv_wscale", json_integer(i.tcpi_rcv_wscale));

        add(json_details, "rto", json_integer(i.tcpi_rto));
        add(json_details, "ato", json_integer(i.tcpi_ato));
        add(json_details, "snd_mss", json_integer(i.tcpi_snd_mss));
        add(json_details, "rcv_mss", json_integer(i.tcpi_rcv_mss));

        add(json_details, "unacked", json_integer(i.tcpi_unacked));
        add(json_details, "sacked", json_integer(i.tcpi_sacked));
        add(json_details, "lost", json_integer(i.tcpi_lost));
        add(json_details, "retrans", json_integer(i.tcpi_retrans));
        add(json_details, "fackets", json_integer(i.tcpi_fackets));

        /* Times */
        add(json_details, "last_data_sent",
            json_integer(i.tcpi_last_data_sent));
        add(json_details, "last_ack_sent", json_integer(i.tcpi_last_ack_sent));
        add(json_details, "last_data_recv",
            json_integer(i.tcpi_last_data_recv));
        add(json_details, "last_ack_recv", json_integer(i.tcpi_last_ack_recv));

        /* Metrics */
        add(json_details, "pmtu", json_integer(i.tcpi_pmtu));
        add(json_details, "rcv_ssthresh", json_integer(i.tcpi_rcv_ssthresh));
        add(json_details, "rtt", json_integer(i.tcpi_rtt));
        add(json_details, "rttvar", json_integer(i.tcpi_rttvar));
        add(json_details, "snd_ssthresh", json_integer(i.tcpi_snd_ssthresh));
        add(json_details, "snd_cwnd", json_integer(i.tcpi_snd_cwnd));
        add(json_details, "advmss", json_integer(i.tcpi_advmss));
        add(json_details, "reordering", json_integer(i.tcpi_reordering));

        add(json_details, "rcv_rtt", json_integer(i.tcpi_rcv_rtt));
        add(json_details, "rcv_space", json_integer(i.tcpi_rcv_space));

        add(json_details, "total_retrans", json_integer(i.tcpi_total_retrans));

        return json_ev;
}

static json_t *build_tcp_ev(const TcpEvent *ev) {
        json_t *r;
        switch (ev->type) {
                case TCP_EV_SOCKET:
                        r = build_tcp_ev_socket((const TcpEvSocket *)ev);
                        break;
                case TCP_EV_BIND:
                        r = build_tcp_ev_bind((const TcpEvBind *)ev);
                        break;
                case TCP_EV_CONNECT:
                        r = build_tcp_ev_connect((const TcpEvConnect *)ev);
                        break;
                case TCP_EV_SHUTDOWN:
                        r = build_tcp_ev_shutdown((const TcpEvShutdown *)ev);
                        break;
                case TCP_EV_LISTEN:
                        r = build_tcp_ev_listen((const TcpEvListen *)ev);
                        break;
                case TCP_EV_ACCEPT:
                        r = build_tcp_ev_accept((const TcpEvAccept *)ev);
                        break;
                case TCP_EV_GETSOCKOPT:
                        r = build_tcp_ev_getsockopt(
                            (const TcpEvGetsockopt *)ev);
                        break;
                case TCP_EV_SETSOCKOPT:
                        r = build_tcp_ev_setsockopt(
                            (const TcpEvSetsockopt *)ev);
                        break;
                case TCP_EV_SEND:
                        r = build_tcp_ev_send((const TcpEvSend *)ev);
                        break;
                case TCP_EV_RECV:
                        r = build_tcp_ev_recv((const TcpEvRecv *)ev);
                        break;
                case TCP_EV_SENDTO:
                        r = build_tcp_ev_sendto((const TcpEvSendto *)ev);
                        break;
                case TCP_EV_RECVFROM:
                        r = build_tcp_ev_recvfrom((const TcpEvRecvfrom *)ev);
                        break;
                case TCP_EV_SENDMSG:
                        r = build_tcp_ev_sendmsg((const TcpEvSendmsg *)ev);
                        break;
                case TCP_EV_RECVMSG:
                        r = build_tcp_ev_recvmsg((const TcpEvRecvmsg *)ev);
                        break;
#if !defined(__ANDROID__) || __ANDROID_API__ >= 21
                case TCP_EV_SENDMMSG:
                        r = build_tcp_ev_sendmmsg((const TcpEvSendmmsg *)ev);
                        break;
                case TCP_EV_RECVMMSG:
                        r = build_tcp_ev_recvmmsg((const TcpEvRecvmmsg *)ev);
                        break;
#endif
                case TCP_EV_GETSOCKNAME:
                        r = build_tcp_ev_getsockname(
                            (const TcpEvGetsockname *)ev);
                        break;
                case TCP_EV_WRITE:
                        r = build_tcp_ev_write((const TcpEvWrite *)ev);
                        break;
                case TCP_EV_READ:
                        r = build_tcp_ev_read((const TcpEvRead *)ev);
                        break;
                case TCP_EV_CLOSE:
                        r = build_tcp_ev_close((const TcpEvClose *)ev);
                        break;
                case TCP_EV_DUP:
                        r = build_tcp_ev_dup((const TcpEvDup *)ev);
                        break;
                case TCP_EV_DUP2:
                        r = build_tcp_ev_dup2((const TcpEvDup2 *)ev);
                        break;
                case TCP_EV_DUP3:
                        r = build_tcp_ev_dup3((const TcpEvDup3 *)ev);
                        break;
                case TCP_EV_WRITEV:
                        r = build_tcp_ev_writev((const TcpEvWritev *)ev);
                        break;
                case TCP_EV_READV:
                        r = build_tcp_ev_readv((const TcpEvReadv *)ev);
                        break;
                case TCP_EV_IOCTL:
                        r = build_tcp_ev_ioctl((const TcpEvIoctl *)ev);
                        break;
                case TCP_EV_SENDFILE:
                        r = build_tcp_ev_sendfile((const TcpEvSendfile *)ev);
                        break;
                case TCP_EV_POLL:
                        r = build_tcp_ev_poll((const TcpEvPoll *)ev);
                        break;
                case TCP_EV_PPOLL:
                        r = build_tcp_ev_ppoll((const TcpEvPpoll *)ev);
                        break;
                case TCP_EV_SELECT:
                        r = build_tcp_ev_select((const TcpEvSelect *)ev);
                        break;
                case TCP_EV_PSELECT:
                        r = build_tcp_ev_pselect((const TcpEvPselect *)ev);
                        break;
                case TCP_EV_FCNTL:
                        r = build_tcp_ev_fcntl((const TcpEvFcntl *)ev);
                        break;
                case TCP_EV_TCP_INFO:
                        r = build_tcp_ev_tcp_info((const TcpEvTcpInfo *)ev);
                        break;
        }
        return r;
}

/* Public functions */

char *alloc_tcp_ev_json(const TcpEvent *ev) {
        json_t *json_ev = build_tcp_ev(ev);
        if (!json_ev) goto error;
        size_t flags = conf_opt_p ? JSON_INDENT(2) : 0;
        char *json_string = json_dumps(json_ev, flags);
        json_decref(json_ev);
        return json_string;
error:
        LOG_FUNC_FAIL;
        return NULL;
}
