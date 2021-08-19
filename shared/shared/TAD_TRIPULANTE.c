/*
 * TAD_TRIPULANTE.c
 *
 *  Created on: 6 may. 2021
 *      Author: utnso
 */

#ifndef TAD_TRIPULANTE


#include "TAD_TRIPULANTE.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/temporal.h>
#include <stdint.h>
#include <pthread.h>

struct tareaTripulante
{
	uint8_t idTripulante;
	uint8_t idPatota;
};

Tripulante* tripulanteCreate(uint8_t id,uint8_t idPa, uint8_t posicionX,uint8_t posicionY)
 {
	Tripulante* devolverTripulante=malloc(sizeof(Tripulante));
	devolverTripulante->id=id;
	devolverTripulante->idPatota=idPa;
	devolverTripulante->posicionX= posicionX;
	devolverTripulante->posicionY=posicionY;
	devolverTripulante->vida=true;
	devolverTripulante->estado=strdup("New");
	return devolverTripulante ;
 }

Tripulante* crear_tripulante(uint8_t id_tripulante, uint8_t id_patota,uint8_t posicionX,uint8_t posicionY){
	Tripulante* tripulante =malloc(sizeof(Tripulante));
	tripulante->id = id_tripulante;
	tripulante->idPatota = id_patota;
	tripulante->estado = strdup("NEW");
	tripulante->vida= true;
	tripulante->Tarea = NULL;
	tripulante->posicionX = posicionX;
	tripulante->posicionY = posicionY;
	tripulante->espera = 0;
	tripulante->kuantum=0;
	tripulante->esta_sabotaje = 0;

	return tripulante;
}


 void mostrarTripulante(Tripulante* tripulante)
 {

 	printf ("Patota: %i Tripulante: %i Estado: %s ",tripulante->idPatota,tripulante->id,tripulante->estado);
 }



#endif

