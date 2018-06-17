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
#include "socketThread.h"
#include "main.h"


extern Message_Queu_t receive_queue [RECEIVE_QUEUE_LENGTH];
extern process_variables_t serial_manager_status;
extern pthread_mutex_t mutexData_receive_serial;
extern pthread_mutex_t mutexData_process_status ;

int write_string_to_socket (int fd);
void read_string_from_socket (int fd);


void* socket_tcp_server (void * socket_thread_parameter){
	socklen_t addr_len;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;
	int newfd;


	int s = socket(AF_INET,SOCK_STREAM, 0);

	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.socket = s ;
	serial_manager_status.thread_socket_status = THREAD_OK;
	pthread_mutex_unlock(&mutexData_process_status);

	bzero((char*) &serveraddr, sizeof(serveraddr));
   	serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(10000);
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(serveraddr.sin_addr.s_addr==INADDR_NONE)
    {
       	fprintf(stderr,"SOCKET:ERROR invalid server IP\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
    }
    if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
    	close(s);
    	printf("SOCKET:ERROR en bind del socket\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
    }
	if (listen (s, 10) == -1) // backlog=10
  	{
    	close(s);
		printf("SOCKET:ERROR en listen del socket\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
  	}
	while (1){
			addr_len = sizeof(struct sockaddr_in);
			printf("SOCKET: Esperando el cliente\r\n");
			if ( (newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1)
      		{
				printf("SOCKET:ERROR en accept del socket\r\n");
				pthread_mutex_lock(&mutexData_process_status);
				serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
				pthread_mutex_unlock(&mutexData_process_status);
				FOREVER_LOOP
	    	}
    		printf  ("SOCKET: conexion desde:  %s\n", inet_ntoa(clientaddr.sin_addr));
    		while (1){
    			pthread_mutex_lock(&mutexData_receive_serial);
    			if (write_string_to_socket(newfd)== 0){
    				read_string_from_socket(newfd);
    			}
    			pthread_mutex_unlock(&mutexData_receive_serial);
    			usleep(500000);
    		}
	}
}


int write_string_to_socket (int fd){
	int index_received_message_queu;
	int size_received_message_queu;
	char buffer[128];
	int return_flag = -1;

	if((index_received_message_queu=cd_get_first_non_free_index_meessage_type(receive_queue,MESSAGE_TYPE_ID,RECEIVE_QUEUE_LENGTH)) !=-1){
		size_received_message_queu = cd_read_message_qeue(receive_queue,MESSAGE_TYPE_ID,buffer,index_received_message_queu);
		pthread_mutex_unlock(&mutexData_receive_serial);
		buffer[size_received_message_queu]='\0';
		if(write(fd,buffer,size_received_message_queu) == -1){
			printf("SOCKET:ERROR en write del socket\r\n");
			pthread_mutex_lock(&mutexData_process_status);
			serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
			pthread_mutex_unlock(&mutexData_process_status);
			FOREVER_LOOP
		}
		printf("SOCKET:Se envio al cliente %d bytes el mensaje %s",size_received_message_queu,buffer);
		return_flag = 0;
	}
return return_flag;
}

void read_string_from_socket (int fd){
	char buffer[128];
	int n;
	int index_send_message_queu;

	if ((n = read(fd,buffer,128))== -1){
		printf("SOCKET:ERROR en read del socket\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
	}
	if (n == 0){
		printf("SOCKET:ERROR en read del socket se recibieron 0 bytes EOF\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
	}
		buffer[n]='\0';
		printf("SOCKET:Se recibio del cliente %d bytes el mensaje %s",n,buffer);
		pthread_mutex_lock(&mutexData_receive_serial);
	if((index_send_message_queu = cd_getFreeIndex(receive_queue,RECEIVE_QUEUE_LENGTH)) == -1){
		printf("SOCKET:COLA de mensajes llena\r\n");
		pthread_mutex_lock(&mutexData_process_status);
		serial_manager_status.thread_socket_status = THREAD_NOT_OK ;
		pthread_mutex_unlock(&mutexData_process_status);
		FOREVER_LOOP
	}
	cd_write_message_qeue(receive_queue,MESSAGE_TYPE_COMMAND,buffer,n,index_send_message_queu);
	pthread_mutex_unlock(&mutexData_receive_serial);
	printf("SOCKET:Se escribio en la cola %d Bytes el mensaje %s",n,buffer);
}
