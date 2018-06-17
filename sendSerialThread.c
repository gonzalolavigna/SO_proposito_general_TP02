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

#include "MessageQueue.h"
#include "sendSerialThread.h"
#include "main.h"



int wait_command (void);
int wait_response (void);

//Extern para poder comunicarse con losm utex tanto de la cola tambien como los del proceso.
//Extern de la cola de mensajes.

extern Message_Queu_t receive_queue [RECEIVE_QUEUE_LENGTH];
extern process_variables_t serial_manager_status;
extern pthread_mutex_t mutexData_receive_serial;
extern pthread_mutex_t mutexData_process_status ;

//Esta funcion queda a la espera de un comando a ser enviado a la placa por la ocla de mensajes.
//Cuando lo recibe lo envia a traves del puerto serie
//A continuacion queda a la espera de recibir el OK, en dicho caso indica transaccion completa.
//Vuelve al estado inicial WAIT_CMD en el que espera un comando por la cola de mensajes.

void* send_console_interface (void * console_number_void){
	int console_number = *((int*) console_number_void);
	int return_flag;
	int status = WAIT_CMD;

	int debug_queu_free_spaces;

	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.thread_send_serial_status = THREAD_OK ;
	pthread_mutex_unlock(&mutexData_process_status);

	while (1){

		if((status == WAIT_CMD) && ((return_flag = wait_command())== 0)){
			status = WAIT_OK;
		}
		if((status == WAIT_OK) && ((return_flag = wait_response())== 0) ){
			debug_queu_free_spaces = cd_getClientOcuppation(receive_queue,RECEIVE_QUEUE_LENGTH);
			printf("SERIAL SENDER:Espacio libre en la cola de mensajes %d\n",debug_queu_free_spaces);
			printf("SERIAL SENDER:Transacción completa\n\n\n");
			status = WAIT_CMD;
		}

		usleep(500000);
	}

}

int wait_command (void){
	int command_index;
	int command_send_size;
	char command_buffer[RECEIVER_MESSAGE_LENGTH];

	pthread_mutex_lock(&mutexData_receive_serial);
	if ((command_index =cd_get_first_non_free_index_meessage_type(receive_queue,MESSAGE_TYPE_COMMAND,RECEIVE_QUEUE_LENGTH)) != -1 ){
		command_send_size = cd_read_message_qeue(receive_queue,MESSAGE_TYPE_COMMAND,command_buffer,command_index);
		pthread_mutex_unlock(&mutexData_receive_serial);
		command_buffer[command_send_size] = '\0';
		serial_send(command_buffer,command_send_size);
		printf("SERIAL SENDER:Se recibio Comando por la cola\n");
		printf("SERIAL SENDER: Se recibio por la cola %d Bytes el string %s",command_send_size,command_buffer);
		printf("SERIAL SENDER: Se enviaron por el puerto serie %d Bytes el string %s",command_send_size,command_buffer);

//		return_flag = wait_response();
		return 0;
	}
	pthread_mutex_unlock(&mutexData_receive_serial);
	return -1;
}

int wait_response (void){
	int respond_index;
	int respond_size;
	char respond_buffer[RECEIVER_MESSAGE_LENGTH];
	int retries = 0;

	pthread_mutex_lock(&mutexData_receive_serial);
	for(retries=0;retries < 10; retries ++ ){
		if((respond_index = cd_get_first_non_free_index_meessage_type(receive_queue,MESSAGE_TYPE_OK,RECEIVE_QUEUE_LENGTH)) != -1){
			respond_size = cd_read_message_qeue(receive_queue,MESSAGE_TYPE_OK,respond_buffer,respond_index);
			pthread_mutex_unlock(&mutexData_receive_serial);
			respond_buffer[respond_size] = '\0';
			printf("SERIAL SENDER:Se recibio por la cola %d Bytes el string %s",respond_size,respond_buffer);
			return 0;
		}
		pthread_mutex_unlock(&mutexData_receive_serial);
		usleep(500000);
	}
	pthread_mutex_unlock(&mutexData_receive_serial);
	printf("SERIAL SENDER: Maximo Retries para recibir OK expirado\n");
	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.thread_send_serial_status = THREAD_NOT_OK ;
	pthread_mutex_unlock(&mutexData_process_status);
	FOREVER_LOOP
	return -1;
}
