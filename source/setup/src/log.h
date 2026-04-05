#ifndef INCL_log__h
#define INCL_log__h

#define LOG(logLevel, fmt, ...) \
    do {\
        sos_printf(fmt, ## __VA_ARGS__); \
        sos_msx("\r"); \
    } while(0)

#endif // INCL_log__h
