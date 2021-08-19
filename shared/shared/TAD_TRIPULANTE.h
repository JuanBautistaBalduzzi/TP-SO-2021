/*
 * TAD_TRIPULANTE.h
 *
 *  Created on: 6 may. 2021
 *      Author: utnso
 */

#ifndef SRC_TAD_TRIPULANTE_H_
#define SRC_TAD_TRIPULANTE_H_
#include <commons/collections/list.h>
#include <stdbool.h>
#include <commons/collections/node.h>
#include <stdio.h>
#include <stdlib.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <stdint.h>
#include <semaphore.h>

typedef struct {
	char* nombre;
	bool 	es_io;
	uint8_t parametro;
	uint8_t posicion_x;
	uint8_t posiciion_y;
	uint8_t duracion;

} tarea_tripulante;

typedef struct
{
	uint8_t idTripulante;
	uint8_t idPatota;

} tareaTripulante;

typedef struct {
	uint8_t id;
	uint8_t idPatota;
	char* estado;
	_Bool vida;
	tarea_tripulante* Tarea;
	uint8_t posicionX;
	uint8_t posicionY;
	int espera;
	int kuantum;
	pthread_t hilo_vida;
	sem_t sem_pasaje_a_exec;
	sem_t hilosEnEjecucion;
	bool primer_inicio;
	bool esta_sabotaje;
} Tripulante;

typedef struct {
	uint32_t id;
	char* estado;
	uint32_t posX;
	uint32_t posY;
	uint32_t proxTarea;
	uint32_t dirLogicaPcb;
}tcb;

Tripulante* tripulanteCreate(uint8_t id, uint8_t idPa, uint8_t posicionX,uint8_t posicionY);

Tripulante* crear_tripulante(uint8_t id_tripulante, uint8_t id_patota,uint8_t posicionX,uint8_t posicionY);

void mostrarTripulante(Tripulante* tripulante);

void eliminar_Tripulante(Tripulante* tripulante);


#endif /* SRC_TAD_TRIPULANTE_H_ */
