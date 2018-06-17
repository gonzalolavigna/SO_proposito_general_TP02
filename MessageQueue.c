#include "MessageQueue.h"
#include <string.h>

//Esta serie de funciones para manejar la cola. de la practica 06 se le hizo modificaciones
//TODO: Es posible mejorar cosas como el tamano de la cola para que sea constante.
//TODO: Codigicart con define mensajes de error para que no sean 0 y -1.
//TODO: Pasar a un define los valor de flagfree.
//TODO: HAcer que funcione como un buffer circular y evitar tener que enviar el indice. Esto reduciria mucho la interfaz en los thread.

///1NOTE: No es una cola per se

//Idem PRactica06
void cd_init(Message_Queu_t* clients, int len)
{
	int i;
	for(i=0;i<len;i++){
		clients[i].flagFree=1;
		clients[i].message_type = MESSAGE_TYPE_NULL;
	}
}
//Idem PRactica06. USar esta funcion antes de hacer un wirte message.
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
//Funcion para obtener cuantos indices estan libres.
//Es un control para ver que en una transacciones no se empiece a llenar,
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

//Una escritura en la cola de mensajes, escribe el string. Se agrega el terminador \0 al final.
//Como dato particulas se agrega un tipo de mensaje para simsplificar e3l filtrado
//Tiene como parametros, la cola de mensajes, el tipo de mensaje que se desea escribir. el string que es el mensaje, el tamano de los datos del string.
//Y el indice dentro de las cola. antes de usar esta funcion se recomenda obener un indice libre con el respectivo mutex.
//Antes de salir de la funcion se configura el indice como vacio. Para que se puedaserr usado por otro thread.
//TODO: Definir MEssage Type como un enum

int cd_write_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_save, int size, int index){
	if(size > MAX_MESSAGE_LENGTH)
		return -1;
	strncpy(clients[index].message,message_to_save,size);
	clients[index].message[size] = '\0';
	clients[index].message_type = message_type;
	clients[index].flagFree = 0;
	return 0;
}

//funcion para obtener un string de datos de la cola.
//Se reciben como parametro la cola de los cliente, el tipo de mensaje (no usado en esta implementacion), el mensaje a grabar y el indice de la coal donde grabarlo.
//Antes de salir se ocupa el flag free como ocupado. Llamar ea esta funcion dentro de un mutex porque pueden haber colisiones.
int cd_read_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_receive, int index){
	int message_size;
	message_size = strlen(clients[index].message);
	if(message_size > MAX_MESSAGE_LENGTH)
		return -1;

	strncpy(message_to_receive,clients[index].message,message_size);
	clients[index].flagFree = 1;
	return message_size;
}

//Funcion para hacer un polling que permite obtener un indice, swi hay un mensaje ocupado con cierto messaghe type.
//Usar esta funcion antes de hacer un read message.
//Util para saber si llego un comando, un id, un ok u otra cosa.
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
