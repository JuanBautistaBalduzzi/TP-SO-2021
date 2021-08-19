/*
 ============================================================================
 Name        : Discord.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

int ciclos_totales;
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <commons/config.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <semaphore.h>
#include <shared/conexion.h>
#include <shared/TAD_PATOTA.h>
#include <shared/TAD_TRIPULANTE.h>

// =============== PAHTS =================
char* PATH_TAREAS;
char* PATH_CONFIG;
t_config* config;
char* PATH_DISCORDIADOR_LOG;
t_log* logger_discordiador;
char* PATH_TRIPULANTE_LOG;
t_log* logger_tripulante;
char* PATH_CONEXIONES_LOG;
t_log* logger_conexiones;


t_log* log_roto;
//-------------------------------------
//PARA EJECUTAR DESDE CONSOLA USAR:
//PATH_CONFIG "../discordiador.config"
//#define PATH_TAREAS "../src/tareas/"
//-------------------------------------
//PARA EJECUTAR DESDE ECLIPSE USAR:
//#define PATH_CONFIG "discordiador.config"
//#define PATH_TAREAS "src/tareas/"
//-------------------------------------


sem_t hilosEnEjecucion;
sem_t multiProcesamiento;

pthread_t firstInit;
pthread_mutex_t mutexIO;
pthread_t hilo_sabotaje;

//Semaforos para modificar las colas -------------
pthread_mutex_t sem_cola_new;
pthread_mutex_t sem_cola_ready;
pthread_mutex_t sem_cola_exec;
pthread_mutex_t sem_cola_bloqIO;
pthread_mutex_t sem_cola_exit;
//------------------------------------------------
pthread_mutex_t socketMongo;
pthread_mutex_t socketMiRam;

pthread_mutex_t trip_comparar;
sem_t pararPlanificacion[2];
sem_t sem_tripulante_en_ready;
sem_t pararIo;
t_list* listaPatotas;

t_queue* new;
t_queue* ready;
t_list* execute;
t_list* finalizado;
t_queue* bloqueados;
t_list* bloqueados_sabotaje;

Tripulante* esta_haciendo_IO;
Tripulante* trip_cmp;

char* ipMiRam;
char* puertoMiRam;
char* ipMongoStore;
char* puertoMongoStore;
char* algoritmo;
char* puertoSabotaje;
int cliente_sabotaje;

int a = 0;
int TripulantesTotales = 0;
int multiProcesos;
int quantum;
int retardoCpu;
int tiempo_sabotaje;
int estado_planificacion;
_Bool primerInicio=true;
uint8_t t_totales=1;
uint8_t p_totales=1;
_Bool correr_programa=true;

/*AVISOS PARROQUIALES

 * ESPERO SEAN DE TU AGRADO MIS COMENTARIOS
 * VAMOS QUE YA LO TENEMOS EL MODULO
 * FALTARIAN EL MANEJO DE STRINGS POR ESO HAY VARIABLES NO INICIALIZADAS
 * Y BUENO DESPUES EL ROUND ROBIN VIENDO EL VIDEO TENGO QUE CAMBIAR UNA COSA POR EL IO
 * Y BUENO SOLO FALTARIA VER EL TEMA DE LOS SABOTAJES DESPUES
 * ESPERO SEAN UTILES MIS COMENTARIOS
 */
//esta funcion es porque no puedo aplicar parcialmente una funcion en c ):

Tripulante* buscar_tripulante(int tripulante_buscado){
	bool _tripulante_en_la_cola(Tripulante* tripulante){
		return tripulante->id == tripulante_buscado;
	}
	t_list* lista_new = new->elements;
	t_list* lista_ready = ready->elements;
	t_list* lista_block = bloqueados->elements;

	pthread_mutex_lock(&sem_cola_new);
	Tripulante* encontrado = list_find(lista_new, (void*) _tripulante_en_la_cola);
	pthread_mutex_unlock(&sem_cola_new);

	if (encontrado != NULL)
		return encontrado;

	pthread_mutex_lock(&sem_cola_ready);
	encontrado = list_find(lista_ready, (void*) _tripulante_en_la_cola);
	pthread_mutex_unlock(&sem_cola_ready);

	if (encontrado != NULL)
		return encontrado;

	pthread_mutex_lock(&sem_cola_exec);
	encontrado = list_find(execute, (void*) _tripulante_en_la_cola);
	pthread_mutex_unlock(&sem_cola_exec);

	if (encontrado != NULL)
		return encontrado;

	pthread_mutex_lock(&sem_cola_bloqIO);
	encontrado = list_find(lista_block, (void*) _tripulante_en_la_cola);
	pthread_mutex_unlock(&sem_cola_bloqIO);
	return encontrado;
}
char* log_rotoo;
void iniciar_paths(char* s){
	if (!strcmp(s,"eclipse")){
		PATH_CONFIG = "discordiador.config";
		PATH_TAREAS = "src/tareas/";
		PATH_DISCORDIADOR_LOG = "src/logs/DISCORDIADOR.log";
		PATH_TRIPULANTE_LOG = "src/logs/TRIPULANTE.log";
		PATH_CONEXIONES_LOG = "src/logs/CONEXIONES.log";
		log_rotoo = "src/logs/log_roto.logo";

	} else {
		PATH_CONFIG = "../discordiador.config";
		PATH_TAREAS = "../src/tareas/";
		PATH_DISCORDIADOR_LOG = "../src/logs/DISCORDIADOR.log";
		PATH_TRIPULANTE_LOG = "../src/logs/TRIPULANTE.log";
		PATH_CONEXIONES_LOG = "../src/logs/CONEXIONES.log";
	}

}

bool cmpTripulantes(Tripulante* uno, Tripulante* dos)
{
	return uno->id<dos->id;
}
void* identidad(void* t)
{
	return t;
}
//esta para saber si es el mismo tripulante
bool esElMismoTripulante(Tripulante* ra)
{

	return (trip_cmp->id == ra->id);
}

double calcular_distancia(Tripulante* tripulante, int x, int y)
{
	double resultado = sqrt( (double) (pow(x-tripulante->posicionX,2) + pow(y-tripulante->posicionY,2)) );
	return resultado;



}

bool es_tarea_IO(char* tarea)
{
	return (string_contains(tarea, "GENERAR") || string_contains(tarea,"CONSUMIR") || string_contains(tarea,"DESCARTAR_BASURA") );
}


int conectarse_mongo()
{

	int socket;
	pthread_mutex_lock(&socketMongo);
	socket=crear_conexion(ipMongoStore,puertoMongoStore);
	pthread_mutex_unlock(&socketMongo);
	while(socket==(-1))
	{
		puts("RECONECTANDO CON MONGO_STORE");
		pthread_mutex_lock(&socketMongo);
		socket=crear_conexion(ipMongoStore,puertoMongoStore);
		pthread_mutex_unlock(&socketMongo);
		sleep(10);
	}
	log_info(logger_conexiones,"Se creo una conexionn con MONGOSTORE Socket: %d",socket);
	return socket;
}
int conectarse_Mi_Ram()
{
	int socket;
	pthread_mutex_lock(&socketMiRam);
	socket = crear_conexion(ipMiRam,puertoMiRam);
	pthread_mutex_unlock(&socketMiRam);

		while(socket==(-1))
		{
			puts("RECONECTANDO CON MI_RAM...");
			pthread_mutex_lock(&socketMiRam);
			socket = crear_conexion(ipMiRam,puertoMiRam);
			pthread_mutex_unlock(&socketMiRam);
			sleep(3);
		}
	log_info(logger_conexiones,"Se creo una conexionn con MI-RAM Socket: %d",socket);
	return socket;
}

int string_to_int(char* palabra)
{
	int ret;
	if(strlen(palabra)==2)
	{
		 ret= (palabra[0]-'0')*10+palabra[1]-'0';
	}
	else
	{
		ret=palabra[0]-'0';
	}
	return ret;
}


void enviar_estado (Tripulante* tripulante){
	if (!strcmp(tripulante->estado,"EXIT"))
		return;
	int socket_miram = conectarse_Mi_Ram();
	t_paquete* paquete = crear_paquete(ACTUALIZAR_ESTADO);
	t_cambio_estado* estado_actualizado  = malloc(sizeof(t_cambio_estado));

	estado_actualizado->id_tripulante= tripulante->id;
	estado_actualizado->id_patota = tripulante->idPatota;
	estado_actualizado->estado = (char) tripulante->estado[0];


	agregar_paquete_cambio_estado(paquete,estado_actualizado);
	log_info(logger_conexiones,"Se envia un papquete ENVIAR_ESTADO, id_trip: %d, id_patota: %d, estado: %c",estado_actualizado->id_tripulante,estado_actualizado->id_patota,estado_actualizado->estado);
	enviar_paquete(paquete,socket_miram);
	liberar_t_cambio_estado(estado_actualizado);

}
void cambiar_estado(Tripulante* tripulante,char* estado){
	free(tripulante->estado);
	log_info(logger_discordiador,"Se le cambia el estado al tripulante %d a %s ",tripulante->id,estado);
	tripulante->estado = strdup(estado);
	enviar_estado(tripulante);
}

void completar_posiciones_iniciales(char* posiciones, t_list* poci)
{
	if (posiciones == NULL || posiciones[0] == '\0')
		return;

	char** pares_xy = string_split(posiciones," ");
	char** get_posicion;

	for(int i=0 ; pares_xy[i] != NULL; i++)
	{
		get_posicion = string_split(pares_xy[i],"|");
		list_add(poci,(void*) strtol(get_posicion[0],NULL,10));
		list_add(poci,(void*) strtol(get_posicion[1],NULL,10));

		free(get_posicion[0]);		// No se si es necesario liberar cada posicion especifica
		free(get_posicion[1]); 		// No se si es necesario liberar cada posicion especific
		free(get_posicion);
	}

	for(int i=0 ; pares_xy[i] != NULL; i++)
	{
		free(pares_xy[i]);
	}
		free(pares_xy);					// No se cuantas posiciones especificas hay
}

char* leer_tareas(char* path, int* cantidad_tareas){
	FILE* archivo;
	char* raiz = strdup(PATH_TAREAS);
	string_append(&raiz,path);
	archivo = fopen(raiz,"r");


	if (archivo==NULL){
			puts("no se pudo leer el path ingresado");
			free(raiz);
			return NULL;
	}
	char* tareas = string_new();
	char* leido = malloc(50);

	while(!feof(archivo)){
		fgets(leido,50,archivo);
		strtok(leido,"\n");			//con esto borro el \n que se lee
		string_append(&tareas,leido);
		string_append(&tareas,"|");
		(*cantidad_tareas)++;
	}

	int ultima_posicion = strlen(tareas);
	tareas[ultima_posicion-1] = '\0'; //Con esto borro el ultimo "|"
	fclose(archivo);

	free(raiz);
	free(leido);
	return tareas;
}

void obtener_parametros_tarea(Tripulante* t, int posX, int posY)
{
	if(es_tarea_IO(t->Tarea))
	{
//		GENERAR_OXIGENO 10;4;4;15
		char** list = string_split(t->Tarea," ");
		if(list[0]==t->Tarea)
		{
			char** parametros=string_split(t->Tarea,(char*)';');
			posX=string_to_int(parametros[1]);
			posY=string_to_int(parametros[2]);
			t->espera=string_to_int(parametros[3]);
		}
		else
		{
//			CONSUMIR_OXIGENO 10;4;4;15
			char** parametros=string_split(list[1], ";");
			posX=string_to_int(parametros[2]);
			posY=string_to_int(parametros[3]);
			t->espera=string_to_int(parametros[4]);

		}

	}
	else
	{
//		MATAR_PERSAS 0;5;5;20
		char** parametros=string_split(t->Tarea,(char*)";");
		posX=(int)string_to_int(parametros[1]);
		posY=(int)string_to_int(parametros[2]);
		t->espera=string_to_int(parametros[3]);
	}
}



void mover_a_finalizado(Tripulante* tripulante){
	bool _es_el_mismo_tripulante(Tripulante* tripu_en_cola){
		return (tripu_en_cola->id == tripulante->id);
		}

	t_list* lista_ready = ready->elements;
	t_list* lista_block = bloqueados->elements;
	log_info(logger_discordiador,"Se mueve al tripulante %d de %s a EXIT ",tripulante->id,tripulante->estado);
	if (string_contains(tripulante->estado,"READY")){
		pthread_mutex_lock(&sem_cola_ready);
		pthread_mutex_lock(&sem_cola_exit);
		list_add(finalizado,list_remove_by_condition(lista_ready,(void*)_es_el_mismo_tripulante ));
		pthread_mutex_unlock(&sem_cola_ready);
		pthread_mutex_unlock(&sem_cola_exit);
		return;
	}

	if (string_contains(tripulante->estado,"BLOCKED")){
		pthread_mutex_lock(&sem_cola_bloqIO);
		pthread_mutex_lock(&sem_cola_exit);
		list_add(finalizado,list_remove_by_condition(lista_block,(void*)_es_el_mismo_tripulante));
		pthread_mutex_unlock(&sem_cola_bloqIO);
		pthread_mutex_unlock(&sem_cola_exit);
		return;
		}

	if (string_contains(tripulante->estado,"EXEC")){
		pthread_mutex_lock(&sem_cola_exec);
		pthread_mutex_lock(&sem_cola_exit);
		list_add(finalizado,list_remove_by_condition(execute,(void*)_es_el_mismo_tripulante));
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_exit);
		return;
		}

}

Patota* buscar_patota(int numero_patota) {
	bool _patota_en_la_cola(Patota* patota){
		return (patota->id == numero_patota);
	}
	Patota* encontrada = list_find(listaPatotas, (void*) _patota_en_la_cola);
	return encontrada;
}

void enviar_fin_patota(Patota* patota){
	if ((patota->cantidad_tripulantes)>0)
		return;

	int socket_miram = conectarse_Mi_Ram();
	t_paquete* paquete = crear_paquete(FIN_PATOTA);
	t_tripulante* estructura = malloc(sizeof(t_tripulante));
	estructura->id_tripulante = 0;
	estructura->id_patota = patota->id;
	estructura ->posicion_x = 0;
	estructura ->posicion_y = 0;

	agregar_paquete_tripulante(paquete,estructura);
	log_info(logger_conexiones,"Se envia un paquete FIN_PATOTA a MI-RAM,id_patota: %d",estructura->id_patota);
	log_info(logger_discordiador,"FINALIZARON todos los tripulantes de la patota: %d",estructura->id_patota);
	enviar_paquete(paquete,socket_miram);
	liberar_t_tripulante(estructura);
}
void eliminarTripulante(Tripulante* tripulante)
{
	if (!strcmp(tripulante->estado,"EXIT"))
		return;

	if(!strcmp(tripulante->estado,"EXEC"))
	{
		sem_post(&multiProcesamiento);
	}
	int socket_miram=conectarse_Mi_Ram();
	t_paquete* paquete = crear_paquete(ELIMINAR_TRIPULANTE);
	t_tripulante* estructura = malloc(sizeof(t_tripulante));
	estructura->id_tripulante = tripulante -> id;
	estructura->id_patota = tripulante->idPatota;
	estructura ->posicion_x = 0;
	estructura ->posicion_y = 0;

	agregar_paquete_tripulante(paquete,estructura);
	log_info(logger_conexiones,"Se envia un paquete ELIMINAR_TRIPULANTE a MI-RAM, id_tripulante: %d, id_patota: %d",estructura->id_tripulante,estructura->id_patota);
	enviar_paquete(paquete,socket_miram);

//	int socket_mongo = conectarse_mongo();
//	paquete = crear_paquete(ELIMINAR_TRIPULANTE);
//	agregar_paquete_tripulante(paquete,estructura);
//	log_info(logger_conexiones,"Se envia un paquete %d a MONGOSTORE, id_tripulante: %d, id_patota: %d",estructura->id_tripulante,estructura->id_patota);
//	enviar_paquete(paquete,socket_mongo);

	liberar_t_tripulante(estructura);
	mover_a_finalizado(tripulante);
	cambiar_estado(tripulante,"EXIT");
	Patota* patota_a_eliminar = buscar_patota(tripulante->idPatota);
	patota_a_eliminar->cantidad_tripulantes--;
	enviar_fin_patota(patota_a_eliminar);
	//sem_post(&(tripulante->sem_pasaje_a_exec));
	pthread_kill(tripulante->hilo_vida,0);

}
void* iniciar_Planificacion()
{
	estado_planificacion = 1;
	log_info(logger_discordiador,"SE INICIA LA PLANIFICACION");
	pthread_detach(firstInit);
	while (correr_programa){
		if(!estado_planificacion)
			sem_wait(&(pararPlanificacion[0]));

//		log_info(log_roto,"me quedo en el semaforo sem_trip_en_ready");
		sem_wait(&sem_tripulante_en_ready);
		if(list_size(ready->elements)==0)
		{
			continue;
		}
		log_info(logger_discordiador,"Se espera signal para ver mover elementos de READY");
//		log_info(log_roto,"me quedo en el semaforo multiprocesamiento");
		sem_wait(&multiProcesamiento);


		//ESTE MUTEX ES PARA PROTEGER LA COLA DE READY
		pthread_mutex_lock(&sem_cola_ready);
		//este para proteger la lista de ejecutados
		pthread_mutex_lock(&sem_cola_exec);
		//AGREGO A LISTA DE EJECUCION
		Tripulante* tripulante = (Tripulante*) queue_pop(ready);
//		log_info(logger_discordiador,"Se mueve al tripulante %d de %s a EXEC",tripulante->id,tripulante->estado);
		cambiar_estado(tripulante,"EXEC");
		list_add(execute, tripulante);
		pthread_mutex_unlock(&sem_cola_ready);
		pthread_mutex_unlock(&sem_cola_exec);
		sem_post(&(tripulante->sem_pasaje_a_exec));
//		log_info(log_roto,"le tiuro un post a semaforo sem_pasaje_A_exec");

	}
}

void ejecutando_a_bloqueado(Tripulante* trp )
{
		bool _es_el_mismo_tripulante(Tripulante* tripu_exec){
			return (tripu_exec->id == trp->id);
	}

		pthread_mutex_lock(&sem_cola_bloqIO);
		pthread_mutex_lock(&sem_cola_exec);
		queue_push(bloqueados,list_remove_by_condition(execute,(void*)_es_el_mismo_tripulante));

		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_bloqIO);
		log_info(logger_discordiador,"Se mueve al tripulante %d de %s a BLOQUEADO_IO ",trp->id, trp->estado);
		cambiar_estado(trp,"BLOQUEADO_IO");
}

void bloqueado_a_ready(Tripulante* bloq)
{

			bool _es_el_mismo_tripulante(Tripulante* tripu_exec){
				return (tripu_exec->id == bloq->id);
		}
	pthread_mutex_lock(&sem_cola_ready);
	pthread_mutex_lock(&sem_cola_bloqIO);
	queue_push(ready,list_remove_by_condition(bloqueados->elements,(void*)_es_el_mismo_tripulante));
	pthread_mutex_unlock(&sem_cola_ready);
	pthread_mutex_unlock(&sem_cola_bloqIO);

	log_info(logger_discordiador,"Se mueve al tripulante %d de %s a READY",bloq->id, bloq->estado);
	cambiar_estado(bloq,"READY");
	sem_post(&sem_tripulante_en_ready);
	log_info(logger_discordiador,"Se hace signal para avisar que hay elementos en READY");
}

void enviar_posicion_mongo(int posx,int posy, Tripulante* tripulante,int socket_cliente){
	t_movimiento_mongo* movimiento = malloc(sizeof(t_movimiento_mongo));
	movimiento->id_tripulante = tripulante->id;
	movimiento->origen_x = posx;
	movimiento->origen_y = posy;
	movimiento->destino_x = tripulante->posicionX;
	movimiento->destino_y = tripulante->posicionY;

	t_paquete* paquete = crear_paquete(MOVIMIENTO_MONGO);
	agregar_paquete_movimiento_mongo(paquete,movimiento);

	log_info(logger_conexiones,"Se envia el mensaje ENVIAR_POSICION a MONGOSTORE,id: %d %d|%d a %d|%d" ,movimiento->id_tripulante,movimiento->origen_x,movimiento->origen_y,movimiento->destino_x,movimiento->destino_y);
	enviar_paquete(paquete,socket_cliente);
	liberar_t_movimiento_mongo(movimiento);

}

void enviar_posicion(Tripulante* tripulante,int socket_miram){
		t_paquete* paquete = crear_paquete(ACTUALIZAR_POS);
		t_tripulante* posiciones_actualizadas = malloc(sizeof(t_tripulante));

		posiciones_actualizadas->id_tripulante= tripulante->id;
		posiciones_actualizadas->id_patota = tripulante->idPatota;
		posiciones_actualizadas-> posicion_x = tripulante->posicionX;
		posiciones_actualizadas-> posicion_y = tripulante->posicionY;

		agregar_paquete_tripulante(paquete,posiciones_actualizadas);

		enviar_paquete(paquete,socket_miram);
		log_info(logger_conexiones,"Se envia el mensaje ENVIAR_POSICION a MI-RAM, tripulante %d de la patota: %d se mueve a %d|%d" ,posiciones_actualizadas->id_tripulante,posiciones_actualizadas->id_patota,posiciones_actualizadas->posicion_x,posiciones_actualizadas->posicion_y);
		liberar_t_tripulante(posiciones_actualizadas);

}

// esta va a avanzar el tripulante paso a paso Y Enviar a miram
void* moverTripulante(Tripulante* tripu)
{
	void* ret = "0";
	int socket_miram = conectarse_Mi_Ram();
	int socket_mongo = conectarse_mongo();
//	if(tripu->esta_sabotaje)
//		liberar_conexion(socket_mongo);
	int posicion_actual_x = tripu->posicionX;
	int posicion_actual_y = tripu->posicionY;

	if (tripu->Tarea->posicion_x > tripu->posicionX) {
		tripu->posicionX++;
		enviar_posicion(tripu,socket_miram);
//		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);
		return ret;

	}

	if (tripu->Tarea->posicion_x < tripu->posicionX) {
		tripu->posicionX--;
		enviar_posicion(tripu,socket_miram);
//		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}

	if (tripu->Tarea->posiciion_y < tripu->posicionY) {
		tripu->posicionY--;
		enviar_posicion(tripu,socket_miram);
//		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}

	if (tripu->Tarea->posiciion_y > tripu->posicionY) {
		tripu->posicionY++;
		enviar_posicion(tripu,socket_miram);
//		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}
	return NULL;
}

void* moverSabotaje(Tripulante* tripu, int pos_sabox, int pos_saboy)
{
	void* ret = "0";
	int socket_miram = conectarse_Mi_Ram();
	int socket_mongo = conectarse_mongo();
	if(tripu->esta_sabotaje)
		liberar_conexion(socket_mongo);
	int posicion_actual_x = tripu->posicionX;
	int posicion_actual_y = tripu->posicionY;

	if (pos_sabox > tripu->posicionX) {
		tripu->posicionX++;
		enviar_posicion(tripu,socket_miram);
		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);
		return ret;

	}

	if (pos_sabox < tripu->posicionX) {
		tripu->posicionX--;
		enviar_posicion(tripu,socket_miram);
		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}

	if (pos_saboy < tripu->posicionY) {
		tripu->posicionY--;
		enviar_posicion(tripu,socket_miram);
		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}

	if (pos_saboy > tripu->posicionY) {
		tripu->posicionY++;
		enviar_posicion(tripu,socket_miram);
		if(!(tripu->esta_sabotaje))
			enviar_posicion_mongo(posicion_actual_x,posicion_actual_y,tripu,socket_mongo);

		return ret;
	}
	return NULL;
}


void enviar_inicio_fin_mongo(Tripulante* enviar, char c){
	char* mensaje;
	int socket_mongostore = conectarse_mongo();
	switch (c){
	case 'I':;
	t_paquete* paquete_inicio = crear_paquete(INICIO_TAREA);
	t_pedido_mongo* mensaje_inicio = malloc(sizeof(t_pedido_mongo));
	mensaje = strdup(enviar->Tarea->nombre);

	mensaje_inicio-> id_tripulante= enviar->id;
	mensaje_inicio-> tamanio_mensaje = strlen(mensaje)+1;
	mensaje_inicio-> mensaje  = mensaje;

	agregar_paquete_pedido_mongo(paquete_inicio,mensaje_inicio);
	enviar_paquete(paquete_inicio,socket_mongostore);

	liberar_t_pedido_mongo(mensaje_inicio);
	break;

	case 'F':;
	t_paquete* paquete_fin = crear_paquete(FIN_TAREA);
	t_pedido_mongo* mensaje_fin = malloc(sizeof(t_pedido_mongo));
	mensaje = strdup(enviar->Tarea->nombre);

	mensaje_fin-> id_tripulante= enviar->id;
	mensaje_fin-> tamanio_mensaje = strlen(mensaje)+1;
	mensaje_fin-> mensaje  = mensaje;

	agregar_paquete_pedido_mongo(paquete_fin,mensaje_fin);
	enviar_paquete(paquete_fin,socket_mongostore);

	liberar_t_pedido_mongo(mensaje_fin);
	break;

	}




}
void enviar_consumir_recurso(Tripulante* tripulante){
	int socket_mongostore = conectarse_mongo();
	t_paquete* paquete = crear_paquete(CONSUMIR_RECURSO);
	t_consumir_recurso* consumir = malloc(sizeof(consumir));
	consumir->cantidad = tripulante->Tarea->parametro;
	char** tarea_dividida = string_split(tripulante->Tarea->nombre,"_");

	consumir->tipo = tarea_dividida[0][0];
	consumir->consumible = tarea_dividida[1][0];

	agregar_paquete_consumir_recurso(paquete,consumir);
	enviar_paquete(paquete,socket_mongostore);
	liberar_t_consumir_recurso(consumir);

	free(tarea_dividida[0]);
	free(tarea_dividida[1]);
	free(tarea_dividida);
}

void enviarMongoStore(Tripulante* enviar) {

	//envia tarea al MONGO STORE
	// char* ="22:09 inicio consumir_oxigeno"
	//char[0], [O,B,C]ocigeno|comida|basura, parametro->tarea
	enviar_inicio_fin_mongo(enviar,'I');
	enviar_consumir_recurso(enviar);
	while((enviar->espera != 0 ) && enviar->vida)
	{
		sem_wait(&pararIo);
		log_info(logger_tripulante,"El tripulante %d consume un ciclo para realizar tarea IO",enviar->id);
		sleep(retardoCpu);
//		ciclos_totales++;
//		printf("%d ciclo actual %d\n",tripulant->id,ciclos_totales);
		enviar->espera--;
		sem_post(&pararIo);
	}
	if(!enviar->vida)
	{
		return;
	}
	//lo paso a cola ready
	// char* ="22:09 Fin consumir_oxigeno"
	enviar_inicio_fin_mongo(enviar,'F');
	bloqueado_a_ready(enviar);

}
void desalojo_exec_a_ready(Tripulante* tripulante){
	bool _es_el_mismo_tripulante(Tripulante* tripu_exec){
		return (tripu_exec->id == tripulante->id);
	}

	queue_push(ready,list_remove_by_condition(execute,(void*)_es_el_mismo_tripulante ));

}

void hacerTareaIO(Tripulante* io) {
	//ACA ME PASE UN POQUITO CON LOS SEMAFOROS REVISARRRR
	ejecutando_a_bloqueado(io);
	sem_post(&multiProcesamiento);
	//libero el recurso de multiprocesamiento porque me voy a io


	pthread_mutex_lock(&mutexIO);
	enviarMongoStore(io);
	pthread_mutex_unlock(&mutexIO);


}
void hacerFifo(Tripulante* tripu) {
	while ((tripu->posicionX != tripu->Tarea->posicion_x || tripu->posicionY != tripu->Tarea->posiciion_y) && tripu->vida) {
		//este es el semaforo para pausar laejecucion
		if (!estado_planificacion)
		{
			sem_wait(&(tripu->hilosEnEjecucion));
			continue;
		}
		log_info(logger_tripulante,"El tripulante %d consume un ciclo para moverse",tripu->id);
		sleep(retardoCpu);
		ciclos_totales++;
		printf("%d ciclo actual %d\n",tripu->id,ciclos_totales);
		moverTripulante(tripu);

	}

	if ((tripu->Tarea->es_io) && tripu->vida)
	{
		hacerTareaIO(tripu);
		log_info(logger_tripulante,"El tripulante %d finaliza su tarea %s",tripu->id,tripu->Tarea->nombre);
		free(tripu->Tarea->nombre);
		free(tripu->Tarea);
		tripu->Tarea = NULL;
//		tripu->Tarea->nombre = NULL;
		return;
	}
	if(!tripu->vida)
	{
		return;
	}
	else {
		//una vez que llego donde tenia que llegar espera lo que tenia que esperar
		if (tripu->primer_inicio){
			enviar_inicio_fin_mongo(tripu,'I');
			tripu->primer_inicio = false;
		}
		while((tripu->espera !=0) && tripu->vida)
		{
			if (!estado_planificacion)
			{
				sem_wait(&(tripu->hilosEnEjecucion));
				continue;
			}
			log_info(logger_tripulante,"El tripulante %d consume un ciclo para realizar su tarea",tripu->id);
			sleep(retardoCpu);
			ciclos_totales++;
			printf("%d ciclo actual %d\n",tripu->id,ciclos_totales);
			tripu->espera --;
		}
		if(!tripu->vida)
		{
			return;
		}
		enviar_inicio_fin_mongo(tripu,'F');
		free(tripu->Tarea->nombre);
		free(tripu->Tarea);
//		tripu->Tarea->nombre = NULL;
		tripu->Tarea =NULL;
		sem_post(&tripu->sem_pasaje_a_exec);
	}

}
void hacerRoundRobin(Tripulante* tripulant) {
	int contadorQuantum = 0;
	if (tripulant->kuantum != 0)
	{
		contadorQuantum = tripulant->kuantum;
	}
	//obtener_parametros_tarea(tripulant, &tarea_x,&tarea_y);
	//todo CREO QUE ESTE HACIA QUE SIEMPRE EJECUTE EL MISMO TRIPULANTE, SIN PASAR POR READY PARA QUE SE REPLANIFIQUE
	while (contadorQuantum < quantum)
	{
		if ((tripulant->posicionX == tripulant->Tarea->posicion_x && tripulant->posicionY == tripulant->Tarea->posiciion_y)&& tripulant->vida)
		{
			break;
		}
		//este es el semaforo para pausar laejecucion
//		todo este semaforo entiendo que es para
		if (!estado_planificacion)
		{
			sem_wait(&(tripulant->hilosEnEjecucion));
			continue;
		}
		log_info(logger_tripulante,"N°Q:%d El tripulante %d consume un ciclo para moverse",contadorQuantum,tripulant->id);
		sleep(retardoCpu);
		ciclos_totales++;
		printf("%d ciclo actual %d\n",tripulant->id,ciclos_totales);
		moverTripulante(tripulant);
		contadorQuantum++;
		//le tiroun post al semaforo que me permite frenar la ejecucion

	}

	if(contadorQuantum<quantum && (tripulant->Tarea->es_io) && tripulant->vida)
	{
		tripulant->kuantum=0;
		hacerTareaIO(tripulant);
		log_info(logger_tripulante,"N°Q:%d El tripulante %d finaliza su tarea %s",contadorQuantum,tripulant->id,tripulant->Tarea->nombre);
		tripulant->kuantum=contadorQuantum;
		free(tripulant->Tarea->nombre);
		free(tripulant->Tarea);
//		tripulant->Tarea->nombre = NULL;
		tripulant->Tarea = NULL;
		return;
	}
	if (tripulant->primer_inicio){
		enviar_inicio_fin_mongo(tripulant,'I');
		tripulant->primer_inicio = false;
	}
	while ((contadorQuantum < quantum) && tripulant->vida && tripulant->espera > 0)
	{
		if (!estado_planificacion){
			sem_wait(&(tripulant->hilosEnEjecucion));
			continue;
		}
		log_info(logger_tripulante,"N°Q:%d El tripulante %d consume un ciclo para realizar su tarea",contadorQuantum,tripulant->id);
		sleep(retardoCpu);
		ciclos_totales++;
		printf("%d ciclo actual %d\n",tripulant->id,ciclos_totales);
		if (tripulant->posicionX == tripulant->Tarea->posicion_x && tripulant->posicionY == tripulant->Tarea->posiciion_y){
			tripulant->espera--;
		} else {
			moverTripulante(tripulant);
		}
		contadorQuantum++;
	}
	if(!tripulant->vida)
	{
		return;
	}
	if (tripulant->posicionX == tripulant->Tarea->posicion_x && tripulant->posicionY == tripulant->Tarea->posiciion_y && tripulant->espera == 0 && tripulant->vida)
	{
		log_info(logger_tripulante,"N°Q:%d El tripulante %d finaliza su tarea %s",contadorQuantum,tripulant->id,tripulant->Tarea->nombre);
		tripulant->kuantum=contadorQuantum;
		enviar_inicio_fin_mongo(tripulant,'F');
		free(tripulant->Tarea->nombre);
		free(tripulant->Tarea);
//		tripulant->Tarea->nombre = NULL;
		tripulant->Tarea = NULL;

		sem_post(&(tripulant->sem_pasaje_a_exec));
		log_info(log_roto,"le tire un post a sem_pasaje_A_exec");
	}else
	{
		log_info(logger_tripulante,"N°Q:%d El tripulante %d es desalojado de exec",contadorQuantum,tripulant->id);
		tripulant->kuantum = 0;
		//protejo las colas o listas

		pthread_mutex_lock(&sem_cola_ready);
		pthread_mutex_lock(&sem_cola_exec);
		desalojo_exec_a_ready(tripulant);
		pthread_mutex_unlock(&sem_cola_ready);
		pthread_mutex_unlock(&sem_cola_exec);
		cambiar_estado(tripulant,"READY");
		log_info(logger_discordiador,"Se mueve al tripulante %d de EXEC a %s",tripulant->id,tripulant->estado);
	//le aviso al semaforo que libere un recurso para que mande otro tripulante
		sem_post(&multiProcesamiento);
		sem_post(&sem_tripulante_en_ready);
		log_info(log_roto,"le tire un post a sem_tripulante_en_ready");
		log_info(log_roto,"le tire un post a multiprocesamiento");
		log_info(logger_discordiador,"Se hace signal para avisar que hay elementos en READY");
	}
}

//CLASIFICA LA TAREA DEL TRIPULANTE

void hacerTarea(Tripulante* trip)
{
	if (string_contains(algoritmo,"FIFO"))
	{
		hacerFifo(trip);
	} else {
		hacerRoundRobin(trip);
	}

}

char* enviar_solicitud_tarea(Tripulante* tripulante){
	int socket_miram = conectarse_Mi_Ram();

	t_paquete* paquete = crear_paquete(PEDIR_TAREA);
	t_tripulante* estructura = malloc(sizeof(t_tripulante));
	estructura->id_tripulante = tripulante->id;
	estructura->id_patota = tripulante->idPatota;

	agregar_paquete_tripulante(paquete,estructura);
	log_info(logger_conexiones,"Se el tripulante %d envio un mensaje PEDIR_TAREA a MI-RAM",tripulante->id);
	char* tarea = enviar_paquete_respuesta_string(paquete,socket_miram);
	log_info(logger_conexiones,"El tripulante %d recibio la tarea: %s",estructura->id_tripulante,tarea);
	liberar_conexion(socket_miram);
	liberar_t_tripulante(estructura);
	return tarea;
}

tarea_tripulante* convertir_tarea(char* tarea){
	 tarea_tripulante* tarea_convertida = malloc(sizeof(tarea_tripulante));
	 char** id_dividido;
	 char** parametros_divididos;
	 if (!strcmp(tarea,"fault")){
		 tarea_convertida->nombre = strdup(tarea);
		 return tarea_convertida;
	 }


	 if (es_tarea_IO(tarea)){
		 id_dividido = string_n_split(tarea,2 ," ");
		 parametros_divididos = string_n_split(id_dividido[1],4,";");

//		 GENERAR_OXIGENO 10;4;4;15
		 tarea_convertida->nombre = strdup(id_dividido[0]);
		 tarea_convertida->es_io = 1;
		 tarea_convertida->parametro = strtol(parametros_divididos[0],NULL,10);
		 tarea_convertida->posicion_x = strtol(parametros_divididos[1],NULL,10);
		 tarea_convertida->posiciion_y = strtol(parametros_divididos[2],NULL,10);
		 tarea_convertida->duracion = strtol(parametros_divididos[3],NULL,10);

		 free(id_dividido[0]);
		 free(id_dividido[1]);
		 free(id_dividido);

		 free(parametros_divididos[0]);
		 free(parametros_divididos[1]);
		 free(parametros_divididos[2]);
		 free(parametros_divididos[3]);
		 free(parametros_divididos);

	 } else {
		 parametros_divididos= string_n_split(tarea,4,";");
 //		 DESCARGAR_ITINERARIO;4;4;15
		 tarea_convertida->nombre = strdup(parametros_divididos[0]);
		 tarea_convertida->es_io = 0;
		 tarea_convertida->parametro = 0;
		 tarea_convertida->posicion_x = strtol(parametros_divididos[1],NULL,10);
		 tarea_convertida->posiciion_y = strtol(parametros_divididos[2],NULL,10);
		 tarea_convertida->duracion = strtol(parametros_divididos[3],NULL,10);

		 free(parametros_divididos[0]);
		 free(parametros_divididos[1]);
		 free(parametros_divididos[2]);
		 free(parametros_divididos[3]);
		 free(parametros_divididos);
	 }

	 return tarea_convertida;
}

void pedir_tarea(Tripulante* tripulante){
	char* tarea = enviar_solicitud_tarea(tripulante);
	if (tarea == NULL){
		puts("No se pudo recibir correctamente la tarea");
		return;
	}
	tarea_tripulante* tarea_convertida = convertir_tarea(tarea);
	tripulante->Tarea = tarea_convertida;
	free(tarea);

}
// LARGA VIDA TRIPULANTE ESPEREMOS CADA TRIPULANTE VIVA UNA VIDA FELIZ Y PLENA
void* vivirTripulante(Tripulante* tripulante) {
	log_info(logger_tripulante,"El tripulante %d inicializo su hilo",tripulante->id);

	while (tripulante->vida) {
		log_info(log_roto,"%d en sem_pasaje_A_exec",tripulante->id);
		sem_wait(&(tripulante->sem_pasaje_a_exec));
		if(tripulante->vida == false)
			break;
		if (tripulante->Tarea == NULL){
			pedir_tarea(tripulante);
			log_info(logger_tripulante,"Al tripulante %d le llego la tarea: %s",tripulante->id,tripulante->Tarea->nombre);
			tripulante->primer_inicio = true;
			tripulante->espera = tripulante->Tarea->duracion;
		}

//		printf("TAREA: %s\n",tripulante->Tarea->nombre);
		if (string_contains(tripulante->Tarea->nombre,"fault")){
			tripulante->vida = false;
			continue;
		}
//		Si MIRAM ya los empieza en ready ya no seria necesario mandarselo con la funcino cambiar_estado()
//		cambiar_estado(tripulante,"EXEC");

			hacerTarea(tripulante);
	}
	log_info(logger_tripulante,"El tripulante %d finalizo su ejecucion",tripulante->id);
	eliminarTripulante(tripulante);
	return NULL;
}

void postear_exec(){
	if (list_is_empty(execute)){
		return;
	}
	int i;
	for (i = 0; i < list_size(execute); i++) {
		Tripulante* tripulante = (Tripulante*) list_get(execute,i);
		sem_post(&(tripulante->hilosEnEjecucion));
	}
}

void* atender_sabotaje(int cliente_sabotaje)
{
	int respuesta;
	t_paquete* paquete_recibido = recibir_paquete(cliente_sabotaje, &respuesta);
	log_info(logger_conexiones, "paquete recibido de tipo: %d",paquete_recibido->codigo_operacion);
	if (paquete_recibido->codigo_operacion == -1 || respuesta == ERROR) {
		liberar_conexion(cliente_sabotaje);
		eliminar_paquete(paquete_recibido);
	}
	t_pedido_mongo* posiciciones_sabotaje = deserializar_pedido_mongo(paquete_recibido);
	if(estado_planificacion){
		estado_planificacion = 0;
		sem_wait(&pararIo);
	}

	//separo los parametros del sabotaje
	char** posiciones_divididas = string_split(posiciciones_sabotaje->mensaje, "|");
	liberar_t_pedido_mongo(posiciciones_sabotaje);
	int posx = strtol(posiciones_divididas[0],NULL,10);
	int posy = strtol(posiciones_divididas[1],NULL,10);
	log_info(logger_discordiador,"Se recibe un sabotaje en la posicion %d|%d",posx,posy);
//	----------------------------------------
//				sem_t testear;
//				sem_init(&testear,0,0);
//				sem_wait(&testear);
//	----------------------------------------
	free(posiciones_divididas[0]);
	free(posiciones_divididas[1]);
	free(posiciones_divididas);

	char* inicio_sabotaje;
	char* fin_sabotaje;


//	log_info(logger_discordiador,"Se recibe un sabotaje en la posicion %d|%d",posx,posy);
	Tripulante* mas_cerca;
	//busco el tripulante mas cerca
	puts("estoy por pasar el sem_exec");
	pthread_mutex_lock(&sem_cola_exec);
	puts("pase el semafoto cola_exec");
	for(int i=0;i<list_size(execute);i++)
	{
		Tripulante* iterado = (Tripulante*) list_get(execute,i);

		free(iterado->estado);
		iterado->estado=strdup("BLOQUEADO_SABOTAJE");
		if(i==0)
		{
			mas_cerca = iterado;
		}
		if(calcular_distancia(iterado, posx, posy) < calcular_distancia(mas_cerca, posx, posy))
		{
			mas_cerca=iterado;
		}


	}
	pthread_mutex_unlock(&sem_cola_exec);
	log_info(logger_discordiador,"SE PASARON TODOS LOS TRIPULANTES DE EXEC A BLOQ_IO");
	int in=0;
			Tripulante * auxiliar;
			t_list* lista_ready = ready->elements;
			pthread_mutex_lock(&sem_cola_ready);
			while(in<list_size(lista_ready))
			{
				Tripulante* iterado_ready = list_get(lista_ready,in);
				free(iterado_ready->estado);
				iterado_ready->estado = strdup("BLOQUEADO_SABOTAJE");
				if(in==0)
				{
					auxiliar= iterado_ready;
				}
				if(calcular_distancia(iterado_ready,posx,posy)<calcular_distancia(auxiliar,posx,posy))
				{
					auxiliar=iterado_ready;
				}
				in++;
			}
			pthread_mutex_unlock(&sem_cola_ready);
			log_info(logger_discordiador,"SE PASARON TODOS LOS TRIPULANTES DE READY A BLOQ_IO");
			in=0;
			t_list* lista_bloqueados = bloqueados->elements;
			Tripulante* iterado_bloqueados;
			pthread_mutex_lock(&sem_cola_bloqIO);
			while(in<list_size(lista_bloqueados))
				{
				   iterado_bloqueados = list_get(lista_bloqueados,in);

				   free(iterado_bloqueados->estado);
				   iterado_bloqueados->estado = strdup("BLOQUEADO_SABOTAJE");
				   in++;
				}
			pthread_mutex_unlock(&sem_cola_bloqIO);
			log_info(logger_discordiador,"SE PASARON TODOS LOS BLOQUEADO DE READY A BLOQ_IO");
			int tiempo_sabota=tiempo_sabotaje;

		if(calcular_distancia(mas_cerca, posx, posy)<=calcular_distancia(auxiliar,posx,posy))
		{
			log_info(logger_discordiador,"EL tripulante mas cercano es: %d",mas_cerca->id);
			mas_cerca->esta_sabotaje = 1;
			while(mas_cerca->posicionX!=posx || mas_cerca->posicionY!=posy)
			{
			sleep(retardoCpu);
			moverSabotaje(mas_cerca,posx,posy);
			log_info(logger_discordiador,"EL tripulante %d se mueve",mas_cerca->id);
			}
			log_info(logger_discordiador,"EL tripulante %d Empieza el sabotaje:",mas_cerca->id);
			inicio_sabotaje = strdup("INICIO_DE_SABOTAJE");
			uint32_t tamanio_inicio_sabotaje = strlen(inicio_sabotaje)+1;
			send(cliente_sabotaje,&tamanio_inicio_sabotaje,sizeof(uint32_t),0);
			send(cliente_sabotaje,inicio_sabotaje,tamanio_inicio_sabotaje,0);
			send(cliente_sabotaje,&(mas_cerca->id),sizeof(uint8_t),0);
			log_info(logger_conexiones,"%d manda el mensaje: %s:",mas_cerca->id,inicio_sabotaje);
			free(inicio_sabotaje);

			while(tiempo_sabota!=0)
			{
				sleep(retardoCpu);
				tiempo_sabota--;
				log_info(logger_discordiador,"%d esta realizando el sabotaje:",mas_cerca->id);
			}
			log_info(logger_discordiador,"EL tripulante %d finaliza el sabotaje:",mas_cerca->id);
			fin_sabotaje = strdup("FINALIZAR_SABOTAJE");
			uint32_t tamanio_fin_sabotaje= strlen(fin_sabotaje)+1;
			send(cliente_sabotaje,&tamanio_fin_sabotaje,sizeof(uint32_t),0);
			send(cliente_sabotaje,fin_sabotaje,tamanio_fin_sabotaje,0);
			log_info(logger_conexiones,"%d manda el mensaje: %s:",mas_cerca->id,fin_sabotaje);
			free(fin_sabotaje);

			mas_cerca->esta_sabotaje = 0;
		}
		else
		{
			log_info(logger_discordiador,"EL tripulante mas cercano es: %d",auxiliar->id);
			auxiliar->esta_sabotaje = 1;
			while(auxiliar->posicionX!=posx||auxiliar->posicionY!=posy)
			{
			sleep(retardoCpu);
			moverSabotaje(auxiliar,posx,posy);
			log_info(logger_discordiador,"EL tripulante %d se mueve",auxiliar->id);
			}
			log_info(logger_discordiador,"EL tripulante %d Empieza el sabotaje:",auxiliar->id);
			inicio_sabotaje = strdup("INICIO_DE_SABOTAJE");
			uint32_t tamanio_inicio_sabotaje = strlen(inicio_sabotaje)+1;
			send(cliente_sabotaje,&tamanio_inicio_sabotaje,sizeof(uint32_t),0);
			send(cliente_sabotaje,inicio_sabotaje,tamanio_inicio_sabotaje,0);
			send(cliente_sabotaje,&(auxiliar->id),sizeof(uint8_t),0);
			log_info(logger_conexiones,"%d manda el mensaje: %s:",auxiliar->id,inicio_sabotaje);
			free(inicio_sabotaje);

			while(tiempo_sabota!=0)
			{
				sleep(retardoCpu);
				tiempo_sabota--;
				log_info(logger_discordiador,"%d esta realizando el sabotaje:",auxiliar->id);
			}
			log_info(logger_discordiador,"EL tripulante %d finaliza el sabotaje:",auxiliar->id);
			fin_sabotaje = strdup("FIN_DE_SABOTAJE");
			uint32_t tamanio_fin_sabotaje= strlen(fin_sabotaje)+1;
			send(cliente_sabotaje,&tamanio_fin_sabotaje,sizeof(uint32_t),0);
			send(cliente_sabotaje,fin_sabotaje,tamanio_fin_sabotaje,0);
			log_info(logger_conexiones,"%d manda el mensaje: %s:",auxiliar->id,fin_sabotaje);
			free(fin_sabotaje);
			auxiliar->esta_sabotaje = 0;



		}

		pthread_mutex_lock(&sem_cola_exec);

			for(int i=0;i<list_size(execute);i++)
			{
				Tripulante* iterado=(Tripulante*)list_get(execute,i);
				free(iterado->estado);
				iterado->estado = strdup("EXEC");
			}
		pthread_mutex_unlock(&sem_cola_exec);
		//DEFINO UN SEM CONTADOR QUE NOS VA A SERVIR PARA PAUSAR LA PLANIFICACION DONDE QUERAMOS DESPUES
		estado_planificacion = 1;
		postear_exec();

		 in=0;
				pthread_mutex_lock(&sem_cola_ready);
				t_list* lista_redy = ready->elements;
				while(in<list_size(lista_redy))
				{
					Tripulante* it_ready = list_get(lista_redy,in);
					free(it_ready->estado);
					it_ready->estado = strdup("READY");
					in++;
				}
				pthread_mutex_unlock(&sem_cola_ready);
				sem_post(&pararPlanificacion[0]);

				in=0;
				t_list* lista_bloq = bloqueados->elements;
				pthread_mutex_lock(&sem_cola_bloqIO);
				while(in<list_size(lista_bloq))
					{
					   Tripulante* trip_agregar=list_get(lista_bloq,in);
					   free(trip_agregar->estado);
					   trip_agregar->estado = strdup("BLOQUEADO_IO");
						in++;
					}
				pthread_mutex_unlock(&sem_cola_bloqIO);
				sem_post(&pararIo);
				liberar_conexion(cliente_sabotaje);
	return NULL;
}

char* enviar_iniciar_patota(Patota* pato,int cantidad_tripulantes){
	int socket_iniciar_patota=conectarse_Mi_Ram();
//Creamos un paquete vacio, que solo contiene el codigo de operacion --------------------
	t_paquete* paquete = crear_paquete(INICIAR_PATOTA);
//Luego creamos la estructura que queremos enviar y la completamos--------------------
	t_iniciar_patota* estructura = malloc(sizeof(t_iniciar_patota));
	estructura->idPatota = pato->id;
	estructura->cantTripulantes = cantidad_tripulantes;
	estructura->tamanio_tareas = strlen(pato->tareas)+1;
	estructura->Tareas = strdup(pato->tareas);
	printf("TAMAÑO TAREAS: %d",estructura->tamanio_tareas);
//Cuando tenemos la estructura la metemos al paquete vacio que teniamos--------------------
	agregar_paquete_iniciar_patota(paquete,estructura);

//comprobamos que esta bien --------------------
//	imprimir_paquete_iniciar_patota(estructura);
//con un paquete ya creado podemos enviarlo y dejar que las funcionen hagan to do--------------------
	log_info(logger_conexiones,"Se envia mensaje INICIAR_PATOTA id: %d, cantidad:%d y tareas: %s",estructura->idPatota,estructura->cantTripulantes,estructura->Tareas);
//	enviar_paquete(paquete,socket_iniciar_patota);
	char* respuesta = enviar_paquete_respuesta_string(paquete,socket_iniciar_patota);
	liberar_conexion(socket_iniciar_patota);
	log_info(logger_conexiones,"Se recibie la respuesta: %s",respuesta);
	liberar_t_iniciar_patota(estructura);
	return respuesta;
}

uint8_t obtener_pos(t_list* lista_posiciones_iniciales){
	uint8_t pos;
	if(!list_is_empty(lista_posiciones_iniciales)){
		pos = (int) list_remove(lista_posiciones_iniciales,0);
	}
	else {
		pos =0;
	};
	return pos;
}

void enviar_tripulante(Tripulante* nuevo_tripulante){
	int socket_miram= conectarse_Mi_Ram();
	t_paquete* paquete = crear_paquete(TRIPULANTE);
	t_tripulante* tripulante = malloc(sizeof(t_tripulante));

	tripulante->id_tripulante = nuevo_tripulante->id;
	tripulante->id_patota = nuevo_tripulante->idPatota;
	tripulante->posicion_x = nuevo_tripulante->posicionX;
	tripulante->posicion_y= nuevo_tripulante->posicionY;
	agregar_paquete_tripulante(paquete,tripulante);

	imprimir_paquete_tripulante(tripulante);
	enviar_paquete(paquete,socket_miram);
	log_info(logger_conexiones,"Se envia un tripulante a MI-RAM, id: %d, id_patota: %d, posx: %d, posy: %d",nuevo_tripulante->id,nuevo_tripulante->idPatota,nuevo_tripulante->posicionX,nuevo_tripulante->posicionY);
	liberar_t_tripulante(tripulante);

}

void recorrer_lista(t_list* lista){
	if (list_is_empty(lista)){
		puts("(cola vacia)\n");
		return;
	}
	int i;
	for (i = 0; i < list_size(lista); i++) {
		Tripulante* tripulante = (Tripulante*) list_get(lista,i);
		printf("Tripulante: %d \t\t Estado: %s \t\t Patota: %d\n", tripulante->id,tripulante->estado,tripulante->idPatota);
	}
	puts("");
}



void waitear_exec(){
	if (list_is_empty(execute)){
		return;
	}
	int i;
	for (i = 0; i < list_size(execute); i++) {
		Tripulante* tripulante = (Tripulante*) list_get(execute,i);
		sem_wait(&(tripulante->hilosEnEjecucion));
	}
}


void recorrer_lista_patota(t_list* lista){
	if (list_is_empty(lista)){
		puts("(cola vacia)\n");
		return;
	}
	int i;
	for (i = 0; i < list_size(lista); i++) {
		Patota* patota = (Patota*) list_get(lista,i);
		printf("Patota: %d \t\t desde: %d \t\t hasta: %d\n", patota->id,patota->inicio,patota->fin);
	}
	puts("");
}
void imprimir_estado_nave() {
	t_list* lista_ready = ready->elements;
	t_list* lista_bloq = bloqueados->elements;
	t_list* lista_new = new->elements;

	puts("------------------");
	char* tiempo = (char*) temporal_get_string_time("%d/%m/%y %H:%M:%S");
	printf("Estado de la nave: %s\n",tiempo);
	free(tiempo);

	puts("COLA NEW");
	pthread_mutex_lock(&sem_cola_new);
	recorrer_lista(lista_new);
	pthread_mutex_unlock(&sem_cola_new);

	puts("COLA READY:");
	pthread_mutex_lock(&sem_cola_ready);
	recorrer_lista(lista_ready);
	pthread_mutex_unlock(&sem_cola_ready);

	puts("COLA EXEC:");
	pthread_mutex_lock(&sem_cola_exec);
	recorrer_lista(execute);
	pthread_mutex_unlock(&sem_cola_exec);

	puts("COLA BLOQUEADOS:");
	pthread_mutex_lock(&sem_cola_bloqIO);
	recorrer_lista(lista_bloq);
	pthread_mutex_unlock(&sem_cola_bloqIO);

	puts("COLA EXIT:");
	pthread_mutex_lock(&sem_cola_exit);
	recorrer_lista(finalizado);
	pthread_mutex_unlock(&sem_cola_exit);

	puts("------------------");
}







int hacerConsola() {
	log_info(logger_discordiador,"Consola iniciada");
	//SIEMPRE HAY QUE SER CORTEZ Y SALUDAR
	puts("Bienvenido a A-MongOS de Cebollita Subcampeon \n");
	int ejecucion = 1;
	while (ejecucion) {
//leo los comandos
		char* linea = readline(">");
		char** codigo_dividido = string_n_split(linea,2," ");
		log_info(logger_discordiador,"comando %s ingresado",linea);
//		[0] CODIGO - [1] PARAMETROS - [2] NULL
		string_to_upper(codigo_dividido[0]);

		if(string_contains(codigo_dividido[0], "INICIAR_PATOTA")) {
			char** parametros_divididos = string_n_split(codigo_dividido[1],3," ");

//			[0] CANTIDAD_TRIPULANTES - [1] PATH_ARCHIVO - [2] POSICIONES - [3] NULL

			int cantidad_tripulantes= strtol(parametros_divididos[0],NULL,10);

			int cantidad_tareas=0;
			char* tareas = leer_tareas(parametros_divididos[1],&cantidad_tareas);
			if(tareas == NULL)
				continue;

			t_list* posiciones_iniciales = list_create();
			//todo//Los unicos paraametros que se usan en obtener parametros son linea y list_posicion
			completar_posiciones_iniciales(parametros_divididos[2],posiciones_iniciales);

			Patota* pato = iniciarPatota(p_totales, tareas,t_totales,cantidad_tripulantes);
			char* respuesta = enviar_iniciar_patota(pato,cantidad_tripulantes);
			if(!strcmp(respuesta,"fault")){
				free(respuesta);
				puts("no hay espacio en ram");
				log_info(logger_discordiador,"NO HAY ESPACIO PARA LA NUEVA PATOTA");
				free(parametros_divididos[0]);
				free(parametros_divididos[1]);
				free(parametros_divididos[2]);
				free(parametros_divididos);
				free(respuesta);
				free(pato->tareas);

				free(codigo_dividido[0]);
				free(codigo_dividido[1]);
				free(codigo_dividido);
				free(linea);
				list_destroy(posiciones_iniciales);
				continue;
			}

			list_add(listaPatotas,(void*) pato);
			log_info(logger_discordiador,"Se creo la patota %d y se agrego a la lista de patotas", pato->id);

			Tripulante* nuevo_tripulante;
			for(int i=0 ; i< cantidad_tripulantes;i++){
				uint8_t posicionX = obtener_pos(posiciones_iniciales);
				uint8_t posicionY = obtener_pos(posiciones_iniciales);
				printf("EStoy por crear al tripulante: %d en %d|%d\n",t_totales,posicionX,posicionY);
				nuevo_tripulante = crear_tripulante(t_totales, p_totales, posicionX, posicionY);
				puts("en teoria se pudo crear\n");
				pthread_mutex_lock(&sem_cola_new);
				queue_push(new,nuevo_tripulante);
				pthread_mutex_unlock(&sem_cola_new);

				enviar_tripulante(nuevo_tripulante);
				log_info(logger_discordiador,"Se creo un tripulante %d con estado: %s y pos inicial %d|%d:",nuevo_tripulante->id, nuevo_tripulante->estado,nuevo_tripulante->posicionX,nuevo_tripulante->posicionY);
				free(nuevo_tripulante->estado);
				pthread_mutex_lock(&sem_cola_ready);
				pthread_mutex_lock(&sem_cola_new);
				queue_push(ready,queue_pop(new));
				pthread_mutex_unlock(&sem_cola_new);
				pthread_mutex_unlock(&sem_cola_ready);
				log_info(logger_discordiador,"Se mueve al tripulante %d NEW a READY ",nuevo_tripulante->id);
				nuevo_tripulante->estado = strdup("READY");
				log_info(logger_discordiador,"Se le cambia el estado al tripulante %d a %s ",nuevo_tripulante->id,nuevo_tripulante->estado);

				log_info(logger_discordiador,"Se hace signal para avisar que hay elementos en READY");
				sem_post(&sem_tripulante_en_ready);

				//inicializamos su hilo y su semaforo
				sem_init(&(nuevo_tripulante->sem_pasaje_a_exec),NULL,0);
				sem_init(&(nuevo_tripulante->hilosEnEjecucion), 0, 0);
				pthread_create(&(nuevo_tripulante->hilo_vida), NULL, (void*) vivirTripulante, (void*) nuevo_tripulante);
				pthread_detach(nuevo_tripulante->hilo_vida);
				t_totales++;
			}
			p_totales++;

//			luego de enviar las tareas leidas a miram ya no nos interesa tenerlas en el discordiador
			free(pato->tareas);
			free(respuesta);
			free(parametros_divididos[0]);
			free(parametros_divididos[1]);
			free(parametros_divididos[2]);
			free(parametros_divididos);
			list_destroy(posiciones_iniciales);
		}


		if (string_contains(linea,"INICIAR_PLANIFICACION"))
		{
			estado_planificacion = 1;
			if(primerInicio)
			{
				pthread_create(&firstInit,NULL,(void*) iniciar_Planificacion,NULL);


				primerInicio=false;
			} else {
			int a = 0;
			//DEFINO UN SEM CONTADOR QUE NOS VA A SERVIR PARA PAUSAR LA PLANIFICACION DONDE QUERAMOS DESPUES
			postear_exec();
			sem_post(&(pararPlanificacion[0]));
			}
			sem_post(&pararIo);

		}
		if (string_contains(linea,"LISTAR") )
		{
				imprimir_estado_nave();

		}
		if (string_contains(linea,"PAUSAR_PLANIFICACION")) {
			estado_planificacion = 0;
			log_info(logger_discordiador,"SE PAUSA LA PLANIFICACION");
			// YA LO HICE LOL BASUCAMENTE LES TIRAS UN WAIT HASTA QUE LLEGUEN A 0 PARA QUE NO PUEDAN EJECUTAR
			sem_wait(&pararIo);
		}
		if (string_contains(linea, "EXPULSAR_TRIPULANTE"))
		{
			//BUENO ACA UN PEQUEÑO INTENTO DE TU TAREA DE MANEJO DE STRINGS PILI
			// FIJATE QUE SOLO SIRVE SI ES DE UN DIGIITO VAS A TENER QUE DIVIDIR EL ESTRING EN EL ESPACIO
			// Y FIJARTE SI EL SUBSTRING TIENE 1 O 2 CARACTERES
			int id_tripulante = strtol(codigo_dividido[1],NULL,10);
			Tripulante* tripulante_rip = buscar_tripulante(id_tripulante);
			if (tripulante_rip == NULL){
				log_info(logger_discordiador,"El tripulante %d ingresado no existe",id_tripulante);
				continue;
			}
			if (!strcmp(tripulante_rip->estado,"EXIT")){
				log_info(logger_discordiador,"El tripulante %d ya ha finalizado",id_tripulante);
				continue;
			}
			log_info(logger_discordiador,"Se encontro al tripulante a expulsar: %d en %s",tripulante_rip->id,tripulante_rip->estado);
			tripulante_rip->vida = false;
			//sem_post(&(tripulante_rip->sem_pasaje_a_exec));

			eliminarTripulante(tripulante_rip);
		}

		if (string_contains(linea,"OBTENER_BITACORA")){
			int socket_bitacora = conectarse_mongo();
			int tripulante_id = strtol(codigo_dividido[1],NULL,10);
			t_paquete* paquete_bitacora= crear_paquete(OBTENER_BITACORA);
			t_pedido_mongo* pedido_bitacora = malloc(sizeof(t_pedido_mongo));

			pedido_bitacora->id_tripulante = tripulante_id;
			pedido_bitacora->mensaje =string_new();
			pedido_bitacora->tamanio_mensaje = strlen(pedido_bitacora->mensaje) + 1;

			agregar_paquete_pedido_mongo(paquete_bitacora,pedido_bitacora);
			log_info(logger_conexiones, "el tripulante %d envia el mensaje OBTENER_BITCORA a MONGOSTORE",tripulante_id);
			char* bitacora = enviar_paquete_respuesta_string(paquete_bitacora,socket_bitacora);
			liberar_conexion(socket_bitacora);
			log_info(logger_conexiones, "el tripulante %d recibe su bitacora: %s",tripulante_id,bitacora);
			liberar_t_pedido_mongo(pedido_bitacora);
			free(bitacora);
		}

		if (string_contains(linea,"EXIT") || linea[0] == '\0'){
			ejecucion = 0;
			int socket_miram = conectarse_Mi_Ram();
			int socket_mongo = conectarse_mongo();
			t_paquete* paquete_miram = crear_paquete(FINALIZAR);
			t_paquete* paquete_mongo = crear_paquete(FINALIZAR);

			t_tripulante* estructura = malloc(sizeof(t_tripulante));
			estructura->id_tripulante = 0;
			estructura->id_patota = 0;
			estructura ->posicion_x = 0;
			estructura ->posicion_y = 0;

			agregar_paquete_tripulante(paquete_miram,estructura);
			agregar_paquete_tripulante(paquete_mongo,estructura);
			log_info(logger_conexiones,"Se emvia mensaje de tipo FINALIZAR a Mongostore y Miram");
			enviar_paquete(paquete_miram,socket_miram);
			enviar_paquete(paquete_mongo,socket_mongo);

			liberar_t_tripulante(estructura);
		}

		free(codigo_dividido[0]);
		free(codigo_dividido[1]);
		free(codigo_dividido);
		free(linea);
	}
	exit(1);
}
int main(int argc, char* argv[]) {
	ciclos_totales = 0;
	estado_planificacion= 0;
	iniciar_paths(argv[1]);
	log_roto = log_create(log_rotoo,"LOG_ROTO",false,LOG_LEVEL_INFO);
	//=========== LOGS ===============
	logger_discordiador = log_create(PATH_DISCORDIADOR_LOG, "DISCORDIADOR", false, LOG_LEVEL_INFO);		// Creamos log
	if (logger_discordiador == NULL) {
		exit(EXIT_FAILURE);  // log_create ya imprime por ṕantalla si hubo un error
	}
	puts("El archivo de logs DISCORDIADOR se creo correctamente");

	logger_tripulante = log_create(PATH_TRIPULANTE_LOG, "TRIPULANTE", false, LOG_LEVEL_INFO);		// Creamos log
	if (logger_tripulante == NULL) {
		exit(EXIT_FAILURE);  // log_create ya imprime por ṕantalla si hubo un error
	}
	puts("El archivo de logs CONEXIONES se creo correctamente");

	logger_conexiones = log_create(PATH_CONEXIONES_LOG, "CONEXIONES", false, LOG_LEVEL_INFO);		// Creamos log
	if (logger_conexiones == NULL) {
		exit(EXIT_FAILURE);  // log_create ya imprime por ṕantalla si hubo un error
	}
	puts("El archivo de logs CONEXIONES se creo correctamente");
	  config = config_create(PATH_CONFIG);														//Creamos config
	  if (config == NULL) {
	    puts("No se pudo crear el archivo de configuracion");
	    exit(EXIT_FAILURE);
	  }
	  puts("El archivo config se creo correctamente");

	  //=========== CONFIG ===============

	ipMiRam= config_get_string_value(config, "IP_MI_RAM_HQ");
	log_info(logger_conexiones, "la IP_MIRAM es: %s", ipMiRam);

	puertoMiRam = config_get_string_value(config, "PUERTO_MI_RAM_HQ");
	log_info(logger_conexiones, "El PUERTO miram es: %s", puertoMiRam);

	ipMongoStore = config_get_string_value(config, "IP_I_MONGO_STORE");
	log_info(logger_conexiones, "la IP_MONGOSTORE es: %s", ipMongoStore);

	puertoMongoStore = config_get_string_value(config, "PUERTO_I_MONGO_STORE");
	log_info(logger_conexiones, "El PUERTO mongostore es: %s", puertoMongoStore);

	puertoSabotaje = config_get_string_value(config, "PUERTO_SABOTAJE");
	log_info(logger_conexiones, "El puerto para el sabotaje es: %s",puertoSabotaje);

	multiProcesos = config_get_int_value(config, "GRADO_MULTITAREA");
	log_info(logger_discordiador, "El grado de multiprocesamiento es: %d",multiProcesos);

	retardoCpu = config_get_int_value(config, "RETARDO_CICLO_CPU");
	log_info(logger_discordiador, "El duracion del ciclo  de cpu: %d",retardoCpu);

	algoritmo = config_get_string_value(config, "ALGORITMO");
	log_info(logger_discordiador, "El algoritmo de planificacion es: %s",algoritmo);

	quantum = config_get_int_value(config, "QUANTUM");
	log_info(logger_discordiador, "El quantum por tripulante es: %d",quantum);

	//=========== INICIALIZACION DE SEMAFOROS===============
	sem_init(&multiProcesamiento, 0, multiProcesos);
	sem_init(&pararIo, 0, 0);
	sem_init(&(pararPlanificacion[0]),0,0);
	sem_init(&sem_tripulante_en_ready,0,0);

	pthread_mutex_init(&mutexIO,NULL);


	new = queue_create();
	pthread_mutex_init(&sem_cola_new, NULL);

	ready = queue_create();
	pthread_mutex_init(&sem_cola_ready, NULL);

	execute = list_create();
	pthread_mutex_init(&sem_cola_exec, NULL);

	bloqueados = queue_create();
	pthread_mutex_init(&sem_cola_bloqIO, NULL);

	finalizado = list_create();
	pthread_mutex_init(&sem_cola_exit, NULL);

	pthread_mutex_init(&socketMiRam, NULL);
	pthread_mutex_init(&socketMongo, NULL);

	//INICIALIZAMOS LAS PILAS Y COLAS RARAS QUE CREAMOS
	listaPatotas = list_create();

	bloqueados_sabotaje = list_create();

	pthread_t consola;
	pthread_create(&consola, NULL, (void *) hacerConsola, NULL);
//	pthread_join(consola,NULL);

	int socket_sabotaje = crear_server(puertoSabotaje);
	log_info(logger_conexiones,"Servidor abierto con el socket %d\n",socket_sabotaje);

	while(correr_programa)
	{
		cliente_sabotaje = esperar_cliente(socket_sabotaje,10);
		if (cliente_sabotaje == -1)
			continue;

		log_info(logger_conexiones, "Nueva conexion recibida con el socket: %d",cliente_sabotaje);
		pthread_create(&hilo_sabotaje,NULL,(void*) atender_sabotaje,cliente_sabotaje);
		pthread_join(hilo_sabotaje,NULL);


	}

	return 0;
}
	/* ADIO' wachin ! */

