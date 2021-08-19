/*
 * serializacion.c
 *
 *  Created on: 6 may. 2021
 *      Author: utnso
 */
#include "conexion.h"
#include "TAD_PATOTA.h"
#include "TAD_TRIPULANTE.h"

//==============================INICIAR_PATOTA========================================
void agregar_paquete_iniciar_patota(t_paquete* paquete, t_iniciar_patota* estructura){
	int offset = 0;
	paquete->buffer->size += sizeof(uint32_t) + sizeof(uint8_t)*2 + estructura->tamanio_tareas;

	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->idPatota), sizeof(uint8_t));		//El entero para el idpatota
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->cantTripulantes), sizeof(uint8_t));		//El entero de la cantidad de tcb
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->tamanio_tareas), sizeof(uint32_t));		//El entero para el tamanio de tareas
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, estructura->Tareas, estructura->tamanio_tareas);	//el archivo de tareas


}

t_iniciar_patota* deserializar_iniciar_patota(t_paquete* paquete){

	t_iniciar_patota* estructura = malloc(sizeof(t_iniciar_patota));
	int offset = 0;

	memcpy(&(estructura->idPatota), paquete->buffer->stream, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->cantTripulantes), paquete->buffer->stream + offset , sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->tamanio_tareas), paquete->buffer->stream + offset , sizeof(uint32_t));
	offset += sizeof(uint32_t);

	estructura->Tareas = (char*) malloc( (int) estructura->tamanio_tareas); //Es necesario  el casteo a (int)??
	memcpy(estructura->Tareas, paquete->buffer->stream + offset , estructura->tamanio_tareas);

	eliminar_paquete(paquete);
	return estructura;

}

void imprimir_paquete_iniciar_patota(t_iniciar_patota* estructura){
	printf("ID PATOTA: %d\n",estructura->idPatota);
	printf("CANTIDAD DE TRIPULANTES: %d\n",estructura->cantTripulantes);
	printf("TAMAÑO DE TAREAS: %d\n",estructura->tamanio_tareas);
	printf("TAREAS: %s\n",estructura->Tareas);
	puts("");
}

void liberar_t_iniciar_patota(t_iniciar_patota* estructura){
	free(estructura->Tareas);
	free(estructura);
}

//==============================TRIPUALNTE========================================

void agregar_paquete_tripulante(t_paquete* paquete, t_tripulante* estructura){
	int offset = 0;

	paquete->buffer->size += sizeof(uint8_t)*4;
	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->id_tripulante), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->id_patota), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->posicion_x), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->posicion_y), sizeof(uint8_t));
}

t_tripulante* deserializar_tripulante(t_paquete* paquete){
	t_tripulante* estructura = malloc(sizeof(Tripulante));
	int offset = 0;

    memcpy(&(estructura->id_tripulante), paquete->buffer->stream, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&(estructura->id_patota), paquete->buffer->stream + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&(estructura->posicion_x), paquete->buffer->stream + offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&(estructura->posicion_y), paquete->buffer->stream + offset, sizeof(uint8_t));

	eliminar_paquete(paquete);
	return estructura;


}

void imprimir_paquete_tripulante(t_tripulante* estructura){
	printf("ID TRIPULANTE: %d\n",estructura->id_tripulante);
	printf("ID PATOTA: %d\n",estructura->id_patota);
	printf("POSICION EN X: %d\n",estructura->posicion_x);
	printf("POSICION EN Y: %d\n",estructura->posicion_y);
	puts("");
}

void liberar_t_tripulante(t_tripulante* estructura){
	free(estructura);
}
//==============================ELIMINAR_TRIPUALNTE========================================

//============================== CAMBIO_ESTADO ========================================

void agregar_paquete_cambio_estado(t_paquete* paquete, t_cambio_estado* estructura){
	int offset = 0;

	paquete->buffer->size += sizeof(uint8_t) * 2 + sizeof(char);
	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->id_tripulante), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->id_patota), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream+offset, &(estructura->estado), sizeof(char));

}

t_cambio_estado* deserializar_cambio_estado(t_paquete* paquete){
	t_cambio_estado* estructura = malloc(sizeof(t_cambio_estado));
	int offset = 0;

    memcpy(&(estructura->id_tripulante), paquete->buffer->stream, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&(estructura->id_patota), paquete->buffer->stream+offset, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(&(estructura->estado), paquete->buffer->stream + offset, sizeof(char));

	eliminar_paquete(paquete);
	return estructura;

}
void imprimir_paquete_cambio_estado(t_cambio_estado* estructura){
	printf("ID TRIPULANTE: %d\n",estructura->id_tripulante);
	printf("ID PATOTA: %d\n",estructura->id_patota);
	printf("ESTADO: %c\n",estructura->estado);
	puts("");
}
void liberar_t_cambio_estado(t_cambio_estado* estructura){
	free(estructura);
}

//============================== PEDIDO_MONGO ========================================
void agregar_paquete_pedido_mongo(t_paquete* paquete, t_pedido_mongo* estructura){
	int offset = 0;

	paquete->buffer->size += sizeof(uint8_t) + sizeof(uint32_t) + estructura->tamanio_mensaje;
	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->id_tripulante), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->tamanio_mensaje), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + offset, estructura->mensaje, estructura->tamanio_mensaje);


}
t_pedido_mongo* deserializar_pedido_mongo(t_paquete* paquete){
	t_pedido_mongo* estructura = malloc(sizeof(t_pedido_mongo));
	int offset = 0;

	memcpy(&(estructura->id_tripulante), paquete->buffer->stream, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->tamanio_mensaje), paquete->buffer->stream+offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	estructura->mensaje = malloc(estructura->tamanio_mensaje);
	memcpy(estructura->mensaje, paquete->buffer->stream + offset, estructura->tamanio_mensaje);

	eliminar_paquete(paquete);
	return estructura;

}
void imprimir_pedido_mongo(t_pedido_mongo* estructura){
	printf("ID TRIPULANTE: %d\n",estructura->id_tripulante);
	printf("TAMANIO MENSAJE: %d\n",estructura->tamanio_mensaje);
	printf("MENSAJE: %s\n",estructura->mensaje);
	puts("");
}

void liberar_t_pedido_mongo(t_pedido_mongo* estructura){
	free(estructura->mensaje);
	free(estructura);
}

//============================== MOVIMIENTO_MONGO ========================================
void agregar_paquete_movimiento_mongo(t_paquete* paquete, t_movimiento_mongo* estructura){
	int offset = 0;

	paquete->buffer->size += sizeof(uint8_t) * 5;
	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->id_tripulante), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset , &(estructura->origen_x), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset , &(estructura->origen_y), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset , &(estructura->destino_x), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset, &(estructura->destino_y), sizeof(uint8_t));

}

t_movimiento_mongo* deserializar_movimiento_mongo(t_paquete* paquete){
	t_movimiento_mongo* estructura = malloc(sizeof(t_movimiento_mongo));
	int offset = 0;

	memcpy(&(estructura->id_tripulante), paquete->buffer->stream, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->origen_x), paquete->buffer->stream + offset , sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->origen_y), paquete->buffer->stream + offset , sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->destino_x), paquete->buffer->stream + offset , sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->destino_y), paquete->buffer->stream + offset , sizeof(uint8_t));

	eliminar_paquete(paquete);
	return estructura;


}
void imprimir_movimiento_mongo(t_movimiento_mongo* estructura){
	printf("ID TRIPULANTE: %d\n",estructura->id_tripulante);
	printf("ORIGEN X: %d\n",estructura->origen_x);
	printf("ORIGEN Y: %d\n",estructura->origen_y);
	printf("DESTINO X: %d\n",estructura->destino_x);
	printf("DESTINO Y: %d\n",estructura->destino_y);

}
void liberar_t_movimiento_mongo(t_movimiento_mongo* estructura){
	free(estructura);
}

//============================== CONSUMIR_MONGO ========================================

void agregar_paquete_consumir_recurso(t_paquete* paquete, t_consumir_recurso* estructura){
	int offset = 0;

	paquete->buffer->size += sizeof(uint8_t) + sizeof(char)*2;
	paquete->buffer->stream = realloc(paquete->buffer->stream,paquete->buffer->size);

	memcpy(paquete->buffer->stream, &(estructura->cantidad), sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(paquete->buffer->stream + offset , &(estructura->tipo), sizeof(char));
	offset += sizeof(char);
	memcpy(paquete->buffer->stream + offset , &(estructura->consumible), sizeof(char));

}
t_consumir_recurso* deserializar_consumir_recurso(t_paquete* paquete){
	t_consumir_recurso* estructura = malloc(sizeof(t_movimiento_mongo));
	int offset = 0;

	memcpy(&(estructura->cantidad), paquete->buffer->stream, sizeof(uint8_t));
	offset += sizeof(uint8_t);
	memcpy(&(estructura->tipo), paquete->buffer->stream + offset , sizeof(char));
	offset += sizeof(char);
	memcpy(&(estructura->consumible), paquete->buffer->stream + offset , sizeof(char));

	eliminar_paquete(paquete);
	return estructura;
}
void imprimir_consumir_recurso(t_consumir_recurso* estructura){
	printf("CANTIDAD: %d\n",estructura->cantidad);
	printf("TIPO: %d\n",estructura->tipo);
	printf("RECURSO: %d\n",estructura->consumible);

}
void liberar_t_consumir_recurso(t_consumir_recurso* estructura){
	free(estructura);
}


//============================== 				  ========================================
