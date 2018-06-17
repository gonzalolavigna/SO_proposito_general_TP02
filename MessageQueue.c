#include "MessageQueue.h"
#include <string.h>

//Esta serie de funciones para manejar la cola.
void cd_init(Message_Queu_t* clients, int len)
{
	int i;
	for(i=0;i<len;i++){
		clients[i].flagFree=1;
		clients[i].message_type = MESSAGE_TYPE_NULL;
	}
}

int cd_getFreeIndex(Message_Queu_t* clients, int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		if(clients[i].flagFree)
			return i;
	}
	return -1;
}

int cd_getClientOcuppation (Message_Queu_t * clients, int len)
{
	int i;
	int count = 0;
	for (i = 0 ; i < len ;  i++){
		if(clients[i].flagFree == 1){
			count++;
		}
	}
	return count;
}

int cd_write_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_save, int size, int index){
	if(size > MAX_MESSAGE_LENGTH)
		return -1;
	strncpy(clients[index].message,message_to_save,size);
	clients[index].message[size] = '\0';
	clients[index].flagFree = 0;
	clients[index].message_type = message_type;
	return 0;

}
int cd_read_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_receive, int index){
	int message_size;
	message_size = strlen(clients[index].message);
	if(message_size > MAX_MESSAGE_LENGTH)
		return -1;

	strncpy(message_to_receive,clients[index].message,message_size);
	clients[index].flagFree = 1;
	return message_size;
}

int cd_get_first_non_free_index_meessage_type (Message_Queu_t * clients, int message_type, int len){
	int i = 0;
	int flag = -1;
	for (i = 0; i < len; i++){
		if(clients[i].flagFree == 0 && clients[i].message_type == message_type){
			flag = i;
			break;
		}

	}
	return flag;
}
