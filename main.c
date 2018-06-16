#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "SerialManager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#include "main.h"
#include "MessageQueue.h"
#include "receiveSerialThread.h"
#include "sendSerialThread.h"
#include "socketThread.h"




process_variables_t serial_manager_status;
Message_Queu_t receive_queue [RECEIVE_QUEUE_LENGTH];
Message_Queu_t send_queue [SEND_QUEUE_LENGTH];

int serial_com_init (char * console);
int parse_argument (int argc);
void bloquear_signals (void);
void desbloquear_signals (void);
int check_thread_integrity (process_variables_t * process_status);
void exit_function (process_variables_t * process_status);

pthread_mutex_t mutexData_receive_serial = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexData_process_status = PTHREAD_MUTEX_INITIALIZER;

void sigint_handler (int sig){
	write(1,"SIGINT recibida finalizando proceso\n",37);
	exit_function(&serial_manager_status);
	exit(2);
}

void sigpipe_handler (int sig){
    write(1,"SIGPIPE recibida finalizando proceso\n",38);
    exit_function(&serial_manager_status);
    exit(2);
}


int main(int argc, char **argv)
{
	int serial_com_port;
	int process_status;
	pthread_t thread_receive_serial;
	pthread_t thread_send_serial;
	pthread_t thread_socket;

	struct sigaction sa_1;
	struct sigaction sa_2;

	serial_manager_status.process_status = WAIT_INIT ;
	serial_manager_status.thread_receive_serial_status = THREAD_NOT_OK;
	serial_manager_status.thread_send_serial_status = THREAD_NOT_OK;
	serial_manager_status.thread_socket_status = THREAD_NOT_OK;


    sa_1.sa_handler = sigint_handler;
    sa_1.sa_flags = 0;
    sigemptyset(&sa_1.sa_mask);
    if(sigaction(SIGINT,&sa_1,NULL) == -1){
        printf("ERROR configurando SIGINT\n");
        return 1;
    }

    sa_2.sa_handler = sigpipe_handler;
    sa_2.sa_flags = 0;
    sigemptyset(&sa_2.sa_mask);

    if(sigaction(SIGPIPE,&sa_2,NULL) == -1){
        printf("ERROR configurando SIGPIPE\n");
        exit(1);
    }
	printf("SERIAL SEVICE SUCCESS:Inicio Serial Service\r\n");

	if (parse_argument(argc) == -1){
		exit(EXIT_FAILURE);
		return -1;
	}
	if((serial_com_port = serial_com_init (argv[1]))<0){
		exit(EXIT_FAILURE);
		return -1;
	}

	serial_manager_status.serial_com_port = serial_com_port;
	cd_init(receive_queue,RECEIVE_QUEUE_LENGTH);


	//console_interface(serial_com_port);
	bloquear_signals();
	pthread_create(&thread_receive_serial,NULL,console_interface,(void *)&serial_com_port);
	pthread_create(&thread_send_serial,NULL,send_console_interface,(void *)&serial_com_port);
	pthread_create(&thread_socket,NULL,socket_tcp_server,NULL);
	desbloquear_signals();

	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.process_status = ALL_THREADS_RUNNING ;
	pthread_mutex_unlock(&mutexData_process_status);

	usleep(500000);
	while (1){
		if((process_status = check_thread_integrity(&serial_manager_status))== -1){
			printf("MAIN:Error en los Threads\n");
			exit_function(&serial_manager_status);
			exit(EXIT_FAILURE);
			return 1;
		}
		usleep(500000);
	}

	exit(EXIT_SUCCESS);
	return 0;
}

int parse_argument (int argc){
	if (argc != 2){
		printf("SERIAL SEVICE ERROR: Numero Incorrecto de parametros, solo se acepta 1\r\n");
		return -1;
	}
	return 0;
}

int serial_com_init (char * console_argument){
	int console_number;

	if(strlen(console_argument) > 2 || strlen(console_argument) < 1){
		printf("SERIAL SEVICE ERROR: Caracteres Parametros de consola > a 2\r\n");
		return -1;
	}


	if(!isdigit(console_argument[0])  && !isdigit(console_argument[1])){
		printf("SERIAL SEVICE ERROR: Consola no es un numero\r\n");
		return -1;
	}
	console_number = strtol(console_argument,NULL,10);

	if (console_number > 21 && console_number < 0){
		printf("SERIAL SERVICE ERROR: Consola Elegida %d fuera de rango\r\n",console_number);
		return -1;
	}
	if(serial_open(console_number,BAUD_RATE) != 0){
		printf("SERIAL SERVICE ERROR: Error Opening Console %d a un BAUD RATE de %d\r\n",console_number,BAUD_RATE);
		return -1;
	}
	printf("SERIAL SEVICE SUCCESS: Inicializando Puerto Serie TTYUSB%d BAUD RATE:%d\r\n\n\n",console_number,BAUD_RATE);
	return console_number;
}

//Funcion para bloquear las señales
void bloquear_signals (void){
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK,&set,NULL);
}

//Funcion para desbloquear las señales
void desbloquear_signals (void){
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_UNBLOCK,&set,NULL);
}

int check_thread_integrity (process_variables_t * process_status){
	pthread_mutex_lock(&mutexData_process_status);
	int process_status_flag = 0;
	if(process_status->thread_receive_serial_status == THREAD_NOT_OK){
		process_status_flag = -1;
		printf("MAIN:RECEIVER THREAD WITH ERROR, exit\n");
	}
	if(process_status->thread_send_serial_status == THREAD_NOT_OK){
		process_status_flag = -1;
		printf("MAIN:SEND THREAD WITH ERROR, exit\n");
	}
	if(process_status->thread_socket_status == THREAD_NOT_OK){
		process_status_flag = -1;
		printf("MAIN:SOCKET THREAD WITH ERROR, exit\n");
	}
	pthread_mutex_unlock(&mutexData_process_status);
	return process_status_flag;
}

void exit_function (process_variables_t * process_status){
	if(process_status->process_status == ALL_THREADS_RUNNING){
		serial_close();
		close(process_status->socket);

	}
	else {
		serial_close();
	}
}
