/*
 * funcionesMemoria.h
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */

#ifndef FUNCIONESMEMORIA_H_
#define FUNCIONESMEMORIA_H_
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <shared/TAD_PATOTA.h>
#include <shared/TAD_TRIPULANTE.h>
#include "mi_ram_hq.h"

typedef struct
{
    int idPatota;
    int segmentoEnLocal;
    int *inicio;
    int tamanio;

} segmentoEnTablaGlobal_struct;
typedef struct
{
    int *inicio;
    int tamanio;

} segmentoEnTabla_struct;
typedef struct
{
    int ID;
    int PID;
    int segmentoOPagina;
    int offsetEnPagina;
    char tipo;
    int tamanio;
} elementoEnLista_struct;
typedef struct
{
    void *ptrHuecoLibre;
    int tamanio;
} espacio_struct;


typedef struct{
    int frame;
    int espacioOcupado;
    unsigned int presencia;
}paginaEnTabla_struct;
typedef struct {
    unsigned int idPatota;
    t_list *tablaDePaginas;
}tablaEnLista_struct;
typedef struct{
    int nroFrame;
    int PID;
    int nroPagina;
    unsigned int uso;
}paginaParaReemplazar_struct;
typedef struct{
	int tamanio;
	int *inicio;
}huecoLibreEnLista_struct;
t_list *listaDeTablasDePaginas;
t_list *listaGlobalDeSegmentos;
t_list *listaSegmentos;
t_list *listaElementos;
t_list *listaHuecosLibres;
t_queue *tablaDeFrames;
void compactacion();
int calcular_direccion_logica_archivo(int);
int calcular_direccion_logica_patota(int);
int espacioLibre;
void *minimo_segmentos_en_tabla(void *, void *);
void *minimo_hueco_libre(void *, void *);
void guardar_en_memoria_segmentacion(void*,int,int,uint32_t,char,int);
void borrar_de_memoria_segmentacion(int, int,char);
bool ordenar_por_posicion(void *, void *);
bool encontrarTablaDePaginas(void*);
bool filtrarPorTipo(void*);
void *buscar_de_memoria_segmentacion(int,int,char);
char tipoUniversal;
int patotaUniversal;
int tamPayloadUniversal;
void guardar_en_memoria_general(void* ,int ,int ,int ,char );
int sacarPaginaDeMemoria();
void guardar_en_memoria_paginacion(void*,int,int,int,char);
void* buscar_en_memoria_paginacion(int,int, char);
void *borrar_de_memoria_paginacion(int, int, char);
void* buscar_en_memoria_general(int ,int , char );
void *borrar_de_memoria_general(int , int , char );
void guardar_en_swap(void*,int,int,int,char);
void actualizar_estado_segmentacion(uint32_t,uint32_t,char);
void actualizar_estado_paginacion(uint32_t,uint32_t,char);
void actualizar_posicion_segmentacion(uint32_t, uint32_t, uint32_t,uint32_t);
void actualizar_lista_elementos_segmentacion(int,int);
int punteroReemplazo;
void actualizar_indice_segmentacion(uint32_t idElemento, uint32_t idPatota);

#endif /* FUNCIONESMEMORIA_H_ */
