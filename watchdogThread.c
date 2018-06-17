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

#include "socketThread.h"
#include "watchdogThread.h"
#include "main.h"

int watchdog_enable;
//Como los script http de python se paran con ctrl+z las funciones no devuelven error de read y write, por lo tanto se implemente un thread de watchdog
pthread_mutex_t mutexData_watchsocket = PTHREAD_MUTEX_INITIALIZER;

//Extern para el mutex del estado del proceso, este thread marca el status del socket y  lo para si expiro
extern pthread_mutex_t mutexData_process_status ;
extern process_variables_t serial_manager_status;

//Funcion para habilitar el watchdog
void watchdog_socket_tcp_enable (void){
	pthread_mutex_lock(&mutexData_watchsocket);
	watchdog_enable = WATCHDOG_ENABLE;
	pthread_mutex_unlock(&mutexData_watchsocket);
}

//Funcion para ser llamada desde afuera para que la funcion usuario los deshabilita
void watchdog_socket_tcp_disabled (void){
	pthread_mutex_lock(&mutexData_watchsocket);
	watchdog_enable = WATCHDOG_DISABLED;
	pthread_mutex_unlock(&mutexData_watchsocket);
}

//Funcion para obtener el estado del watchdog manejando los mutex
int get_watchdog_socket_tcp_status (void){
	int temp;

	pthread_mutex_lock(&mutexData_watchsocket);
	temp = watchdog_enable;
	pthread_mutex_unlock(&mutexData_watchsocket);
	return temp;
}

//Funcion principal de este thread, que habilita el watchdog quedando a la espera de que finalice el sleep fijo en esta implementacion
//Si no tenemos el disabled en el watchdog se finaloiza el proceso mandando el mensaje correspondiente para que lo atiene el
//thread principal.
void* watchdog_socket_tcp (void * socket_thread_parameter){
		pthread_mutex_lock(&mutexData_watchsocket);
		watchdog_enable = WATCHDOG_ENABLE;
		pthread_mutex_unlock(&mutexData_watchsocket);

		//printf("WATCHDOG:INICIO DEL SLEEP con WATCHDOG Valor decimal %d en string %s\n",get_watchdog_socket_tcp_status(),get_watchdog_socket_tcp_status()==WATCHDOG_ENABLE ? "WATCHDOG ENABLE": "WATCHDOG DISABLED");
		sleep(WATCHDOG_TIMEOUT);
		//printf("WATCHDOG:FIN DEL SLEEP con WATCHDOG Valor decimal %d en string %s\n",get_watchdog_socket_tcp_status(),get_watchdog_socket_tcp_status()==WATCHDOG_ENABLE ? "WATCHDOG ENABLE": "WATCHDOG DISABLED");

		if(get_watchdog_socket_tcp_status()==WATCHDOG_ENABLE){
			pthread_mutex_lock(&mutexData_process_status);
			serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
			pthread_mutex_unlock(&mutexData_process_status);
			printf("WATCHDOG READ TCP: Timeout expiro comunicación en el cliente\n");
			FOREVER_LOOP
		}
		return NULL;
}
