/*
 * funcionesMemoria.c
 *
 *  Created on: 21 jul. 2021
 *      Author: utnso
 */
#include "funcionesMemoria.h"

int menorEntreDos(int elem1, int elem2){
    if(elem1 > elem2){
        return elem2;
    }
    else
        return elem1;
}
int encontrarFrameDisponible(){
    for (int j = 0; j < cantidadPaginas; ++j) {
        if (bitarrayMemoria[j] == 0){
            return j;
        }
    }
    return -1;
}

int encontrarFrameEnSwapDisponible(){
    int i=0;
    while (bitarraySwap[i]==1){
        if (i==tamSwap/tamPagina){
            break;
        }
        i++;
    }
    return i;
}
int calcular_direccion_logica_archivo( int idPatota){
	pthread_mutex_lock(&mutexMemoria);
	tipoUniversal ='A';
	t_list *listaFiltrada;
	listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
	elementoEnLista_struct *elementoBuscado = malloc(sizeof(elementoEnLista_struct));
	for(int i =0; i< list_size(listaFiltrada);i++){
		elementoBuscado = list_get(listaFiltrada,i);
		if(elementoBuscado->ID==idPatota){
			break;
		}
	}
	pthread_mutex_unlock(&mutexMemoria);

	return (elementoBuscado->segmentoOPagina+elementoBuscado->offsetEnPagina);


}
int calcular_direccion_logica_patota(int idPatota){
	pthread_mutex_lock(&mutexMemoria);
	tipoUniversal ='P';
	t_list *listaFiltrada;
	listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
	elementoEnLista_struct *elementoBuscado = malloc(sizeof(elementoEnLista_struct));
	for(int i =0; i< list_size(listaFiltrada);i++){
		elementoBuscado = list_get(listaFiltrada,i);
		if(elementoBuscado->ID==idPatota){
			break;
		}
	}
	pthread_mutex_unlock(&mutexMemoria);

	return (elementoBuscado->segmentoOPagina+elementoBuscado->offsetEnPagina);
}

void guardar_en_memoria_general(void* payload,int idElemento,int tamPayload,int pid,char tipo){
    pthread_mutex_lock(&mutexMemoria);
    espacioLibre -= tamPayload;
    if (strcmp(esquemaMemoria,"PAGINACION")==0){
		guardar_en_memoria_paginacion(payload, idElemento, tamPayload, pid, tipo);

	}else if(strcmp(esquemaMemoria,"SEGMENTACION")==0){
		guardar_en_memoria_segmentacion( payload, idElemento, tamPayload, pid, tipo,tipoDeGuardado);
	}
    pthread_mutex_unlock(&mutexMemoria);


}

void* buscar_en_memoria_general(int idElementoABuscar,int PID, char tipo){
	void* retornar;
    pthread_mutex_lock(&mutexMemoria);
    if (strcmp(esquemaMemoria,"PAGINACION")==0){
		retornar =  buscar_en_memoria_paginacion( idElementoABuscar, PID,  tipo);

	}else if(strcmp(esquemaMemoria,"SEGMENTACION")==0){
		retornar =  buscar_de_memoria_segmentacion( idElementoABuscar, PID,tipo);
	}
    pthread_mutex_unlock(&mutexMemoria);
	return retornar;



}
void *borrar_de_memoria_general(int idElemento, int idPatota, char tipo){
    pthread_mutex_lock(&mutexMemoria);

	if (strcmp(esquemaMemoria,"PAGINACION")==0){
		borrar_de_memoria_paginacion( idElemento, idPatota,  tipo);

	}else if(strcmp(esquemaMemoria,"SEGMENTACION")==0){
		borrar_de_memoria_segmentacion(idElemento,idPatota,tipo);
	}
    pthread_mutex_unlock(&mutexMemoria);


}

void guardar_en_memoria_paginacion(void* payload,int idElemento,int tamPayload,int pid,char tipo){
	loggearTablaDeFrames();
	int indicePaginaCorrespondiente;
    int indiceTablaCorrespondiente;
    elementoEnLista_struct *nuevoElemento= malloc(sizeof(elementoEnLista_struct));
    nuevoElemento->tipo = tipo;
    nuevoElemento->PID = pid;
    tablaEnLista_struct *tablaCorrespondiente = malloc(sizeof(tablaEnLista_struct));
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaEnLista_struct *tablaIterante;
        tablaIterante = list_get(listaDeTablasDePaginas,i);
        if(tablaIterante->idPatota == pid){
            tablaCorrespondiente = tablaIterante;
            indiceTablaCorrespondiente = i;
            break;
        }

    }

    paginaEnTabla_struct *paginaParcialmenteLlena = malloc(sizeof (paginaEnTabla_struct));
    paginaParcialmenteLlena->frame = -1;
    for (int i = 0; i < list_size(tablaCorrespondiente->tablaDePaginas); ++i) {
        paginaEnTabla_struct *paginaIterante;
        paginaIterante = list_get(tablaCorrespondiente->tablaDePaginas,i);
        if(paginaIterante->espacioOcupado < tamPagina && paginaIterante->presencia==1){
            paginaParcialmenteLlena = paginaIterante;
            indicePaginaCorrespondiente = i;
            break;
        }

    }

    int payLoadYaGuardado = 0;
    int *direccionFisica;
    if (paginaParcialmenteLlena->frame != -1){
        paginaParaReemplazar_struct *paginaReemplazable3 = malloc(sizeof(paginaParaReemplazar_struct));
        paginaReemplazable3->uso=1;
        paginaReemplazable3->PID=pid;
        direccionFisica = (int)memoria + (int)(paginaParcialmenteLlena->frame * tamPagina + paginaParcialmenteLlena->espacioOcupado);
        int menorEntre2 = menorEntreDos(tamPayload,(tamPagina-paginaParcialmenteLlena->espacioOcupado));
        memcpy(direccionFisica,payload,menorEntre2);
        nuevoElemento->offsetEnPagina = paginaParcialmenteLlena->espacioOcupado;
        nuevoElemento->segmentoOPagina = indicePaginaCorrespondiente;
        nuevoElemento->tamanio = tamPayload;
        if(tipo=='A'){
        	//log_info(logger,"Tamanio de la tarea que le meto a la lista de elementos: %d y que recibo como parametro: %d",nuevoElemento->tamanio,tamPayload);
        }
        nuevoElemento->ID = idElemento;
        list_add(listaElementos,nuevoElemento);
        if(tipo=='A'){
        	nuevoElemento = list_get(listaElementos,list_size(listaElementos)-1);
        	//log_info(logger,"Tamanio de la tarea despues de guardar %d",nuevoElemento->tamanio);
        }
        payLoadYaGuardado += menorEntreDos(tamPayload,tamPagina-paginaParcialmenteLlena->espacioOcupado);
        payload = (int)payload + menorEntreDos(tamPayload,tamPagina-paginaParcialmenteLlena->espacioOcupado);
        paginaParcialmenteLlena->espacioOcupado += menorEntreDos(tamPayload,tamPagina - paginaParcialmenteLlena->espacioOcupado);
        list_replace(tablaCorrespondiente->tablaDePaginas,indicePaginaCorrespondiente,paginaParcialmenteLlena);
        while (payLoadYaGuardado<tamPayload) {
            int frameDisponible = encontrarFrameDisponible();
            if (frameDisponible == -1){
                frameDisponible = sacarPaginaDeMemoria();
            	//guardar_en_swap(payload,idElemento,tamPayload-payLoadYaGuardado,pid,tipo);
            }
            paginaReemplazable3->uso=1;
            paginaReemplazable3->PID=pid;
            paginaReemplazable3->nroFrame = frameDisponible;
            direccionFisica = (int)memoria + (int)(frameDisponible * tamPagina);
            menorEntre2 = menorEntreDos(tamPagina,tamPayload-payLoadYaGuardado);
            memcpy(direccionFisica,payload, menorEntre2);
            paginaEnTabla_struct *nuevaPagina = malloc(sizeof (paginaEnTabla_struct));
            nuevaPagina->frame = frameDisponible;
            nuevaPagina->presencia = 1;
            nuevaPagina->espacioOcupado = menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payLoadYaGuardado = (int)payLoadYaGuardado + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payload = (int)payload + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            paginaReemplazable3->nroPagina = list_add(tablaCorrespondiente->tablaDePaginas,nuevaPagina);
            ////log_info(logger,"Uso de la pagina que pushea: %d",paginaReemplazable3->uso);
            queue_push(tablaDeFrames,paginaReemplazable3);
            bitarrayMemoria[frameDisponible] = 1;
            paginaParaReemplazar_struct *paginaDePrueba = malloc(sizeof(paginaParaReemplazar_struct));
            //log_info(logger2,"guardo la pagina %d del proceso %d",paginaReemplazable3->nroPagina,paginaReemplazable3->PID);
            //loggearTablaDeFrames();

        }

    }
    else {
        int frameDisponible = encontrarFrameDisponible();
        paginaParaReemplazar_struct *paginaReemplazable = malloc(sizeof(paginaParaReemplazar_struct));
        paginaReemplazable->uso=1;
        paginaReemplazable->PID=pid;
        if (frameDisponible == -1) {
            frameDisponible = sacarPaginaDeMemoria();
        	//guardar_en_swap(payload, idElemento, tamPayload, pid, tipo);
        }
            int *direccionFisica = memoria + (frameDisponible * tamPagina);
            memcpy(direccionFisica, payload, menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina));
            paginaEnTabla_struct *nuevaPagina = malloc(sizeof(paginaEnTabla_struct));
            nuevaPagina->frame = frameDisponible;
            nuevaPagina->presencia = 1;
            nuevaPagina->espacioOcupado = menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
            paginaReemplazable->nroFrame = frameDisponible;
            payload = (int) payload + menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
            payLoadYaGuardado = (int) payLoadYaGuardado + menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
            nuevoElemento->segmentoOPagina = list_add(tablaCorrespondiente->tablaDePaginas, nuevaPagina);
            nuevoElemento->offsetEnPagina = 0;
            nuevoElemento->tamanio = tamPayload;
            nuevoElemento->ID = idElemento;
            list_add(listaElementos,nuevoElemento);
            paginaReemplazable->nroPagina = nuevoElemento->segmentoOPagina;
            bitarrayMemoria[frameDisponible] = 1;
            ////log_info(logger,"Uso de la pagina que pushea: %d",paginaReemplazable->uso);
            queue_push(tablaDeFrames, paginaReemplazable);
            ////log_info(logger2,"guardo la pagina %d del proceso %d",paginaReemplazable->nroPagina,paginaReemplazable->PID);
            //loggearTablaDeFrames();
            while (payLoadYaGuardado < tamPayload) {
                paginaParaReemplazar_struct *paginaReemplazable2 = malloc(sizeof(paginaParaReemplazar_struct));
                paginaReemplazable2->uso=1;
                paginaReemplazable2->PID=pid;
                int frameDisponible = encontrarFrameDisponible();
                if (frameDisponible == -1) {
                	frameDisponible = sacarPaginaDeMemoria();
                	//guardar_en_swap(payload, idElemento, tamPayload-payLoadYaGuardado, pid, tipo);
                    //break;
                }
					int *direccionFisica = memoria + (frameDisponible * tamPagina);
					memcpy(direccionFisica, payload, menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina));
					paginaEnTabla_struct *nuevaPagina = malloc(sizeof(paginaEnTabla_struct));
					nuevaPagina->frame = frameDisponible;
					nuevaPagina->presencia = 1;
					nuevaPagina->espacioOcupado = menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
					paginaReemplazable2->nroFrame = frameDisponible;
					payload += menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
					payLoadYaGuardado += menorEntreDos(tamPayload - payLoadYaGuardado, tamPagina);
					paginaReemplazable2->nroPagina = list_add(tablaCorrespondiente->tablaDePaginas, nuevaPagina);
					////log_info(logger,"Uso de la pagina que pushea: %d",paginaReemplazable2->uso);
					queue_push(tablaDeFrames, paginaReemplazable2);
					bitarrayMemoria[frameDisponible] = 1;
					////log_info(logger2,"guardo la pagina %d del proceso %d",paginaReemplazable2->nroPagina,paginaReemplazable2->PID);
					//loggearTablaDeFrames();

            }
    }
    loggearTablaDeFrames();
}


void guardar_en_swap(void* payload,int idElemento,int tamPayload,int pid,char tipo){
    int indicePaginaCorrespondiente;
    int indiceTablaCorrespondiente;

    elementoEnLista_struct *nuevoElemento= malloc(sizeof(elementoEnLista_struct));
    nuevoElemento->tipo = 'M';
    nuevoElemento->PID = pid;
    for(int i=0;i<list_size(listaElementos);i++){
    	elementoEnLista_struct *elementoIterante= malloc(sizeof(elementoEnLista_struct));
    	elementoIterante = list_get(listaElementos,i);
    	if((elementoIterante->ID == idElemento) && (elementoIterante->tipo == tipo)){
    		nuevoElemento = elementoIterante;
    		break;
    	}
    }




    tablaEnLista_struct *tablaCorrespondiente = malloc(sizeof(tablaEnLista_struct));

    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaEnLista_struct *tablaIterante;
        tablaIterante = list_get(listaDeTablasDePaginas,i);
        if(tablaIterante->idPatota == pid){
            tablaCorrespondiente = tablaIterante;
            indiceTablaCorrespondiente = i;
            break;
        }

    }

    paginaEnTabla_struct *paginaParcialmenteLlena = malloc(sizeof (paginaEnTabla_struct));
    paginaParcialmenteLlena->frame = -1;
    for (int i = 0; i < list_size(tablaCorrespondiente->tablaDePaginas); ++i) {
        paginaEnTabla_struct *paginaIterante;
        paginaIterante = list_get(tablaCorrespondiente->tablaDePaginas,i);
        if(paginaIterante->espacioOcupado < tamPagina && paginaIterante->presencia==0){
        	paginaParcialmenteLlena = paginaIterante;
            indicePaginaCorrespondiente = i;
            break;
        }

    }

    int payLoadYaGuardado = 0;
    int *direccionFisica;

    if (paginaParcialmenteLlena->frame != -1){

        direccionFisica = (int)memoriaSwap + (int)(paginaParcialmenteLlena->frame * tamPagina + paginaParcialmenteLlena->espacioOcupado);
        int menorEntre2 = menorEntreDos(tamPayload,(tamPagina-paginaParcialmenteLlena->espacioOcupado));
        memcpy(direccionFisica,payload,menorEntre2);
        if(nuevoElemento->tipo=='M'){
        	nuevoElemento->tipo = tipo;
        	nuevoElemento->offsetEnPagina = paginaParcialmenteLlena->espacioOcupado;
        	nuevoElemento->segmentoOPagina = indicePaginaCorrespondiente;
        	nuevoElemento->tamanio = tamPayload;
        	nuevoElemento->ID = idElemento;
        	list_add(listaElementos,nuevoElemento);
        }

        payLoadYaGuardado += menorEntreDos(tamPayload,tamPagina-paginaParcialmenteLlena->espacioOcupado);
        payload = (int)payload + menorEntreDos(tamPayload,tamPagina-paginaParcialmenteLlena->espacioOcupado);
        paginaParcialmenteLlena->espacioOcupado += menorEntreDos(tamPayload,tamPagina - paginaParcialmenteLlena->espacioOcupado);
        list_replace(tablaCorrespondiente->tablaDePaginas,indicePaginaCorrespondiente,paginaParcialmenteLlena);
        while (payLoadYaGuardado<tamPayload) {
            int frameDisponible = encontrarFrameEnSwapDisponible();
            direccionFisica = (int)memoriaSwap + (int)(frameDisponible * tamPagina);
            menorEntre2 = menorEntreDos(tamPagina,tamPayload-payLoadYaGuardado);
            memcpy(direccionFisica,payload, menorEntre2);
            paginaEnTabla_struct *nuevaPagina = malloc(sizeof (paginaEnTabla_struct));
            nuevaPagina->frame = frameDisponible;
            nuevaPagina->presencia = 0;
            nuevaPagina->espacioOcupado = menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payLoadYaGuardado = (int)payLoadYaGuardado + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payload = (int)payload + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            list_add(tablaCorrespondiente->tablaDePaginas,nuevaPagina);
            bitarraySwap[frameDisponible] = 1;
        }

    }
    else{
        int frameDisponible = encontrarFrameEnSwapDisponible();
        int *direccionFisica =(int) memoriaSwap + (frameDisponible * tamPagina);
        char* payloadProbando;
        payloadProbando=payload;
        //printf("Payload la concha de tu madre: %s\n",payloadProbando);
        memcpy(direccionFisica,payload, menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina));
        paginaEnTabla_struct *nuevaPagina = malloc(sizeof (paginaEnTabla_struct));
        nuevaPagina->frame = frameDisponible;
        nuevaPagina->presencia = 0;
        nuevaPagina->espacioOcupado = menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
        payload =(int)payload + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
        payLoadYaGuardado =(int)payLoadYaGuardado + menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
        if(nuevoElemento->tipo == 'M'){
        	nuevoElemento->tipo = tipo;
        	nuevoElemento->segmentoOPagina = list_add(tablaCorrespondiente->tablaDePaginas,nuevaPagina);
        	nuevoElemento->offsetEnPagina=0;
        	nuevoElemento->tamanio = tamPayload;
        	nuevoElemento->ID = idElemento;
        	list_add(listaElementos,nuevoElemento);
        }

        bitarraySwap[frameDisponible] = 1;

        while (payLoadYaGuardado<tamPayload) {

        	int frameDisponible = encontrarFrameEnSwapDisponible();
            int *direccionFisica = (int) memoriaSwap + (frameDisponible * tamPagina);
            memcpy(direccionFisica,payload, menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina));
            paginaEnTabla_struct *nuevaPagina = malloc(sizeof (paginaEnTabla_struct));
            nuevaPagina->frame = frameDisponible;
            nuevaPagina->presencia=0;
            nuevaPagina->espacioOcupado = menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payload +=menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            payLoadYaGuardado += menorEntreDos(tamPayload-payLoadYaGuardado,tamPagina);
            list_add(tablaCorrespondiente->tablaDePaginas,nuevaPagina);
            bitarraySwap[frameDisponible] = 1;
        }

    }
    msync(memoriaSwap, tamSwap, MS_SYNC);
}




void* minimo_segmentos_en_tabla(void *elem1,void*elem2){
    segmentoEnTabla_struct *elem1bis = elem1;
    segmentoEnTabla_struct *elem2bis = elem2;
    if(elem1bis->inicio < elem2bis->inicio){
        return elem1bis;
    } else{
        return elem2bis;
    }

}
void* minimo_hueco_libre(void *elem1,void*elem2){
    espacio_struct *elem1bis = elem1;
    espacio_struct *elem2bis = elem2;
    if(elem1bis->tamanio < elem2bis->tamanio){
        return elem1bis;
    } else{
        return elem2bis;
    }

}
void* minimo_hueco_libre_mejorado(void *elem1,void*elem2){
	huecoLibreEnLista_struct *elem1bis = elem1;
	huecoLibreEnLista_struct *elem2bis = elem2;
    if(elem1bis->tamanio < elem2bis->tamanio){
        return elem1bis;
    } else{
        return elem2bis;
    }

}
bool ordenar_por_posicion_local(void *tam1, void *tam2){
    segmentoEnTabla_struct *tam1bis = tam1;
    segmentoEnTabla_struct *tam2bis = tam2;
    return (tam1bis->inicio < tam2bis->inicio);

}

bool ordenar_por_posicion_global(void *tam1, void *tam2){
    segmentoEnTablaGlobal_struct *tam1bis = tam1;
    segmentoEnTablaGlobal_struct  *tam2bis = tam2;
    return (tam1bis->inicio < tam2bis->inicio);

}

bool ordenar_por_nro_frame(void *pag1, void *pag2){
    paginaParaReemplazar_struct *pag1bis = pag1;
    paginaParaReemplazar_struct  *pag2bis = pag2;
    return (pag1bis->nroFrame < pag2bis->nroFrame);

}

bool filtrarPorTipo(void* elemento){
    elementoEnLista_struct *comparador = elemento;
    return comparador->tipo == tipoUniversal;
}
bool filtrarPorTamanioValido(void* elemento){
	huecoLibreEnLista_struct *comparador = elemento;
    return comparador->tamanio >= tamPayloadUniversal;
}
void traerPaginaAMemoria(paginaEnTabla_struct* paginaATraer, t_list* tablaDePaginas,int indiceDeLaPaginaATraer,int PID){
	log_info(logger2,"Tengo que traer la pagina: %d del proceso: %d",indiceDeLaPaginaATraer, PID);
	loggearTablaDeFrames();
	int frameDisponible = encontrarFrameDisponible();
	if(frameDisponible != -1){
		int* direccionFisicaPaginaEnSwap = (int)memoriaSwap + (paginaATraer->frame * tamPagina);
		int* direccionFisicaPaginaEnMemoria = (int) memoria + (frameDisponible * tamPagina);
		memcpy(direccionFisicaPaginaEnMemoria,direccionFisicaPaginaEnSwap,tamPagina);
		paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
		paginaAActualizar->presencia = 1;
		paginaAActualizar->frame = frameDisponible;
		paginaAActualizar->espacioOcupado = paginaATraer->espacioOcupado;
		list_replace(tablaDePaginas,indiceDeLaPaginaATraer,paginaAActualizar);
		paginaParaReemplazar_struct *paginaAReponer = malloc(sizeof(paginaParaReemplazar_struct));
		paginaAReponer->nroFrame = frameDisponible;
		paginaAReponer->nroPagina = indiceDeLaPaginaATraer; //list_add(tablaDePaginas,paginaAActualizar);
		paginaAReponer->uso = 1;
		paginaAReponer->PID = PID;
		queue_push(tablaDeFrames,paginaAReponer);
		bitarrayMemoria[frameDisponible]=1;
		//bitarraySwap[paginaATraer->frame]=0;
		log_info(logger, "Eligio como 'Victima' una pagina libre");
		//log_info(logger2,"Meto la pagina %d del proceso en un lugar libre",paginaAReponer->nroPagina,paginaAReponer->PID);
		//loggearTablaDeFrames();
	}else{
		int frameEnSwap = paginaATraer->frame;
		paginaParaReemplazar_struct *paginaAReemplazar = malloc(sizeof(paginaParaReemplazar_struct));
		if (strcmp(alg_remplazo,"LRU")==0){
			paginaAReemplazar = queue_pop(tablaDeFrames);
		} else{
			list_sort(tablaDeFrames->elements,ordenar_por_nro_frame);
			for (int i=0;i<list_size(tablaDeFrames->elements);i++){
				paginaParaReemplazar_struct *paginaDePrueba = malloc(sizeof(paginaParaReemplazar_struct));
				paginaDePrueba = list_get(tablaDeFrames->elements,i);
				//log_info(logger, "Bit de uso de la pagina %d de la patota %d: %d",paginaDePrueba->nroPagina,paginaDePrueba->PID,paginaDePrueba->uso);
			}
			while (1) {
				paginaAReemplazar = list_get(tablaDeFrames->elements, punteroReemplazo);
				if (paginaAReemplazar->uso==1){
					paginaAReemplazar->uso = 0;
					list_replace(tablaDeFrames->elements,punteroReemplazo,paginaAReemplazar);
					if (punteroReemplazo+1 == queue_size(tablaDeFrames)){
						punteroReemplazo = 0;
					} else{
						punteroReemplazo++;
					}
				} else{
					list_remove(tablaDeFrames->elements,punteroReemplazo);
					if(punteroReemplazo == queue_size(tablaDeFrames)){
						punteroReemplazo = 0;
					}
					break;
				}
			}

			//loggearTablaDeFrames();
		}
		//log_info(logger, "Pagina elegida como victima: %d del proceso %d",paginaAReemplazar->nroPagina,paginaAReemplazar->PID);
		int* direccionFisicaPaginaEnSwap = (int)memoriaSwap + (paginaATraer->frame * tamPagina);
		int* direccionFisicaPaginaEnMemoria = (int) memoria + (paginaAReemplazar->nroFrame * tamPagina);
		void* direccionAuxiliar = malloc(tamPagina);
		memcpy(direccionAuxiliar,direccionFisicaPaginaEnMemoria,tamPagina);
		memcpy(direccionFisicaPaginaEnMemoria,direccionFisicaPaginaEnSwap,tamPagina);
		memcpy(direccionFisicaPaginaEnSwap,direccionAuxiliar,tamPagina);
		paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
		paginaAActualizar = list_get(tablaDePaginas,indiceDeLaPaginaATraer);
		paginaAActualizar->presencia = 1;
		paginaAActualizar->frame = paginaAReemplazar->nroFrame;
		list_replace(tablaDePaginas,indiceDeLaPaginaATraer,paginaAActualizar);
		paginaParaReemplazar_struct *paginaAReponer = malloc(sizeof(paginaParaReemplazar_struct));
		paginaAReponer->nroFrame = paginaAActualizar->frame;
		paginaAReponer->nroPagina = indiceDeLaPaginaATraer;
		paginaAReponer->uso = 1;
		paginaAReponer->PID = PID;
		queue_push(tablaDeFrames,paginaAReponer);
		tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
		t_list *tablaDePaginasBuscada;
		for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
			tablaBuscada = list_get(listaDeTablasDePaginas,i);
			if (tablaBuscada->idPatota == paginaAReemplazar->PID){
				tablaDePaginasBuscada = tablaBuscada->tablaDePaginas;
				break;
			}
		}
    paginaEnTabla_struct *paginaAActualizar2 = malloc(sizeof(paginaEnTabla_struct));
    paginaAActualizar2 = list_get(tablaDePaginasBuscada,paginaAReemplazar->nroPagina);
    paginaAActualizar2->presencia = 0;
    paginaAActualizar2->frame = frameEnSwap;
    list_replace(tablaDePaginasBuscada,paginaAReemplazar->nroPagina,paginaAActualizar2);
    }
	loggearTablaDeFrames();
}

void* buscar_en_memoria_paginacion(int idElementoABuscar,int PID, char tipo){
    loggearTablaDeFrames();
	elementoEnLista_struct *elementoEvaluado = malloc(sizeof(elementoEnLista_struct));
    int paginaInicial= -1,offset=-1,tamanioPayload=-1;

    for (int i = 0; i < list_size(listaElementos); ++i) {
        elementoEvaluado = list_get(listaElementos,i);
        if (elementoEvaluado->ID == idElementoABuscar && elementoEvaluado->tipo==tipo && elementoEvaluado->PID == PID){
            paginaInicial = elementoEvaluado->segmentoOPagina;
            offset = elementoEvaluado->offsetEnPagina;
            tamanioPayload = elementoEvaluado->tamanio;
            //log_info(logger,"Tamanio del payload que va a buscar: %d",tamanioPayload);
            break;
        }
    }
    if (paginaInicial == -1){
    	//log_info(logger,"No encontre el elemento buscado");
    	return 0;
    }
    tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
    t_list *tablaDePaginas;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaBuscada = list_get(listaDeTablasDePaginas,i);
        if (tablaBuscada->idPatota == PID){
            tablaDePaginas = tablaBuscada->tablaDePaginas;
            break;
        }
    }

    void* payloadADevolver = malloc(tamanioPayload);
    //log_info(logger,"Payload en el malloc %d de tamanio %d",payloadADevolver,tamanioPayload);
    int tamanioPorGuardar = tamanioPayload;
    paginaEnTabla_struct *paginaDeLectura = malloc(sizeof(paginaEnTabla_struct));
    int movimientoDepagina=1;
    paginaDeLectura = list_get(tablaDePaginas,paginaInicial);
    if (paginaDeLectura->presencia == 0){
    	log_info(logger,"Debo traer la pagina %d del proceso %d",paginaInicial,PID);
    	////loggearTablaDeFrames();
    	traerPaginaAMemoria(paginaDeLectura,tablaDePaginas,paginaInicial,PID);
        msync(memoriaSwap, tamSwap, MS_SYNC);
        paginaDeLectura = list_get(tablaDePaginas,paginaInicial);
    }else if(paginaDeLectura->presencia == 1){
        paginaParaReemplazar_struct *paginaAux = malloc(sizeof(paginaParaReemplazar_struct));
        for(int j=0;j<list_size(tablaDeFrames->elements);j++){
        	paginaParaReemplazar_struct *paginaIterante = malloc(sizeof(paginaParaReemplazar_struct ));
        	paginaIterante = list_get(tablaDeFrames->elements,j);
        	if(paginaIterante->nroPagina == paginaInicial){
        		paginaAux = list_remove(tablaDeFrames->elements,j);
        		paginaAux->uso = 1;
        		log_info(logger2,"Le puse el bit de uso en 1 a la pagina: %d del proceso :%d", paginaInicial,PID);
        		queue_push(tablaDeFrames,paginaAux);
        		break;
        	}
        }
    }
    //log_info(logger,"Voy a buscar un tipo %c que esta en el frame %d y tiene tamanio %d con offset %d",tipo,paginaDeLectura->frame,elementoEvaluado->tamanio,elementoEvaluado->offsetEnPagina);
    int* direccionFisicaDeLaPagina;
    direccionFisicaDeLaPagina = (int)memoria + (paginaDeLectura->frame * tamPagina + offset);
    memcpy(payloadADevolver,direccionFisicaDeLaPagina, menorEntreDos(tamanioPayload,tamPagina-offset));
    //char* prueba = malloc(9);
    //memcpy(prueba,direccionFisicaDeLaPagina, menorEntreDos(tamanioPayload,tamPagina-offset));
    //log_info(logger,"Tarea que sale del memcpy: %s",prueba);
    payloadADevolver = (int) payloadADevolver + menorEntreDos(tamanioPayload,tamPagina-offset);
    tamanioPorGuardar -= menorEntreDos(tamanioPayload,tamPagina-offset);
    //log_info(logger,"Tamanio por guardar: %d",tamanioPorGuardar);
    while(tamanioPorGuardar>0){
        paginaDeLectura = list_get(tablaDePaginas,paginaInicial+movimientoDepagina);
        if (paginaDeLectura->presencia == 0){
        	log_info(logger,"Debo traer la pagina %d del proceso %d",paginaInicial,PID);
        	//loggearTablaDeFrames();
        	traerPaginaAMemoria(paginaDeLectura,tablaDePaginas, paginaInicial+movimientoDepagina,PID);
            msync(memoriaSwap, tamSwap, MS_SYNC);
            paginaDeLectura = list_get(tablaDePaginas,paginaInicial+movimientoDepagina);
        }else{
        	paginaParaReemplazar_struct *paginaAux = malloc(sizeof(paginaParaReemplazar_struct));
        	for(int j=0;j<list_size(tablaDeFrames->elements);j++){
        		paginaParaReemplazar_struct *paginaIterante = malloc(sizeof(paginaParaReemplazar_struct ));
        		paginaIterante = list_get(tablaDeFrames->elements,j);
        		if(paginaIterante->nroPagina == paginaInicial){
        			paginaAux = list_remove(tablaDeFrames->elements,j);
        			paginaAux->uso = 1;
        			queue_push(tablaDeFrames,paginaAux);
        			break;
        		}
        	}

        }
        direccionFisicaDeLaPagina = (int)memoria + (paginaDeLectura->frame * tamPagina);
        memcpy(payloadADevolver,direccionFisicaDeLaPagina, menorEntreDos(tamanioPorGuardar,tamPagina));
        payloadADevolver += menorEntreDos(tamanioPorGuardar,tamPagina);
        tamanioPorGuardar -= menorEntreDos(tamanioPorGuardar,tamPagina);
        movimientoDepagina ++;
        //log_info(logger,"Tamanio por guardar: %d",tamanioPorGuardar);
    }
    //log_info(logger,"Posicion del payload a devolver: %d",payloadADevolver);
    payloadADevolver = (int)payloadADevolver - tamanioPayload;
    //log_info(logger,"Payload en el buscar: %s",(char*)payloadADevolver);
    //log_info(logger,"Posicion del payload a devolver: %d",payloadADevolver);

    loggearTablaDeFrames();
    return payloadADevolver;

}
void actualizarListaElementos(int paginaEliminada,int PID){
    for (int i = 0; i < list_size(listaElementos); ++i){
        elementoEnLista_struct *elementoIterante = malloc(sizeof(elementoEnLista_struct));
        elementoIterante = list_get(listaElementos,i);
        if((elementoIterante->segmentoOPagina > paginaEliminada) && (elementoIterante->PID == PID)){
        	elementoIterante->segmentoOPagina -= 1;
        	//log_info("Tipo %c y tamanio %d",elementoIterante->tipo,elementoIterante->tamanio);
        	list_replace(listaElementos,i,elementoIterante);
        }
        //free(elementoIterante);
    }

}

void actualizarTablaDeFrames(int paginaEliminada,int PID){
    for (int i = 0; i < list_size(tablaDeFrames->elements); ++i){
    	paginaParaReemplazar_struct *elementoIterante = malloc(sizeof(paginaParaReemplazar_struct));
        elementoIterante = list_get(tablaDeFrames->elements,i);
        if((elementoIterante->nroPagina > paginaEliminada) && (elementoIterante->PID == PID)){
        	elementoIterante->nroPagina -= 1;
        	list_replace(tablaDeFrames->elements,i,elementoIterante);
        }

    }
}


void actualizarListaGlobalDeSegmentos(int paginaEliminada,int PID){
    for (int i = 0; i < list_size(listaGlobalDeSegmentos); ++i) {
        segmentoEnTablaGlobal_struct *segmentoIterante = malloc(sizeof(segmentoEnTablaGlobal_struct));
        		segmentoIterante = list_get(listaGlobalDeSegmentos,i);
        if((segmentoIterante->segmentoEnLocal > paginaEliminada) && (segmentoIterante->idPatota == PID)){
            segmentoIterante->segmentoEnLocal -= 1;
            list_replace(listaGlobalDeSegmentos,i,segmentoIterante);
        }
        //free(segmentoIterante);
    }
}

void *borrar_de_memoria_paginacion(int idElemento, int idPatota, char tipo){
	tipoUniversal = tipo;
    bool borroPagina=false;
	t_list* listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
    elementoEnLista_struct *elementoEvaluado = malloc(sizeof(elementoEnLista_struct));

    int paginaInicial,offset,tamanioPayload,posicionElementoEvaluado;
    for (int i = 0; i < list_size(listaElementos); ++i) {
    	elementoEvaluado = list_get(listaElementos,i);
        if (elementoEvaluado->ID == idElemento && elementoEvaluado->tipo==tipo){
        	paginaInicial = elementoEvaluado->segmentoOPagina;
            offset = elementoEvaluado->offsetEnPagina;
            tamanioPayload = elementoEvaluado->tamanio;
            posicionElementoEvaluado = i;

        }
    }
    //free(elementoEvaluado);
    espacioLibre += tamanioPayload;
    tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
    t_list *tablaDePaginas;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaBuscada = list_get(listaDeTablasDePaginas,i);
        if (tablaBuscada->idPatota == idPatota){
            tablaDePaginas = tablaBuscada->tablaDePaginas;
            break;
        }
    }
    //free(tablaBuscada);
    int payloadBorrado=0;
    paginaEnTabla_struct* paginaABorrarEnPrimerCaso = malloc(sizeof(paginaEnTabla_struct));
    paginaABorrarEnPrimerCaso = list_get(tablaDePaginas, paginaInicial);
    if ((offset == 0 && tamanioPayload >= tamPagina) || (offset == 0 && paginaABorrarEnPrimerCaso->espacioOcupado==tamanioPayload)){
    	if (paginaABorrarEnPrimerCaso->presencia == 1){
    		bitarrayMemoria[paginaABorrarEnPrimerCaso->frame] = 0;
    	}else{
    		bitarraySwap[paginaABorrarEnPrimerCaso->frame] = 0;
    	}

    	list_remove(tablaDePaginas,paginaInicial);
        actualizarTablaDeFrames(paginaInicial,idPatota);
        borroPagina=true;
        actualizarListaElementos(paginaInicial,idPatota);
        for(int i=0;i<list_size(tablaDeFrames->elements);i++){
        	paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
        	paginaABorrar = list_get(tablaDeFrames->elements,i);
        	if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
        		list_remove(tablaDeFrames->elements,i);
        		//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
        		//loggearTablaDeFrames();
        	}
        }
        list_remove(listaElementos,posicionElementoEvaluado);
        actualizarListaElementos(paginaInicial,idPatota);
        payloadBorrado += tamPagina;
        actualizarListaElementos(paginaInicial,idPatota);
        //free(paginaABorrarEnPrimerCaso);
    }else if(offset == 0 && tamanioPayload < tamPagina){
    	paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
    	paginaAActualizar =	list_get(tablaDePaginas,paginaInicial);
    	paginaAActualizar->espacioOcupado -= tamanioPayload;
    	if (paginaAActualizar->espacioOcupado == 0){
    		if (paginaAActualizar->presencia == 1){
    		   bitarrayMemoria[paginaAActualizar->frame] = 0;
    		}else{
    		   bitarraySwap[paginaAActualizar->frame] = 0;
    		}
    		list_remove(tablaDePaginas,paginaInicial);
    		actualizarTablaDeFrames(paginaInicial,idPatota);
    		borroPagina=true;
    		actualizarListaElementos(paginaInicial,idPatota);
    		for(int i=0;i<list_size(tablaDeFrames->elements);i++){
    			paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
    			paginaABorrar = list_get(tablaDeFrames->elements,i);

    			if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
    				list_remove(tablaDeFrames->elements,i);
    				//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
    				//loggearTablaDeFrames();

    			}
    		}
    	}else{
    	    list_replace(tablaDePaginas,paginaInicial,paginaAActualizar);
    	}
    	list_remove(listaElementos,posicionElementoEvaluado);
    	payloadBorrado += tamanioPayload;
    }
    else if(offset!=0 && tamanioPayload >= tamPagina-offset){
    	paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
        paginaAActualizar =	list_get(tablaDePaginas,paginaInicial);
        paginaAActualizar->espacioOcupado -= tamPagina-offset;
        if (paginaAActualizar->espacioOcupado == 0){
        	if (paginaAActualizar->presencia == 1){
        		bitarrayMemoria[paginaAActualizar->frame] = 0;

        	}else{
        		bitarraySwap[paginaAActualizar->frame] = 0;
        	}
        	list_remove(tablaDePaginas,paginaInicial);
        	actualizarTablaDeFrames(paginaInicial,idPatota);
        	borroPagina=true;
        	actualizarListaElementos(paginaInicial,idPatota);
        	for(int i=0;i<list_size(tablaDeFrames->elements);i++){
        		paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
        		paginaABorrar = list_get(tablaDeFrames->elements,i);

        		if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
        			list_remove(tablaDeFrames->elements,i);
        			//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
        			//loggearTablaDeFrames();
        		}
        	}
        }else{
        	list_replace(tablaDePaginas,paginaInicial,paginaAActualizar);
        }

        list_remove(listaElementos,posicionElementoEvaluado);
        payloadBorrado += (tamPagina-offset);
    }else if(offset!=0 && tamanioPayload < tamPagina-offset){
    	paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
    	paginaAActualizar = list_get(tablaDePaginas,paginaInicial);
    	paginaAActualizar->espacioOcupado -= tamanioPayload;
    	if (paginaAActualizar->espacioOcupado == 0){
    		if (paginaAActualizar->presencia == 1){
    		   bitarrayMemoria[paginaAActualizar->frame] = 0;
    		}else{
    		   bitarraySwap[paginaAActualizar->frame] = 0;
    		}
    		list_remove(tablaDePaginas,paginaInicial);
    		actualizarTablaDeFrames(paginaInicial,idPatota);
    		borroPagina=true;
    		actualizarListaElementos(paginaInicial,idPatota);
    		for(int i=0;i<list_size(tablaDeFrames->elements);i++){
    			paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
    			paginaABorrar = list_get(tablaDeFrames->elements,i);
    			if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
    				list_remove(tablaDeFrames->elements,i);
    				//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
    				//loggearTablaDeFrames();
    			}
    		}

    	}else{
    		list_replace(tablaDePaginas,paginaInicial,paginaAActualizar);
    	}
    	payloadBorrado += tamanioPayload;
    	list_remove(listaElementos,posicionElementoEvaluado);
    }
    while(payloadBorrado < tamanioPayload){
    	if(!borroPagina){
    		paginaInicial++;
    	}else{
    		borroPagina = false;
    	}

    	paginaEnTabla_struct* paginaABorrarCasoUnoUno = malloc(sizeof(paginaEnTabla_struct));
    	paginaABorrarCasoUnoUno = list_get(tablaDePaginas, paginaInicial);
    	if ((tamanioPayload -  payloadBorrado >= tamPagina) || (paginaABorrarCasoUnoUno->espacioOcupado == (tamanioPayload-payloadBorrado))){
    		if (paginaABorrarCasoUnoUno->presencia == 1){
    			bitarrayMemoria[paginaABorrarCasoUnoUno->frame] = 0;
    		}else{
    			bitarraySwap[paginaABorrarCasoUnoUno->frame] = 0;
    		}

    		list_remove(tablaDePaginas,paginaInicial);
    		actualizarTablaDeFrames(paginaInicial,idPatota);
    		for(int i=0;i<list_size(tablaDeFrames->elements);i++){
    			paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
    			paginaABorrar = list_get(tablaDeFrames->elements,i);
    			if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
    				list_remove(tablaDeFrames->elements,i);
    				//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
    				//loggearTablaDeFrames();
    			}
    		}
    		borroPagina=true;
    		payloadBorrado += tamPagina;
    		actualizarListaElementos(paginaInicial,idPatota);
    		//free(paginaABorrarCasoUnoUno);
    	}else if(tamanioPayload -  payloadBorrado < tamPagina){
    		paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
        	paginaAActualizar =	list_get(tablaDePaginas,paginaInicial);
        	paginaAActualizar->espacioOcupado -= (tamanioPayload-payloadBorrado);
        	if (paginaAActualizar->espacioOcupado == 0){
        		if (paginaAActualizar->presencia == 1){
        		   bitarrayMemoria[paginaAActualizar->frame] = 0;

        		}else{
        		   bitarraySwap[paginaAActualizar->frame] = 0;
        		}
        		list_remove(tablaDePaginas,paginaInicial);
        		actualizarTablaDeFrames(paginaInicial,idPatota);
        		borroPagina=true;
        		actualizarListaElementos(paginaInicial,idPatota);
        		for(int i=0;i<list_size(tablaDeFrames->elements);i++){
        			paginaParaReemplazar_struct* paginaABorrar = malloc(sizeof(paginaParaReemplazar_struct));
        			paginaABorrar = list_get(tablaDeFrames->elements,i);
        			if(paginaABorrar->PID==idPatota && paginaABorrar->nroPagina==paginaInicial){
        				list_remove(tablaDeFrames->elements,i);
        				//log_info(logger2,"Borro la pagina %d del proceso %d",paginaABorrar->nroPagina,paginaABorrar->PID);
        				//loggearTablaDeFrames();
        			}
        		}
        	}else{
        	    list_replace(tablaDePaginas,paginaInicial,paginaAActualizar);
        	}
        	payloadBorrado += (tamanioPayload -  payloadBorrado);
        }

    }
}

void guardar_en_memoria_segmentacion(void* payload,int idElemento,int tamPayload,uint32_t pid,char tipo, int tipoDeGuardado)
{
    int huecoLibre;
    t_list *listaSegmentos;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaEnLista_struct *tablaIterante = malloc(sizeof(tablaEnLista_struct));
        tablaIterante = list_get(listaDeTablasDePaginas,i);
        if (tablaIterante->idPatota == pid){
            listaSegmentos=tablaIterante->tablaDePaginas;
        }
        //free(tablaIterante);
    }


    if (list_is_empty(listaGlobalDeSegmentos) == 1)
    {
        memcpy(memoria,payload,tamPayload);
        segmentoEnTabla_struct *nuevoSegmento = malloc(sizeof(segmentoEnTabla_struct));
        segmentoEnTablaGlobal_struct *nuevoSegmentoGlobal = malloc(sizeof(segmentoEnTablaGlobal_struct));
        nuevoSegmentoGlobal->inicio=memoria;
        nuevoSegmentoGlobal->tamanio=tamPayload;
        nuevoSegmentoGlobal->idPatota=pid;
        nuevoSegmento->inicio = memoria;
        log_info(logger,"Guarda el tripulante %d de la patota %d en la posicion %d",idElemento,pid,memoria);
        nuevoSegmento->tamanio = tamPayload;
        elementoEnLista_struct *elementoNuevo = malloc(sizeof(elementoEnLista_struct));
        int segmentoGuardado = list_add_sorted(listaSegmentos,nuevoSegmento,ordenar_por_posicion_local);
        elementoNuevo->segmentoOPagina = segmentoGuardado;
        nuevoSegmentoGlobal->segmentoEnLocal = segmentoGuardado;
        list_add_sorted(listaGlobalDeSegmentos,nuevoSegmentoGlobal,ordenar_por_posicion_global);
        elementoNuevo->offsetEnPagina=0;
        elementoNuevo->tipo = tipo;
        elementoNuevo->tamanio = tamPayload;
        elementoNuevo->ID = idElemento;
        elementoNuevo->PID=pid;
        list_add(listaElementos,elementoNuevo);
        if (tipoDeGuardado == BESTFIT){
        	huecoLibreEnLista_struct* mejorEspacio = list_get(listaHuecosLibres,0);
        	int *posicionHuecoLibre = mejorEspacio->inicio;
        	if(mejorEspacio->tamanio == tamPayload){
        		for (int i = 0; i < list_size(listaHuecosLibres); ++i) {
        			huecoLibreEnLista_struct *huecoIterante = list_get(listaHuecosLibres,i);
        			if(huecoIterante->inicio == mejorEspacio->inicio){
        				list_remove(listaHuecosLibres,i);
        				break;
        			}
        		}
        	}else if (mejorEspacio->tamanio > tamPayload) {
        		//log_info(logger,"Entra al if");
        		for (int i = 0; i < list_size(listaHuecosLibres); ++i) {
        			huecoLibreEnLista_struct *huecoIterante = list_get(listaHuecosLibres,i);
        			if(huecoIterante->inicio == mejorEspacio->inicio){
        				mejorEspacio->inicio = (int) mejorEspacio->inicio + tamPayload;
        				mejorEspacio->tamanio -= tamPayload;
        				list_replace(listaHuecosLibres,i,mejorEspacio);
        			}
        		}
        	}
        }
    }
    else
    {
        int tamanioListaSegmentos = list_size(listaGlobalDeSegmentos);
        switch (tipoDeGuardado) {
            case FIRSTFIT: {
            	segmentoEnTablaGlobal_struct* segmentoGlobalAComparar = malloc(sizeof(segmentoEnTablaGlobal_struct));
            	segmentoGlobalAComparar = list_get(listaGlobalDeSegmentos, 0);
            	if (segmentoGlobalAComparar->inicio != memoria){
                    huecoLibre = (int)segmentoGlobalAComparar->inicio - (int)memoria;
                    if (huecoLibre >= tamPayload){
                        segmentoEnTabla_struct *nuevoSegmento = malloc(sizeof(segmentoEnTabla_struct));
                        segmentoEnTablaGlobal_struct *nuevoSegmentoGlobal = malloc(sizeof(segmentoEnTablaGlobal_struct));
                        nuevoSegmentoGlobal->inicio=memoria;
                        nuevoSegmentoGlobal->tamanio=tamPayload;
                        nuevoSegmentoGlobal->idPatota=pid;
                        nuevoSegmento->inicio = memoria;
                        log_info(logger,"Guarda el tripulante %d de la patota %d en la posicion %d",idElemento,pid,memoria);
                        nuevoSegmento->tamanio = tamPayload;
                        log_info(logger,"Guarda el tripulante %d de la patota %d en la posicion %d",idElemento,pid,memoria);
                        elementoEnLista_struct *elementoNuevo = malloc(sizeof(elementoEnLista_struct));
                        elementoNuevo->segmentoOPagina = list_add_sorted(listaSegmentos,nuevoSegmento,ordenar_por_posicion_local);
                        nuevoSegmentoGlobal->segmentoEnLocal = elementoNuevo->segmentoOPagina;
                        list_add_sorted(listaGlobalDeSegmentos,nuevoSegmentoGlobal,ordenar_por_posicion_global);
                        elementoNuevo->offsetEnPagina=0;
                        elementoNuevo->tipo = tipo;
                        elementoNuevo->tamanio = tamPayload;
                        elementoNuevo->ID = idElemento;
                        elementoNuevo->PID = pid;
                        actualizar_lista_elementos_segmentacion(elementoNuevo->segmentoOPagina,pid);
                        list_add(listaElementos,elementoNuevo);
                        break;
                    }

                }
                for (int i = 0; i < tamanioListaSegmentos; i++) {
                    segmentoEnTablaGlobal_struct *segmentoIterante = malloc(sizeof(segmentoEnTablaGlobal_struct));
                    segmentoIterante = list_get(listaGlobalDeSegmentos, i);
                    if (i + 1 == (list_size(listaGlobalDeSegmentos))) {

                        huecoLibre = ((int)memoria + tamMemoria) -  ((int)segmentoIterante->inicio + segmentoIterante->tamanio);
                    } else{

                        segmentoEnTablaGlobal_struct *segmentoSiguiente = list_get(listaGlobalDeSegmentos, i + 1);
                        huecoLibre =((int)segmentoSiguiente->inicio) - ((int)segmentoIterante->inicio + segmentoIterante->tamanio);
                    }

                    if (tamPayload <= huecoLibre) {

                        int *posicionInicioHuecoLibre = (int)(segmentoIterante->inicio) + (segmentoIterante->tamanio);
                        //*(tripulante_struct *) posicionInicioHuecoLibre = tcb;
                        memcpy(posicionInicioHuecoLibre,payload,tamPayload);
                        segmentoEnTabla_struct *nuevoSegmento = malloc(sizeof(segmentoEnTabla_struct));
                        segmentoEnTablaGlobal_struct *nuevoSegmentoGlobal = malloc(sizeof(segmentoEnTablaGlobal_struct));
                        nuevoSegmentoGlobal->inicio=posicionInicioHuecoLibre;
                        nuevoSegmentoGlobal->tamanio=tamPayload;
                        nuevoSegmentoGlobal->idPatota=pid;
                        nuevoSegmento->inicio = posicionInicioHuecoLibre;
                        nuevoSegmento->tamanio = tamPayload;
                        log_info(logger,"Guarda el tripulante %d de la patota %d en la posicion %d",idElemento,pid,posicionInicioHuecoLibre);
                        elementoEnLista_struct *elementoNuevo = malloc(sizeof(elementoEnLista_struct));
                        elementoNuevo->segmentoOPagina = list_add_sorted(listaSegmentos,nuevoSegmento,ordenar_por_posicion_local);
                        nuevoSegmentoGlobal->segmentoEnLocal = elementoNuevo->segmentoOPagina;
                        list_add_sorted(listaGlobalDeSegmentos,nuevoSegmentoGlobal,ordenar_por_posicion_global);
                        elementoNuevo->offsetEnPagina = 0;
                        elementoNuevo->tipo = tipo;
                        elementoNuevo->tamanio = tamPayload;
                        elementoNuevo->ID = idElemento;
                        elementoNuevo->PID = pid;
                        actualizar_lista_elementos_segmentacion(elementoNuevo->segmentoOPagina,pid);
                        list_add(listaElementos,elementoNuevo);
                        break;
                    }
                    //free(segmentoIterante);

                }
                break;
            }

            case BESTFIT:{
            	//log_info(logger,"Entra al case");
            	t_list *listaHuecosLibresFiltrada;
            	tamPayloadUniversal = tamPayload;
            	listaHuecosLibresFiltrada = list_filter(listaHuecosLibres,filtrarPorTamanioValido);
            	//log_info(logger,"Tamanio de la lista filtrada: %d y sin filtrar: %d",list_size(listaHuecosLibresFiltrada),list_size(listaHuecosLibres));
            	if (list_is_empty(listaHuecosLibresFiltrada) == 1){
            		//log_info(logger,"Entra a la compactacion");
            		compactacion();
            		listaHuecosLibresFiltrada = list_filter(listaHuecosLibres,filtrarPorTamanioValido);
            	}
            	huecoLibreEnLista_struct* mejorEspacio = list_get_minimum(listaHuecosLibresFiltrada,minimo_hueco_libre_mejorado);
            	int *posicionHuecoLibre = mejorEspacio->inicio;
            	if(mejorEspacio->tamanio == tamPayload){
            		for (int i = 0; i < list_size(listaHuecosLibres); ++i) {
            			huecoLibreEnLista_struct *huecoIterante = list_get(listaHuecosLibres,i);
            			if(huecoIterante->inicio == mejorEspacio->inicio){
            				//log_info(logger,"Tamanio del mejor lugar: %d y posicion: %d",mejorEspacio->tamanio,mejorEspacio->inicio);
            				list_remove(listaHuecosLibres,i);
            				break;
            			}
					}
            	}else if (mejorEspacio->tamanio > tamPayload) {
            		//log_info(logger,"Tamanio del mejor lugar: %d",mejorEspacio->tamanio);
            		for (int i = 0; i < list_size(listaHuecosLibres); ++i) {
            			huecoLibreEnLista_struct *huecoIterante = list_get(listaHuecosLibres,i);
            			if(huecoIterante->inicio == mejorEspacio->inicio){
            				mejorEspacio->inicio = (int) mejorEspacio->inicio + tamPayload;
            				mejorEspacio->tamanio -= tamPayload;
            				list_replace(listaHuecosLibres,i,mejorEspacio);
            			}
            		}
				}
                 /*if ((int)((segmentoEnTablaGlobal_struct *)(list_get(listaGlobalDeSegmentos, 0)))->inicio != memoria){
                	segmentoEnTablaGlobal_struct* segmentoIteranteSeg = list_get(listaGlobalDeSegmentos, 0);
                	huecoLibre = (int)segmentoIteranteSeg->inicio - (int)memoria;
                    if (huecoLibre >= tamPayload){
                        espacio_struct *nuevoHuecoLibre = malloc(sizeof (espacio_struct)) ;
                        nuevoHuecoLibre->tamanio = huecoLibre;
                        nuevoHuecoLibre->ptrHuecoLibre = memoria;
                        list_add(listaDeEspaciosLibres,nuevoHuecoLibre);

                    }
                }
                for (int i = 0; i < list_size(listaGlobalDeSegmentos); i++) {
                    segmentoEnTablaGlobal_struct *segmentoIterante;
                    segmentoIterante = list_get(listaGlobalDeSegmentos, i);

                    if (i + 1 == (list_size(listaGlobalDeSegmentos))) {

                        huecoLibre = ((int)memoria + tamMemoria) - ((int)segmentoIterante->inicio + segmentoIterante->tamanio);
                    } else {

                        segmentoEnTablaGlobal_struct *segmentoSiguiente = list_get(listaGlobalDeSegmentos, i + 1);
                        huecoLibre = (int)segmentoSiguiente->inicio - ((int)segmentoIterante->inicio + segmentoIterante->tamanio);
                    }

                    if (tamPayload <= huecoLibre) {
                        int *posicionInicioHuecoLibre = (int)segmentoIterante->inicio + segmentoIterante->tamanio;
                        espacio_struct *nuevoHuecoLibre = malloc(sizeof (espacio_struct)) ;
                        nuevoHuecoLibre->tamanio = huecoLibre;
                        nuevoHuecoLibre->ptrHuecoLibre = posicionInicioHuecoLibre;
                        list_add(listaDeEspaciosLibres,nuevoHuecoLibre);
                    }


                }*/


            	//espacio_struct *punteroHuecoMinimo;
            	//punteroHuecoMinimo = list_get_minimum(listaDeEspaciosLibres,minimo_hueco_libre);
            	//*(tripulante_struct *) punteroHuecoMinimo->ptrHuecoLibre = tcb;
            	//log_info(logger,"Posicion donde se hace el memcpy: %d",posicionHuecoLibre);
            	memcpy(posicionHuecoLibre,payload,tamPayload);
            	segmentoEnTabla_struct *nuevoSegmento = malloc(sizeof(segmentoEnTabla_struct));
            	segmentoEnTablaGlobal_struct *nuevoSegmentoGlobal = malloc(sizeof(segmentoEnTablaGlobal_struct));
            	nuevoSegmentoGlobal->inicio=posicionHuecoLibre;
            	nuevoSegmentoGlobal->tamanio=tamPayload;
            	nuevoSegmentoGlobal->idPatota=pid;
            	nuevoSegmento->inicio = posicionHuecoLibre;
            	log_info(logger,"Guarda el tripulante %d de la patota %d en la posicion %d",idElemento,pid,posicionHuecoLibre);
            	nuevoSegmento->tamanio = tamPayload;
            	elementoEnLista_struct *elementoNuevo = malloc(sizeof(elementoEnLista_struct));
            	int segmentoGuardado = list_add_sorted(listaSegmentos,nuevoSegmento,ordenar_por_posicion_local);
            	elementoNuevo->segmentoOPagina = segmentoGuardado;
            	nuevoSegmentoGlobal->segmentoEnLocal = elementoNuevo->segmentoOPagina;
            	list_add_sorted(listaGlobalDeSegmentos,nuevoSegmentoGlobal,ordenar_por_posicion_global);
            	elementoNuevo->offsetEnPagina=0;
            	elementoNuevo->tipo = tipo;
            	elementoNuevo->tamanio = tamPayload;
            	elementoNuevo->ID = idElemento;
            	elementoNuevo->PID = pid;
            	actualizar_lista_elementos_segmentacion(elementoNuevo->segmentoOPagina,pid);
            	list_add(listaElementos,elementoNuevo);


                //list_destroy(listaDeEspaciosLibres);
                break;
            }
        }
    }
}

void borrar_de_memoria_segmentacion(int idElementoABorrar, int idPatota, char tipoDeElemento){

    t_list *listaSegmentos;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaEnLista_struct *tablaIterante = malloc(sizeof(tablaEnLista_struct));
        tablaIterante = list_get(listaDeTablasDePaginas,i);
        if (tablaIterante->idPatota == idPatota){
            listaSegmentos=tablaIterante->tablaDePaginas;
        }
        //free(tablaIterante);
    }
    for (int i=0;i< list_size(listaElementos);i++){
        elementoEnLista_struct *elementoEvaluado = list_get(listaElementos,i);
        if (elementoEvaluado->PID == idPatota){
        	segmentoEnTabla_struct *segmentoEvaluado = list_get(listaSegmentos,elementoEvaluado->segmentoOPagina);
        	if ((elementoEvaluado->ID == idElementoABorrar) && (elementoEvaluado->tipo == tipoDeElemento)){
        		segmentoEnTablaGlobal_struct *segmentoGlobalIterante = malloc(sizeof(segmentoGlobalIterante));
                for (int j = 0; j < list_size(listaGlobalDeSegmentos); ++j) {
                	segmentoGlobalIterante = list_get(listaGlobalDeSegmentos,j);
                    if ((segmentoGlobalIterante->idPatota == idPatota) && (segmentoGlobalIterante->segmentoEnLocal == elementoEvaluado->segmentoOPagina)){
                    	huecoLibreEnLista_struct *nuevoHuecoLibre = malloc(sizeof(huecoLibreEnLista_struct));
                    	nuevoHuecoLibre->inicio = segmentoGlobalIterante->inicio;
                    	nuevoHuecoLibre->tamanio = segmentoGlobalIterante->tamanio;
                    	list_add(listaHuecosLibres,nuevoHuecoLibre);
                    	//log_info(logger,"Tamanio de la lista sin filtrar: %d",list_size(listaHuecosLibres));
                    	list_remove(listaGlobalDeSegmentos,j);
                        actualizarListaElementos(elementoEvaluado->segmentoOPagina,idPatota);
                        actualizarListaGlobalDeSegmentos(elementoEvaluado->segmentoOPagina,idPatota);
                        break;
                    }
                }
                list_remove(listaSegmentos,elementoEvaluado->segmentoOPagina);
                espacioLibre += elementoEvaluado->tamanio;
                list_remove(listaElementos,i);
                break;
            }

        }
    }

}

void *buscar_de_memoria_segmentacion(int idElementoABuscar,int idPatota, char tipoDeElemento){
    tipoUniversal = tipoDeElemento;
    t_list* listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
    t_list *listaSegmentos;
    tablaEnLista_struct *tablaIterante;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaIterante = list_get(listaDeTablasDePaginas,i);
        if (tablaIterante->idPatota == idPatota){
            listaSegmentos=tablaIterante->tablaDePaginas;
            break;
        }
        //free(tablaIterante);
    }
    elementoEnLista_struct *elementoEvaluado;
    segmentoEnTabla_struct *segmentoEvaluado;
    for (int s=0;s< list_size(listaFiltrada);s++){
        elementoEvaluado= list_get(listaFiltrada,s);
        segmentoEvaluado = list_get(listaSegmentos,elementoEvaluado->segmentoOPagina);
        if (tipoDeElemento == 'T'){
            tcb* elementoABuscar = malloc(sizeof(tcb));
            //elementoABuscar = (tripulante_struct*)segmentoEvaluado->inicio;
            memcpy(elementoABuscar,segmentoEvaluado->inicio, sizeof(tcb));
            if (elementoABuscar->id == idElementoABuscar && elementoEvaluado->tipo=='T'){
            	list_destroy(listaFiltrada);
            	//free(elementoABuscar);
            	return elementoABuscar;
            }
        }else if(tipoDeElemento == 'A'){
            char *elementoABuscar = malloc(elementoEvaluado->tamanio);
            if (elementoEvaluado->ID == idElementoABuscar && elementoEvaluado->tipo == 'A'){
                memcpy(elementoABuscar,segmentoEvaluado->inicio, elementoEvaluado->tamanio);
                //log_info(logger,"Direccion donde encontro las tareas: %d",segmentoEvaluado->inicio);
                list_destroy(listaFiltrada);
                //free(elementoABuscar);
                return elementoABuscar;

            }
        }else if(tipoDeElemento == 'P'){
            pcb *elementoABuscar = malloc(sizeof(pcb));
            if (elementoEvaluado->ID == idElementoABuscar && elementoEvaluado->tipo == 'P'){
                memcpy(elementoABuscar,segmentoEvaluado->inicio, elementoEvaluado->tamanio);
                list_destroy(listaFiltrada);
                //free(elementoABuscar);
                return elementoABuscar;
            }
        }
        //free(elementoEvaluado);
        //free(segmentoEvaluado);
    }
}

void compactacion(){
	if(list_is_empty(listaGlobalDeSegmentos)!=1){
		huecoLibreEnLista_struct* nuevoGranHuecoLibre = malloc(sizeof(huecoLibreEnLista_struct));
		for (int i =0;i<list_size(listaGlobalDeSegmentos);i++){
			////log_info(logger,"Tamanio de la lista global de segmentos: %d",list_size(listaGlobalDeSegmentos));
			if(i==0){
				segmentoEnTablaGlobal_struct *primerSegmento = malloc(sizeof(segmentoEnTablaGlobal_struct));
				primerSegmento = list_get(listaGlobalDeSegmentos,0);
				if(primerSegmento->inicio != memoria){
					memcpy(memoria,primerSegmento->inicio,primerSegmento->tamanio);
					primerSegmento->inicio = memoria;
					list_replace(listaGlobalDeSegmentos,0,primerSegmento);
					t_list *listaSegmentosLocal = malloc(sizeof(tablaEnLista_struct));
					for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
						tablaEnLista_struct *tablaIterante = malloc(sizeof(tablaEnLista_struct));
						tablaIterante = list_get(listaDeTablasDePaginas,i);
						if (tablaIterante->idPatota == primerSegmento->idPatota){
							listaSegmentosLocal=tablaIterante->tablaDePaginas;
							break;
						}
						//free(tablaIterante);
					}
					segmentoEnTabla_struct *primerSegmentoLocal = list_get(listaSegmentosLocal,0);
					primerSegmentoLocal->inicio=memoria;
					list_replace(listaSegmentosLocal,0,primerSegmentoLocal);
				}
				//free(primerSegmento);
			}else{
				segmentoEnTablaGlobal_struct *segmentoActual = list_get(listaGlobalDeSegmentos,i);
				segmentoEnTablaGlobal_struct *segmentoAnterior = list_get(listaGlobalDeSegmentos,i-1);
				if ((int)segmentoActual->inicio != ((int)segmentoAnterior->inicio + segmentoAnterior->tamanio)){
					memcpy((int)segmentoAnterior->inicio+segmentoAnterior->tamanio,segmentoActual->inicio,segmentoActual->tamanio);
					segmentoActual->inicio = (int)segmentoAnterior->inicio+segmentoAnterior->tamanio;
					list_replace(listaGlobalDeSegmentos,i,segmentoActual);
					t_list *listaSegmentosLocalActual = malloc(sizeof(tablaEnLista_struct));
					for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
						tablaEnLista_struct *tablaIterante = malloc(sizeof(tablaEnLista_struct));
						tablaIterante = list_get(listaDeTablasDePaginas,i);
						if (tablaIterante->idPatota == segmentoActual->idPatota){
							listaSegmentosLocalActual=tablaIterante->tablaDePaginas;
							break;
						}
						//free(tablaIterante);
					}
					t_list *listaSegmentosLocalAnterior = malloc(sizeof(tablaEnLista_struct));
					for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
						tablaEnLista_struct *tablaIterante = malloc(sizeof(tablaEnLista_struct));
						tablaIterante = list_get(listaDeTablasDePaginas,i);
						if (tablaIterante->idPatota == segmentoAnterior->idPatota){
							listaSegmentosLocalAnterior=tablaIterante->tablaDePaginas;
							break;
						}
						//free(tablaIterante);
					}
					segmentoEnTabla_struct *segmentoLocalActual = list_get(listaSegmentosLocalActual,segmentoActual->segmentoEnLocal);
					segmentoLocalActual->inicio=segmentoActual->inicio;
					list_replace(listaSegmentosLocalActual,segmentoActual->segmentoEnLocal,segmentoLocalActual);
				}
			}
		}
		list_clean_and_destroy_elements(listaHuecosLibres,free);
		int ultimoIndice = list_size(listaGlobalDeSegmentos)-1;
		segmentoEnTablaGlobal_struct* segmentoParaCrearHueco = malloc(sizeof(segmentoEnTablaGlobal_struct));
		segmentoParaCrearHueco = list_get(listaGlobalDeSegmentos,ultimoIndice);
		nuevoGranHuecoLibre->inicio = (int)segmentoParaCrearHueco->inicio + segmentoParaCrearHueco->tamanio;
		nuevoGranHuecoLibre->tamanio = espacioLibre;
		list_add(listaHuecosLibres,nuevoGranHuecoLibre);
	}
}

void actualizar_estado_paginacion(uint32_t idElemento, uint32_t idPatota, char nuevoEstado){
	elementoEnLista_struct *elementoAReemplazar = malloc(sizeof(elementoEnLista_struct));
	for(int i=0;i<list_size(listaElementos);i++){
		elementoAReemplazar = list_get(listaElementos,i);
		if(elementoAReemplazar->ID == idElemento && elementoAReemplazar->tipo=='T'){
			break;
		}
	}

	tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
	t_list *tablaDePaginas;
	for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
		tablaBuscada = list_get(listaDeTablasDePaginas,i);
	    if (tablaBuscada->idPatota == idPatota){
	    	tablaDePaginas = tablaBuscada->tablaDePaginas;
	        break;
	    }
	}

	paginaEnTabla_struct *paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
	int primeraPagina = elementoAReemplazar->segmentoOPagina;


	tcb* tcbAModificar = malloc(sizeof(tcb));
	tcbAModificar = buscar_en_memoria_paginacion(idElemento, idPatota, 'T');
	tcbAModificar->estado = nuevoEstado;
	void* payload = tcbAModificar;
	int frameInicial = paginaInicial->frame;
	int* direccionFisica;
	int payloadYaGuardado=0;
	int tamPayload = 21;
	direccionFisica = (int)memoria + (frameInicial * tamPagina + elementoAReemplazar->offsetEnPagina);
	int menorEntre2 = menorEntreDos(tamPayload,(tamPagina-elementoAReemplazar->offsetEnPagina));
	memcpy(direccionFisica,payload,menorEntre2);
	payloadYaGuardado += menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
	payload = (int)payload + menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
	while (payloadYaGuardado<tamPayload) {
		primeraPagina++;
	    paginaInicial=list_get(tablaDePaginas,primeraPagina);
	    frameInicial = paginaInicial->frame;
	    if (paginaInicial->presencia == 0){
	    	direccionFisica = (int)memoriaSwap + (frameInicial * tamPagina);
	    }else{
	    	direccionFisica = (int)memoria + (frameInicial * tamPagina);
	    }
	    menorEntre2 = menorEntreDos(tamPagina,tamPayload-payloadYaGuardado);
	    memcpy(direccionFisica,payload, menorEntre2);
	    payloadYaGuardado = (int)payloadYaGuardado + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
	    payload = (int)payload + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
	}
}





void actualizar_posicion_paginacion(uint32_t idElemento, uint32_t idPatota, uint32_t nuevaPosX,uint32_t nuevaPosY){
	tipoUniversal = 'T';
	t_list* listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
	elementoEnLista_struct *elementoAReemplazar = malloc(sizeof(elementoEnLista_struct));
	for(int i=0;i<list_size(listaElementos);i++){
		elementoAReemplazar = list_get(listaElementos,i);
		if(elementoAReemplazar->ID == idElemento && elementoAReemplazar->tipo=='T'){
			break;
		}
	}

	tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
	t_list *tablaDePaginas;
	for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
		tablaBuscada = list_get(listaDeTablasDePaginas,i);
	    if (tablaBuscada->idPatota == idPatota){
	    	tablaDePaginas = tablaBuscada->tablaDePaginas;
	        break;
	    }
	}

	paginaEnTabla_struct *paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
	int primeraPagina = elementoAReemplazar->segmentoOPagina;


	tcb* tcbAModificar = malloc(sizeof(tcb));
	tcbAModificar = buscar_en_memoria_paginacion(idElemento, idPatota, 'T');
	int frameInicial = paginaInicial->frame;
	tcbAModificar->posX = nuevaPosX;
	tcbAModificar->posY = nuevaPosY;
	void* payload = tcbAModificar;

	int* direccionFisica;
	int payloadYaGuardado=0;
	int tamPayload = 21;
	direccionFisica = (int)memoria + (frameInicial * tamPagina + elementoAReemplazar->offsetEnPagina);
	        int menorEntre2 = menorEntreDos(tamPayload,(tamPagina-elementoAReemplazar->offsetEnPagina));
	        memcpy(direccionFisica,payload,menorEntre2);
	        payloadYaGuardado += menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
	        payload = (int)payload + menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
	        while (payloadYaGuardado<tamPayload) {
	        	primeraPagina++;
	        	paginaInicial=list_get(tablaDePaginas,primeraPagina);
	        	frameInicial = paginaInicial->frame;
	        	direccionFisica = (int)memoria + (frameInicial * tamPagina);
	        	menorEntre2 = menorEntreDos(tamPagina,tamPayload-payloadYaGuardado);
	        	memcpy(direccionFisica,payload, menorEntre2);
	        	paginaEnTabla_struct *nuevaPagina = malloc(sizeof (paginaEnTabla_struct));
	        	payloadYaGuardado = (int)payloadYaGuardado + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
	        	payload = (int)payload + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
	        }
}






void actualizar_estado_segmentacion(uint32_t idElemento, uint32_t idPatota, char nuevoEstado){

	elementoEnLista_struct *elementoAReemplazar;
	for(int i=0;i<list_size(listaElementos);i++){
		elementoAReemplazar = list_get(listaElementos,i);
		if(elementoAReemplazar->ID == idElemento && elementoAReemplazar->tipo == 'T'){
			break;
		}
		//free(elementoAReemplazar);
	}

	t_list *tablaDePaginas;
	tablaEnLista_struct *tablaBuscada;
	for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
		tablaBuscada = list_get(listaDeTablasDePaginas,i);
	    if (tablaBuscada->idPatota == idPatota){
	    	tablaDePaginas = tablaBuscada->tablaDePaginas;
	        break;
	    }
	    //free(tablaBuscada);
	}

	segmentoEnTabla_struct *paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
	int *inicioSegmento = paginaInicial->inicio;
	tcb* tcbAModificar = malloc(sizeof(tcb));
	//log_info(logger,"Antes de la busqueda");
	tcbAModificar = buscar_de_memoria_segmentacion(idElemento, idPatota, 'T');
	tcbAModificar->estado = nuevoEstado;
	memcpy(inicioSegmento,tcbAModificar,paginaInicial->tamanio);
}




void actualizar_posicion_segmentacion(uint32_t idElemento, uint32_t idPatota, uint32_t nuevaPosX,uint32_t nuevaPosY){

	elementoEnLista_struct *elementoAReemplazar;
	for(int i=0;i<list_size(listaElementos);i++){
		elementoAReemplazar = list_get(listaElementos,i);
		if(elementoAReemplazar->ID == idElemento && elementoAReemplazar->tipo == 'T'){
			break;
		}
	}

	tablaEnLista_struct *tablaBuscada;
	t_list *tablaDePaginas;
	for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
		tablaBuscada = list_get(listaDeTablasDePaginas,i);
	    if (tablaBuscada->idPatota == idPatota){
	    	tablaDePaginas = tablaBuscada->tablaDePaginas;
	        break;
	    }
	    //free(tablaBuscada);
	}

	segmentoEnTabla_struct *paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
	int inicioSegmento = paginaInicial->inicio;

	tcb* tcbAModificar = malloc(sizeof(tcb));
	tcbAModificar = buscar_de_memoria_segmentacion(idElemento, idPatota, 'T');
	tcbAModificar->posX = nuevaPosX;
	tcbAModificar->posY = nuevaPosY;
	memcpy(inicioSegmento,tcbAModificar,paginaInicial->tamanio);
	//free(tcbAModificar);
}

void actualizar_indice_segmentacion(uint32_t idElemento, uint32_t idPatota){
    tipoUniversal = 'T';
    t_list* listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
    elementoEnLista_struct *elementoAReemplazar = malloc(sizeof(elementoEnLista_struct));
    for(int i=0;i<list_size(listaFiltrada);i++){
        elementoAReemplazar = list_get(listaFiltrada,i);
        if(elementoAReemplazar->ID == idElemento){
            break;
        }
        //free(elementoAReemplazar);
    }

    tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
    t_list *tablaDePaginas;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaBuscada = list_get(listaDeTablasDePaginas,i);
        if (tablaBuscada->idPatota == idPatota){
            tablaDePaginas = tablaBuscada->tablaDePaginas;
            break;
        }
        //free(tablaBuscada);
    }

    segmentoEnTabla_struct* paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
    int inicioSegmento = paginaInicial->inicio;

    tcb* tcbAModificar = malloc(sizeof(tcb));
    tcbAModificar = buscar_de_memoria_segmentacion(idElemento, idPatota, 'T');
    tcbAModificar->proxTarea++;
    void* payload = tcbAModificar;


    memcpy(inicioSegmento,tcbAModificar,paginaInicial->tamanio);
}

void actualizar_indice_paginacion(uint32_t idElemento, uint32_t idPatota){
    tipoUniversal = 'T';
    t_list* listaFiltrada = list_filter(listaElementos,filtrarPorTipo);
    elementoEnLista_struct *elementoAReemplazar = malloc(sizeof(elementoEnLista_struct));
    for(int i=0;i<list_size(listaFiltrada);i++){
        elementoAReemplazar = list_get(listaFiltrada,i);
        if(elementoAReemplazar->ID == idElemento){
            break;
        }
    }

    tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
    t_list *tablaDePaginas;
    for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
        tablaBuscada = list_get(listaDeTablasDePaginas,i);
        if (tablaBuscada->idPatota == idPatota){
            tablaDePaginas = tablaBuscada->tablaDePaginas;
            break;
        }
    }

    paginaEnTabla_struct* paginaInicial=list_get(tablaDePaginas,elementoAReemplazar->segmentoOPagina);
    int primeraPagina = elementoAReemplazar->segmentoOPagina;


    tcb* tcbAModificar = malloc(sizeof(tcb));
    tcbAModificar = buscar_en_memoria_paginacion(idElemento, idPatota, 'T');
    int frameInicial = paginaInicial->frame;
    tcbAModificar->proxTarea++;
    void* payload = tcbAModificar;

    int* direccionFisica;
    int payloadYaGuardado=0;
    int tamPayload = 21;
    direccionFisica = (int)memoria + (frameInicial * tamPagina + elementoAReemplazar->offsetEnPagina);
    int menorEntre2 = menorEntreDos(tamPayload,(tamPagina-elementoAReemplazar->offsetEnPagina));
    memcpy(direccionFisica,payload,menorEntre2);
    payloadYaGuardado += menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
    payload = (int)payload + menorEntreDos(tamPayload,tamPagina-elementoAReemplazar->offsetEnPagina);
    while (payloadYaGuardado<tamPayload) {
        primeraPagina++;
        paginaInicial=list_get(tablaDePaginas,primeraPagina);
        frameInicial = paginaInicial->frame;
        if (paginaInicial->presencia == 0){
            direccionFisica = (int)memoriaSwap + (frameInicial * tamPagina);
        }else{
            direccionFisica = (int)memoria + (frameInicial * tamPagina);
        }
        menorEntre2 = menorEntreDos(tamPagina,tamPayload-payloadYaGuardado);
        memcpy(direccionFisica,payload, menorEntre2);
        payloadYaGuardado = (int)payloadYaGuardado + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
        payload = (int)payload + menorEntreDos(tamPayload-payloadYaGuardado,tamPagina);
    }
}

void actualizar_lista_elementos_segmentacion(int segmentoAgregado,int PID){
	elementoEnLista_struct* elementoIterante = malloc(sizeof(elementoEnLista_struct));
	for (int i=0;i<list_size(listaElementos);i++){
		elementoIterante = list_get(listaElementos,i);
		if ((elementoIterante->segmentoOPagina >= segmentoAgregado) && (elementoIterante->PID == PID)){
			elementoIterante->segmentoOPagina++;
			list_replace(listaElementos,i,elementoIterante);
		}
	}
}

int sacarPaginaDeMemoria(){
	paginaParaReemplazar_struct *paginaAReemplazar = malloc(sizeof(paginaParaReemplazar_struct));
	if (strcmp(alg_remplazo,"LRU")==0){
		paginaAReemplazar = queue_pop(tablaDeFrames);
	} else{
		list_sort(tablaDeFrames->elements,ordenar_por_nro_frame);
		//log_info(logger2, "Puntero reemplazo: %d",punteroReemplazo);
		for (int i=0;i<list_size(tablaDeFrames->elements);i++){
			paginaParaReemplazar_struct *paginaDePrueba = malloc(sizeof(paginaParaReemplazar_struct));
			paginaDePrueba = list_get(tablaDeFrames->elements,i);
			////log_info(logger, "Bit de uso de la pagina %d de la patota %d: %d",paginaDePrueba->nroPagina,paginaDePrueba->PID,paginaDePrueba->uso);
		}
		while (1) {

			paginaAReemplazar = list_get(tablaDeFrames->elements, punteroReemplazo);
			if (paginaAReemplazar->uso==1){
				paginaAReemplazar->uso = 0;
				list_replace(tablaDeFrames->elements,punteroReemplazo,paginaAReemplazar);
				if (punteroReemplazo+1 == queue_size(tablaDeFrames)){
					punteroReemplazo = 0;
				}else{
					punteroReemplazo++;
				}
			} else{
				list_remove(tablaDeFrames->elements,punteroReemplazo);
				if(punteroReemplazo == queue_size(tablaDeFrames)){
					punteroReemplazo = 0;
				}else{
					punteroReemplazo++;
				}
				break;
			}
		}
		//log_info(logger2,"Voy a sacar la pagina %d del proceso %d",paginaAReemplazar->nroPagina,paginaAReemplazar->PID);
		//loggearTablaDeFrames();
	}
	int frameEnSwap = encontrarFrameEnSwapDisponible();
	int* direccionFisicaPaginaEnSwap = (int)memoriaSwap + (frameEnSwap * tamPagina);
	int* direccionFisicaPaginaEnMemoria = (int) memoria + (paginaAReemplazar->nroFrame * tamPagina);
	memcpy(direccionFisicaPaginaEnSwap,direccionFisicaPaginaEnMemoria,tamPagina);
	bitarraySwap[frameEnSwap]=1;
	tablaEnLista_struct *tablaBuscada = malloc(sizeof(tablaEnLista_struct));
	t_list *tablaDePaginasBuscada;
	for (int i = 0; i < list_size(listaDeTablasDePaginas); ++i) {
		tablaBuscada = list_get(listaDeTablasDePaginas,i);
		if (tablaBuscada->idPatota == paginaAReemplazar->PID){
			tablaDePaginasBuscada = tablaBuscada->tablaDePaginas;
			break;
		}
	}
	paginaEnTabla_struct *paginaAActualizar = malloc(sizeof(paginaEnTabla_struct));
	paginaAActualizar =list_get(tablaDePaginasBuscada,paginaAReemplazar->nroPagina);
	paginaAActualizar->presencia = 0;
	paginaAActualizar->frame = frameEnSwap;
	list_replace(tablaDePaginasBuscada,paginaAReemplazar->nroPagina,paginaAActualizar);
	return paginaAReemplazar->nroFrame;
}

bool filtrar_Tareas_patota(void* elemento){
    elementoEnLista_struct *comparador = elemento;
    return (comparador->tipo == 'A' && comparador->PID == patotaUniversal);
}

int contarTareas(int idPatota){
	patotaUniversal = idPatota;
	t_list* listaFiltrada = list_filter(listaElementos,filtrar_Tareas_patota);
	return list_size(listaFiltrada);
}
