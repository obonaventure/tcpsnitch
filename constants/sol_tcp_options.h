static const IntStrPair SOL_TCP_OPTIONS[] = {
#ifdef TCP_NODELAY
    ADD(TCP_NODELAY),
#endif
#ifdef TCP_MAXSEG
    ADD(TCP_MAXSEG),
#endif
#ifdef TCP_CORK
    ADD(TCP_CORK),
#endif
#ifdef TCP_KEEPIDLE
    ADD(TCP_KEEPIDLE),
#endif
#ifdef TCP_KEEPINTVL
    ADD(TCP_KEEPINTVL),
#endif
#ifdef TCP_KEEPCNT
    ADD(TCP_KEEPCNT),
#endif
#ifdef TCP_SYNCNT
    ADD(TCP_SYNCNT),
#endif
#ifdef TCP_LINGER2
    ADD(TCP_LINGER2),
#endif
#ifdef TCP_DEFER_ACCEPT
    ADD(TCP_DEFER_ACCEPT),
#endif
#ifdef TCP_WINDOW_CLAMP
    ADD(TCP_WINDOW_CLAMP),
#endif
#ifdef TCP_INFO
    ADD(TCP_INFO),
#endif
#ifdef TCP_QUICKACK
    ADD(TCP_QUICKACK),
#endif
#ifdef TCP_CONGESTION
    ADD(TCP_CONGESTION),
#endif
#ifdef TCP_MD5SIG
    ADD(TCP_MD5SIG),
#endif
#ifdef TCP_THIN_LINEAR_TIMEOUTS
    ADD(TCP_THIN_LINEAR_TIMEOUTS),
#endif
#ifdef TCP_THIN_DUPACK
    ADD(TCP_THIN_DUPACK),
#endif
#ifdef TCP_USER_TIMEOUT
    ADD(TCP_USER_TIMEOUT),
#endif
#ifdef TCP_REPAIR
    ADD(TCP_REPAIR),
#endif
#ifdef TCP_REPAIR_QUEUE
    ADD(TCP_REPAIR_QUEUE),
#endif
#ifdef TCP_QUEUE_SEQ
    ADD(TCP_QUEUE_SEQ),
#endif
#ifdef TCP_REPAIR_OPTIONS
    ADD(TCP_REPAIR_OPTIONS),
#endif
#ifdef TCP_FASTOPEN
    ADD(TCP_FASTOPEN),
#endif
#ifdef TCP_TIMESTAMP
    ADD(TCP_TIMESTAMP),
#endif
#ifdef TCP_NOTSENT_LOWAT
    ADD(TCP_NOTSENT_LOWAT)
#endif
};
