#ifndef SRC_CONEXION_H_
#define SRC_CONEXION_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>


#include "TAD_PATOTA.h"
#include "TAD_TRIPULANTE.h"

/*
 * Creo el enum para no tener que pensar si devuelvo un "uno" o un "cero""
 */

#define ERROR 0
#define OK 1
//typedef enum {
//	ERROR,
//	OK
//} RESPUESTA;
// NO SE PORQUE ECLIPSE ME TOMA MAL LOS ENUM LPM



typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer;

typedef struct {
    uint8_t codigo_operacion;
    t_buffer* buffer;
} t_paquete;
typedef enum {
	MIRAM,
	MONGOSTORE,
	DISCORDIADOR
}CONEXION_A;
/*========================== TIPOS DE MENSAJES =================================*/
typedef enum {
/* 0 */	INICIAR_PATOTA, //Es lo mismo que iniciar_paota?		//LISTO
/* 1 */	TRIPULANTE,												//LISTO
/* 2 */	PEDIR_TAREA,											//LISTO
/* 3 */	ACTUALIZAR_POS,											//LISTO
/* 4 */ ELIMINAR_TRIPULANTE,									//LISTO
/* 5 */ ACTUALIZAR_ESTADO,										//LISTO
/* 6 */ OBTENER_BITACORA,										//LISTO
/* 7 */ MOVIMIENTO_MONGO,										//LISTO
/* 8 */ SABOTAJE,												//LISTO
/* 9 */ INICIO_TAREA,											//LISTO
/* 10 */ FIN_TAREA,												//LISTO
/* 11 */ INICIO_SABOTAJE,										//LISTO
/* 12 */ FIN_SABOTAJE,											//LISTO
/* 13 */ CONSUMIR_RECURSO,										//LISTO
/* 14 */ FINALIZAR,												//LISTO
/* 15 */ FIN_PATOTA												//LISTO


}CODE_OP;

typedef struct
{
	uint8_t idTripulante;
	uint8_t posX;
	uint8_t posY;
}id_and_pos;

typedef struct
{
	uint32_t idTripulante;
	uint32_t estado_length;
	char* estado;
}cambio_estado;

typedef struct
{
	uint8_t idPatota;
	uint8_t cantTripulantes;
	uint32_t tamanio_tareas;
	char* Tareas;
//	FILE* Tareas;
}t_iniciar_patota;

typedef struct
{
	uint8_t id_tripulante;
	uint8_t id_patota;
	uint8_t posicion_x;
	uint8_t posicion_y;

}t_tripulante;

typedef struct {

	uint8_t id_tripulante;
	uint8_t id_patota;
	char estado;

}t_cambio_estado;

typedef struct {

	uint8_t id_tripulante;
	uint32_t tamanio_mensaje;
	char* mensaje;

}t_pedido_mongo;
typedef struct {
	uint8_t id_tripulante;
	uint8_t origen_x;
	uint8_t origen_y;
	uint8_t destino_x;
	uint8_t destino_y;

}t_movimiento_mongo;

typedef struct {
	uint8_t cantidad;
	char tipo;
	char consumible;

}t_consumir_recurso;

/*========================== TODOS =================================*/
void crear_buffer(t_paquete* paquete);
t_paquete* crear_paquete(CODE_OP codigo);
int enviar_paquete(t_paquete* paquete, int socket_cliente);
t_paquete* recibir_paquete(int socket_cliente, int* respuesta);

void liberar_conexion(int);
void eliminar_paquete(t_paquete* paquete);
/*========================== CLIENTE =================================*/
int crear_conexion(char*, char*);
int cliente(CONEXION_A modulo);

/*========================== SERVIDOR =================================*/
int crear_server(char*);
int esperar_cliente(int,int);

/*========================== MANEJO DE PAQUETES =================================*/
void* serializar_paquete(t_paquete* paquete, int bytes);

//---------------------------- INICIAR_PATOTA ----------------------------
void agregar_paquete_iniciar_patota(t_paquete* paquete, t_iniciar_patota* estructura);
t_iniciar_patota* deserializar_iniciar_patota(t_paquete* paquete);
void imprimir_paquete_iniciar_patota(t_iniciar_patota* estructura);
void liberar_t_iniciar_patota(t_iniciar_patota* estructura);



//--------------------------- TRIPULANTE -----------------------------
void agregar_paquete_tripulante(t_paquete* paquete, t_tripulante* estructura);
t_tripulante* deserializar_tripulante(t_paquete* paquete);
void imprimir_paquete_tripulante(t_tripulante* estructura);
void liberar_t_tripulante(t_tripulante* estructura);

//--------------- ELIMINAR_TRIPULANTE ----------------------------------------

//--------------- MOVIMIENTO_TRIPULANTE ----------------------------------------

//------------------- CAMBIO_ESTADO -------------------------------------
void agregar_paquete_cambio_estado(t_paquete* paquete, t_cambio_estado* estructura);
t_cambio_estado* deserializar_cambio_estado(t_paquete* paquete);
void imprimir_paquete_cambio_estado(t_cambio_estado* estructura);
void liberar_t_cambio_estado(t_cambio_estado* estructura);


//---------------------- SOLICITAR TAREA ----------------------------------
char* enviar_paquete_respuesta_string(t_paquete* paquete,int socket);

/*
 * CAPAZ PODEMOS USAR UN T_TRIPULANTE O T_ELIMINAR_TRIPULANTE
 */

//-----------------------PEDIDO_MONGO---------------------------------

void agregar_paquete_pedido_mongo(t_paquete* paquete, t_pedido_mongo* estructura);
t_pedido_mongo* deserializar_pedido_mongo(t_paquete* paquete);
void imprimir_pedido_mongo(t_pedido_mongo* estructura);
void liberar_t_pedido_mongo(t_pedido_mongo* estructura);

//----------------------- MOVIMIENTO_MONGO ---------------------------------
void agregar_paquete_movimiento_mongo(t_paquete* paquete, t_movimiento_mongo* estructura);
t_movimiento_mongo* deserializar_movimiento_mongo(t_paquete* paquete);
void imprimir_movimiento_mongo(t_movimiento_mongo* estructura);
void liberar_t_movimiento_mongo(t_movimiento_mongo* estructura);

//----------------------- CONSUMIR RECURSOS ---------------------------------
void agregar_paquete_consumir_recurso(t_paquete* paquete, t_consumir_recurso* estructura);
t_consumir_recurso* deserializar_consumir_recurso(t_paquete* paquete);
void imprimir_consumir_recurso(t_consumir_recurso* estructura);
void liberar_t_consumir_recurso(t_consumir_recurso* estructura);



#endif /* SRC_CONEXION_H_ */
