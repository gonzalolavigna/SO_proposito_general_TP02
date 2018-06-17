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
//La aplicacion consiste de 3 threads,
//Threads de recepcion para adquirir los datos del puerto serie,
//Threads de transmision para enviar los datos a traves del puerto serie
//Threads del socket que se encarga de comunicarse por TCP/IP con la aplicacion en Python
#include "receiveSerialThread.h"
#include "sendSerialThread.h"
#include "socketThread.h"



//Estructura de datos que indica el estado de todos los threads
process_variables_t serial_manager_status;
//Cola de mensajes para comunicarse entre los distintos threads
Message_Queu_t receive_queue [RECEIVE_QUEUE_LENGTH];

//Funcion que inicializa el puerto serie y parsea la consola que viene en forma de string
int serial_com_init (char * console);
//Parsea los argumentos que vienen al ser llamada la funcion
int parse_argument (int argc);
//Bloque las señales provenientes del SO, para poder crear los threads
void bloquear_signals (void);
//Desbloquea las señales provenientes del SO, para ejecutar el thread principal y que pueda manerjar las señales entrantes
void desbloquear_signals (void);

//Los threads se comunican a traves de una estructura de datos, cuando detectan un error escriban este estructura, esta funcion
//chequea que los threads funcionan correctamente. Si algun thread manda un mensaje de error cierra la aplicacion
int check_thread_integrity (process_variables_t * process_status);
//Funcion que cierra los socket si estan abiertos y tambien cierra el puerto serie
void exit_function (process_variables_t * process_status);

//Mutex que sirve para no tener conflicto entre la cola de mensajes de los threads
pthread_mutex_t mutexData_receive_serial = PTHREAD_MUTEX_INITIALIZER;
//Mutex que sirve para que los threads manden mensaje de status
pthread_mutex_t mutexData_process_status = PTHREAD_MUTEX_INITIALIZER;


//Handler para la signa del SO, Ctrl+C.
void sigint_handler (int sig){
	write(1,"SIGINT recibida finalizando proceso\n",37);
	exit_function(&serial_manager_status); //Se cierra el socket y tambien el puerto serie
	exit(2);
}

//Handler para la signa del SO, SIGPIPE cuando se rompe un pipe.
void sigpipe_handler (int sig){
    write(1,"SIGPIPE recibida finalizando proceso\n",38); //Se cierra el socket y tambien el puerto serie
    exit_function(&serial_manager_status);
    exit(2);
}

//Se acepta un solo parametro que es el numero del puerto serie a ingresar. Si se ingresan un valor que no sea un digito el proceso sale.
int main(int argc, char **argv)
{
	//Variable para guardar el puerto serie almacena
	int serial_com_port;
	//Variable para ver si el proceso se encuetnra funcionando correctamente
	int process_status;
	//Se guardan los indicadores de los threads por si son necesarios mas adelante
	pthread_t thread_receive_serial;
	pthread_t thread_send_serial;
	pthread_t thread_socket;

	//Estructuras para poder sobreescribir los handler de las señales
	struct sigaction sa_1;
	struct sigaction sa_2;

	printf("SERIAL SEVICE:Inicio Serial Service\r\n");

	//Se inicializan las variables de estado de los threads que se van a crer.
	serial_manager_status.process_status = WAIT_INIT ;
	serial_manager_status.thread_receive_serial_status = THREAD_NOT_OK;
	serial_manager_status.thread_send_serial_status = THREAD_NOT_OK;
	serial_manager_status.thread_socket_status = THREAD_NOT_OK;

	//Sobreescritura de los handler de las señales
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

    //Se parsean los argumentan que ingresa el usuario por el proceso (Si se coloca mas de un argumento o el argumento esta fuera de rango se sale)
	if (parse_argument(argc) == -1){
		exit(EXIT_FAILURE);
		return -1;
	}
	if((serial_com_port = serial_com_init (argv[1]))<0){
		exit(EXIT_FAILURE);
		return -1;
	}

	//Se carga en la estructura de datos el puerto serie que va a ser utilizado. De esta manera puede cerrarse mas tarde.
	serial_manager_status.serial_com_port = serial_com_port;
	//Se inicializa la cola de mensajes con todos valores en cero y que todos los espacios estan libres.
	cd_init(receive_queue,RECEIVE_QUEUE_LENGTH);


	//Se crean los thread de recepcion por el puerto serie, transmision por el puerto serie y el socket TCP/IP por el puerto 10.0000
	bloquear_signals();
	pthread_create(&thread_receive_serial,NULL,console_interface,(void *)&serial_com_port);
	pthread_create(&thread_send_serial,NULL,send_console_interface,(void *)&serial_com_port);
	pthread_create(&thread_socket,NULL,socket_tcp_server,NULL);
	desbloquear_signals();

	//Se indica que inicializaron todos los threads.
	pthread_mutex_lock(&mutexData_process_status);
	serial_manager_status.process_status = ALL_THREADS_RUNNING ;
	pthread_mutex_unlock(&mutexData_process_status);

	//Se duerme este proceso medio segundo.
	usleep(500000);
	while (1){
		if((process_status = check_thread_integrity(&serial_manager_status))== -1){
			printf("MAIN:Error en los Threads\n");
			exit_function(&serial_manager_status);
			exit(EXIT_FAILURE);
			return 1;
		}
		//Para evitar que este proceso este todo el tiempo polling se duerme por medio segundo. Asi se pasa control al S.O
		usleep(500000);
	}

	exit(EXIT_SUCCESS);
	return 0;
}

//Si el argumento es distinto se sale con error de esta funcion (-1). Si se reciben dos parametros se sale correctamente (0)
//TODO: Cambiar valor de retorno por -1 a un flag
//TODO: Cambiar valor de retorno por  0 a un flag
int parse_argument (int argc){
	if (argc != 2){
		printf("SERIAL SEVICE ERROR: Numero Incorrecto de parametros, solo se acepta 1\r\n");
		return -1;
	}
	return 0;
}

//Se recibe un string que es el valor en string del puerto serie a abrir.
//Se parsea verificando que este en el rango valido de la API serial. Se verifica que sea un digito. Y luego se inicializa el puerto.
//Devuelve error si el parametro no es un numero valido de puerto y si el puerto serie no pudo inicializarse.
//TODO: Cambiar valor de retorno por -1 a un flag
//TODO: Cambiar valor de retorno por  0 a un flag
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

//Funcion para bloquear todas las señales
void bloquear_signals (void){
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK,&set,NULL);
}

//Funcion para desbloquear todas las señales
void desbloquear_signals (void){
	sigset_t set;
	sigfillset(&set);
	pthread_sigmask(SIG_UNBLOCK,&set,NULL);
}

//Todos los threads cuando inicializan escriben que estan OKEY, si encuentran en su flujo una situacion de error escriben las variable status con THREAD_NOT_OK
//Para este diseño se toma el criterio que si un thread arroja un error se abora la operacion cerrando todos los fd y los socket.
//Se identifica el socket que falla para imprimir por consola con motivos de debug.
//TODO: Crear un objeto para manejar la integridad de los threads.
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

//Parecio coherente crear una funcion que permita cerrar todos los procesos. Si todos los procesos pudieron inicializar se abrio un socket por eso el if.
//TODO: Crear un objeto para manejar la integridad de los threads.
void exit_function (process_variables_t * process_status){
	if(process_status->process_status == ALL_THREADS_RUNNING){
		serial_close();
		close(process_status->socket);

	}
	else {
		serial_close();
	}
}
