#define BAUD_RATE 115200
#define RECEIVER_MESSAGE_LENGTH 10
#define BUFFER_SIZE 128
#define RECEIVE_QUEUE_LENGTH 50
#define SEND_QUEUE_LENGTH 50
#define RECEIVER_MESSAGE_LENGTH 10

#define THREAD_OK 0
#define THREAD_NOT_OK 1
#define ALL_THREADS_RUNNING 0
#define WAIT_INIT 1

#define FOREVER_LOOP while(1){;};

typedef struct {
	int serial_com_port;
	int socket;
	int thread_receive_serial_status;
	int thread_send_serial_status;
	int thread_socket_status;
	int process_status;
} process_variables_t;
