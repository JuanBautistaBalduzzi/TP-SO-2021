/*
 * mi_ram_hq.h
 *
 *  Created on: 29 may. 2021
 *      Author: utnso
 */

#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include <nivel-gui/nivel-gui.h>
#include <nivel-gui/tad_nivel.h>
#include <curses.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <shared/conexion.h>
#include <shared/TAD_PATOTA.h>
#include <shared/TAD_TRIPULANTE.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/string.h>
#include <signal.h>
#include <unistd.h>
#include "funcionesMemoria.h"

#define PUERTOPREDETERMINADO 6667

typedef struct
{
   uint32_t pid;
   uint32_t* tareas ;
}PCB;

typedef struct
{
   uint32_t tid;
   char* estado;
   uint32_t posx;
   uint32_t posy;
   uint32_t* prox_tarea;
   uint32_t* puntero_pcb;
}TCB;

NIVEL* crear_mapa();
void administrar_cliente(int);
char intAChar(int);
void dibujarTripulante(tcb*,char);
void borrarTripulante(char);
void actualizarPosicion(id_and_pos* ,char );
void dumpDeMemoria();
void manejoDump(int);
void manejoCompactacion(int);
NIVEL* nivel;
t_config* config;
t_log* logger;
t_log* logger2;
int vectorIdTripulantes[94];
int tamMemoria;
char* esquemaMemoria;
int tamPagina;
int tamSwap;
char* path_swap;
char* alg_remplazo;
char* crit_seleccion;
char* puerto;
bool funcionando;

pthread_mutex_t mutexMemoria;
pthread_mutex_t mutexMapa;
pthread_mutex_t mutexEstado;
pthread_mutex_t mutexEnviarTarea;



void *memoria;
void* memoriaSwap;
int *bitarrayMemoria;
int *bitarraySwap;
int cantidadPaginas;
int cantidadPaginasSwap;
char *algoritmoReemplazo;
int tipoDeGuardado;

#define BESTFIT 1
#define FIRSTFIT 2

#endif /* MI_RAM_HQ_H_ */
