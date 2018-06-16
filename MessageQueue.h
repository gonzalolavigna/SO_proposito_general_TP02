#define MAX_MESSAGE_LENGTH 20
#define MESSAGE_TYPE_NULL 0
#define MESSAGE_TYPE_ID 1
#define MESSAGE_TYPE_OK 2
#define MESSAGE_TYPE_COMMAND 3

struct S_Message_Queue
{
	char message[MAX_MESSAGE_LENGTH];
	int flagFree;
	int message_type;
};
typedef struct S_Message_Queue Message_Queu_t;

void cd_init(Message_Queu_t* clients, int len);
int cd_getFreeIndex(Message_Queu_t* clients, int len);
int cd_getClientOcuppation (Message_Queu_t * clients, int len);
int cd_write_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_save, int size, int index);
int cd_read_message_qeue (Message_Queu_t * clients, int message_type, char * message_to_receive, int index);
int cd_get_first_non_free_index_meessage_type (Message_Queu_t * clients, int message_type, int len);
