/*
 * TAD_PATOTA.c
 *
 *  Created on: 6 may. 2021
 *      Author: utnso
 */

#ifndef TAD_PATOTA
#include "TAD_PATOTA.h"
#include <stdio.h>
#include <stdlib.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/temporal.h>
#include "TAD_TRIPULANTE.h"


struct tareasPatota
{
	uint8_t id;
	uint32_t tareas_length;
	char* tareas;
};
struct Patota
{
	uint8_t id;
	char* tareas;
};
struct pcb
{
	uint32_t id;
	uint32_t tareas;
};

//void agregarTripulantes(int cantidad_tripulantes, Patota* patota ,t_list* lista_posiciones_tripulantes,uint8_t id_tripulantes)
//{
//	uint8_t i=0;
//	while (i < cantidad)
//	{
//		if(list_is_empty(listaTripulantes)!= true)
//		{
//		tripu++;
//		int pos_x= (int)list_remove(listaTripulantes,NULL);
//		int pos_y=  (int)list_remove(listaTripulantes,NULL);
//		patota->tripulacion[i]=tripulanteCreate(tripu,patota->id,(uint8_t)pos_x, (uint8_t)pos_y);
//
//		}
//		else
//		{
//			tripu++;
//			uint8_t pos =0;
//			patota->tripulacion[i] =tripulanteCreate(tripu,patota->id,pos,pos);
//
//		};
//		i++;
//	}
//}
 Patota* iniciarPatota(int id_patota,char* tareas,int inicio, int fin){

	Patota* devolverPatota = malloc(sizeof(Patota));
	devolverPatota->id = id_patota;
	devolverPatota->tareas = tareas;
	devolverPatota->inicio = inicio;
	devolverPatota->fin = inicio + fin - 1;
	devolverPatota->cantidad_tripulantes = fin;


	return devolverPatota;
}


#endif
