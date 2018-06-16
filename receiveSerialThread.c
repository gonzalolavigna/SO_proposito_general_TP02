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
#include "main.h"
#include "receiveSerialThread.h"

#define RECEIVER_MESSAGE_LENGTH 10

extern Message_Queu_t receive_queue [RECEIVE_QUEUE_LENGTH];
extern process_variables_t serial_manager_status;
extern pthread_mutex_t mutexData_receive_serial;
extern pthread_mutex_t mutexData_process_status ;


static void check_receiver_message (int receiver_serial_n_char,char * receiver_serial_buffer_receiver);
static int write_serial_queue (char * message_to_transfer, int message_type, int size);

void* console_interface (void * console_number_void){
	char 	receiver_serial_buffer_receiver[BUFFER_SIZE];
	int 	receiver_serial_n_char;

	int console_number = *((int*) console_number_void);

	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.thread_receive_serial_status = THREAD_OK ;
	pthread_mutex_unlock(&mutexData_process_status);

	bzero(&receiver_serial_buffer_receiver[0],10);
	while(1){
		if((receiver_serial_n_char = serial_receive(receiver_serial_buffer_receiver,128))<0){
			pthread_mutex_lock(&mutexData_process_status);
			serial_manager_status.thread_receive_serial_status = THREAD_NOT_OK ;
			pthread_mutex_unlock(&mutexData_process_status);
			FOREVER_LOOP
		}
		check_receiver_message(receiver_serial_n_char,receiver_serial_buffer_receiver);
	}
}


static void check_receiver_message (int receiver_serial_n_char,char * receiver_serial_buffer_receiver) {

static 	int 	acc_receiver_serial_n_char = 0;
static  char 	receiver_meesage[128];

char message_to_transfer[11];

int i = 0;

	if(receiver_serial_n_char == 0){
		return;
	}

	strncat(receiver_meesage,receiver_serial_buffer_receiver,receiver_serial_n_char);
	acc_receiver_serial_n_char += receiver_serial_n_char;
	//Check start of a message
	for (i = 0; (i < strlen(receiver_meesage)) && (acc_receiver_serial_n_char > 0) ;i++){
		if(receiver_meesage[0] == '>'){
			if(strncmp(receiver_meesage,">OK\r\n",5) == 0){
				strncpy(message_to_transfer,receiver_meesage,5);
				message_to_transfer[5] = '\0';
				if(write_serial_queue(message_to_transfer,MESSAGE_TYPE_OK,6)== -1)
					return;
				acc_receiver_serial_n_char -= 5;
				if(acc_receiver_serial_n_char > 0)
					strncpy(&receiver_meesage[0],&receiver_meesage[5],acc_receiver_serial_n_char);
				printf("SERIAL RECEIVER:OK Recibido por el puerto serie\n");
			}
			else if(((strncmp(receiver_meesage,">ID:",4) == 0)) && (strncmp(&receiver_meesage[8],"\r\n",2)== 0)) {
				strncpy(message_to_transfer,receiver_meesage,10);
				message_to_transfer[10] = '\0';
				if(write_serial_queue(message_to_transfer,MESSAGE_TYPE_ID,11)== -1)
					return;
				acc_receiver_serial_n_char -= 10;
				if(acc_receiver_serial_n_char > 0)
					strncpy(&receiver_meesage[0],&receiver_meesage[10],acc_receiver_serial_n_char);
				printf("SERIAL RECEIVER:Mensaje Recibido por el puerto serie:%s",message_to_transfer);
			}
			else if (acc_receiver_serial_n_char >= 10){
				acc_receiver_serial_n_char--;
				strncpy(&receiver_meesage[0],&receiver_meesage[1],acc_receiver_serial_n_char);
			}
			else {
				break;
			}
		}else { //Hago un shift <<1 del string
			acc_receiver_serial_n_char--;
			strncpy(&receiver_meesage[0],&receiver_meesage[1],acc_receiver_serial_n_char);
		}
	}

if (acc_receiver_serial_n_char == 0){
	bzero(receiver_meesage,128);
}

}

static int write_serial_queue (char * message_to_transfer, int message_type, int size){
	int index;
	pthread_mutex_lock(&mutexData_receive_serial);
	if((index=cd_getFreeIndex(receive_queue,RECEIVE_QUEUE_LENGTH))== -1){
		printf("Receiver Queue Llena Exit");
		return -1;
	}

	if(cd_write_message_qeue(receive_queue,message_type,message_to_transfer,size,index)==-1){
		printf("Error Escribiendo Mensaje en la cola");
		return -1;
	}
	pthread_mutex_unlock(&mutexData_receive_serial);
	return 0;
}
