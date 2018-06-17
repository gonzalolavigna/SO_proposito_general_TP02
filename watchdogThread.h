#define WATCHDOG_ENABLE 0
#define WATCHDOG_DISABLED 1
#define WATCHDOG_TIMEOUT 5

void* watchdog_socket_tcp (void * socket_thread_parameter);
void watchdog_socket_tcp_enable (void);
void watchdog_socket_tcp_disabled (void);

