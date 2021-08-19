/*
 ============================================================================
 Name        : mi_ram_hq.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

// =============== PAHTS =================
//-------------------------------------
//PARA EJECUTAR DESDE ECLIPSE USAR:
//#define PATH_CONFIG "src/mi_ram_hq.config"
//-------------------------------------
//PARA EJECUTAR DESDE CONSOLA USAR:
#define PATH_CONFIG "../src/mi_ram_hq.config"
//-------------------------------------

#include "mi_ram_hq.h"


#define ASSERT_CREATE(nivel, id, err)


int main(void) {
	int socketCliente, socketServer;

	config = config_create(PATH_CONFIG);
	funcionando=true;
	tamMemoria = config_get_int_value(config, "TAMANIO_MEMORIA");
	esquemaMemoria = config_get_string_value(config, "ESQUEMA_MEMORIA");
	tamPagina = config_get_int_value(config, "TAMANIO_PAGINA");
	tamSwap = config_get_int_value(config, "TAMANIO_SWAP");
	path_swap = config_get_string_value(config, "PATH_SWAP");
	alg_remplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
	crit_seleccion = config_get_string_value(config, "CRITERIO_SELECCION");
	puerto = config_get_string_value(config, "PUERTO");
	pthread_mutex_init(&mutexMemoria, NULL);
	for(int i =0; i<94;i++){
		vectorIdTripulantes[i] = -1;
	}

	signal(SIGUSR1,&manejoDump);
	signal(SIGUSR2,&manejoCompactacion);
	if (strcmp(crit_seleccion,"FF") == 0){
		tipoDeGuardado = FIRSTFIT;
	}else{
		tipoDeGuardado = BESTFIT;
	}
	if((logger = log_create("../log_memoria.log", "Memoria", 0, LOG_LEVEL_INFO)) == NULL)
	{
		printf(" No pude leer el logger\n");
		exit(1);
	}


	if((logger2 = log_create("../log_tablaDeFrames.log", "Memoria", 0, LOG_LEVEL_INFO)) == NULL)
	{
		printf(" No pude leer el logger\n");
		exit(1);
	}

	memoria = malloc(tamMemoria);
	log_info(logger,"Direccion inicial de memoria: %d",memoria);
	listaElementos = list_create();
	listaDeTablasDePaginas = list_create();
	if (strcmp(esquemaMemoria,"PAGINACION")==0){
		memoriaSwap = malloc(tamSwap);
		cantidadPaginas = tamMemoria / tamPagina;
		cantidadPaginasSwap = tamSwap / tamPagina;
		bitarrayMemoria = calloc(cantidadPaginas, sizeof(int));
		bitarraySwap = calloc(cantidadPaginasSwap, sizeof(int));
		tablaDeFrames = queue_create();
		punteroReemplazo = 0;
		espacioLibre = tamMemoria + tamSwap;
		int swap = open(path_swap, O_CREAT | O_RDWR);
		ftruncate(swap, tamSwap);
		mmap(memoriaSwap, tamSwap, PROT_READ | PROT_WRITE, MAP_SHARED, swap, 0);
		close(swap);
		msync(memoriaSwap, tamSwap, MS_SYNC);


	}else if (strcmp(esquemaMemoria,"SEGMENTACION")==0){
		listaHuecosLibres = list_create();
		huecoLibreEnLista_struct *primerHueco = malloc(sizeof(huecoLibreEnLista_struct));
		primerHueco->inicio = memoria;
		primerHueco->tamanio = tamMemoria;
		list_add(listaHuecosLibres,primerHueco);
		listaSegmentos = list_create();
		espacioLibre = tamMemoria;
		listaGlobalDeSegmentos = list_create();

	}

	nivel = crear_mapa();
	socketServer = crear_server(puerto);

	while (funcionando) {
		socketCliente = esperar_cliente(socketServer, 5);
		if (socketCliente == -1)
			continue;

			pthread_t hiloCliente;
			pthread_create(&hiloCliente,NULL,(void*)administrar_cliente,socketCliente);
			pthread_detach(hiloCliente);
	}


	liberar_conexion(socketServer);

	//crear_personajes(nivel, patota);
	return EXIT_SUCCESS;
	}



void administrar_cliente(int socketCliente){
		int respuesta;
		t_paquete* paquete_recibido = recibir_paquete(socketCliente, &respuesta);
		if (paquete_recibido->codigo_operacion == -1 || respuesta == ERROR) {
			liberar_conexion(socketCliente);
			eliminar_paquete(paquete_recibido);
		}

//		printf("PAQUETE DE TIPO %d RECIBIDO\n",paquete_recibido->codigo_operacion);

	switch(paquete_recibido->codigo_operacion) {
		case INICIAR_PATOTA:;
			//printf("Inicio una patota /n");
			t_iniciar_patota* estructura_iniciar_patota = deserializar_iniciar_patota(paquete_recibido);
			//imprimir_paquete_iniciar_patota(estructura_iniciar_patota);
			int espacioNecesario;
			espacioNecesario = estructura_iniciar_patota->tamanio_tareas + (estructura_iniciar_patota->cantTripulantes*21)+8;
			if(espacioLibre<espacioNecesario){
				log_info(logger, "No tengo espacio en la memoria para guardar la patota %d\n",estructura_iniciar_patota->idPatota);

				char* fault = strdup("fault");
				uint32_t tamanio_fault = strlen(fault)+1;
				send(socketCliente,&tamanio_fault,sizeof(uint32_t),0);
				send(socketCliente, fault,tamanio_fault,0);
				free(fault);
			}else{
				char* fault = strdup("Ok");
				uint32_t tamanio_fault = strlen(fault)+1;
				send(socketCliente,&tamanio_fault,sizeof(uint32_t),0);
				send(socketCliente, fault,tamanio_fault,0);
				free(fault);
				pcb* nuevaPatota = malloc(sizeof(pcb));
				nuevaPatota->id = estructura_iniciar_patota->idPatota;
				if (strcmp(esquemaMemoria,"PAGINACION")==0){
					tablaEnLista_struct *nuevaTablaPatota = malloc(sizeof(tablaEnLista_struct));
					nuevaTablaPatota->idPatota = nuevaPatota->id;
					log_info(logger, "Recibi la patota: %d\n",nuevaPatota->id);
					nuevaTablaPatota->tablaDePaginas = list_create();
					list_add(listaDeTablasDePaginas, nuevaTablaPatota);
				}else{
					tablaEnLista_struct *nuevaListaDeTablasDePaginas = malloc(sizeof(tablaEnLista_struct));
					nuevaListaDeTablasDePaginas->tablaDePaginas = list_create();
					nuevaListaDeTablasDePaginas->idPatota=estructura_iniciar_patota->idPatota;
					list_add(listaDeTablasDePaginas,nuevaListaDeTablasDePaginas);
				}
				nuevaPatota->tareas = 0; //calcular_direccion_logica_archivo(estructura_iniciar_patota->idPatota);
				guardar_en_memoria_general(nuevaPatota,estructura_iniciar_patota->idPatota,sizeof(pcb),estructura_iniciar_patota->idPatota,'P');
				log_info(logger, "Guarde el PCB de la patota %d\n",estructura_iniciar_patota->idPatota);
				if (strcmp(esquemaMemoria,"SEGMENTACION")==0){
					//log_info(logger, "Tareas: %s\n",estructura_iniciar_patota->Tareas);
					//log_info(logger, "TAMANIO DE TAREAS:  %d",estructura_iniciar_patota->tamanio_tareas);
					//log_info(logger,"Tareas: %s",estructura_iniciar_patota->Tareas);
					guardar_en_memoria_general(estructura_iniciar_patota->Tareas,estructura_iniciar_patota->idPatota,estructura_iniciar_patota->tamanio_tareas,estructura_iniciar_patota->idPatota,'A');
					log_info(logger, "Guarde las tareas de la patota %d\n",estructura_iniciar_patota->idPatota);
				}else{
					char **arrayTareas = string_split(estructura_iniciar_patota->Tareas,"|");
					//log_info(logger,"Tarea 1: %s",arrayTareas[1]);
					int i =0;
					while(arrayTareas[i]!=NULL){
						guardar_en_memoria_general(arrayTareas[i],i,strlen(arrayTareas[i]),estructura_iniciar_patota->idPatota,'A');
						log_info(logger,"Guarde la tarea %s de tamanio %d",arrayTareas[i],strlen(arrayTareas[i]));
						i++;
					}
				}
			}
			liberar_t_iniciar_patota(estructura_iniciar_patota);
			liberar_conexion(socketCliente);
			break;
		case TRIPULANTE:;
			//printf("CASE TRIPULANTE /n");
				t_tripulante* estructura_tripulante = deserializar_tripulante(paquete_recibido);
				tcb *nuevoTripulante = malloc(sizeof(tcb));
				nuevoTripulante->id = estructura_tripulante->id_tripulante;
				nuevoTripulante->estado = 'R';
				nuevoTripulante->posX = estructura_tripulante->posicion_x;
				nuevoTripulante->posY = estructura_tripulante->posicion_y;
				nuevoTripulante->proxTarea=0;
				nuevoTripulante->dirLogicaPcb=(uint32_t)calcular_direccion_logica_patota((int)estructura_tripulante->id_patota);
				guardar_en_memoria_general(nuevoTripulante,estructura_tripulante->id_tripulante,21,estructura_tripulante->id_patota,'T');
				for(int i =0; i<94;i++){
					if (vectorIdTripulantes[i]==-1){
						vectorIdTripulantes[i] = nuevoTripulante->id;
						pthread_mutex_lock(&mutexMapa);
						dibujarTripulante(nuevoTripulante,(i+33));
						pthread_mutex_unlock(&mutexMapa);
						break;
					}
				}
				log_info(logger, "Guarde el tripulante %d\n",estructura_tripulante->id_tripulante);
				//printf("CREE UN TRIPULANTE: %d\n",nuevoTripulante->id);

				liberar_t_tripulante(estructura_tripulante);
				liberar_conexion(socketCliente);
				break;
		case ELIMINAR_TRIPULANTE:;
			//printf("eliminar TRIPULANTE /n");
				t_tripulante* tripulante_a_eliminar = deserializar_tripulante(paquete_recibido);

				borrar_de_memoria_general(tripulante_a_eliminar->id_tripulante, tripulante_a_eliminar->id_patota, 'T');
				for(int i =0; i<94;i++){
					if (vectorIdTripulantes[i]==tripulante_a_eliminar->id_tripulante){
						vectorIdTripulantes[i] = 0;
						pthread_mutex_lock(&mutexMapa);
						borrarTripulante((i+33));
						pthread_mutex_unlock(&mutexMapa);
						break;
					}
				}
				log_info(logger, "Borre el tripulante %d\n",tripulante_a_eliminar->id_tripulante);
				log_info(logger2,"Borro el tripulante %d",tripulante_a_eliminar->id_tripulante);
				//loggearTablaDeFrames();
				liberar_conexion(socketCliente);
				liberar_t_tripulante(tripulante_a_eliminar);
				break;

		case PEDIR_TAREA:;
			//printf("PEDIR TAREA /n");
				t_tripulante* tripulante_solicitud = deserializar_tripulante(paquete_recibido);
				//imprimir_paquete_tripulante(tripulante_solicitud);
				tcb *tripulanteATraer = malloc(sizeof(tcb));
				tripulanteATraer = buscar_en_memoria_general(tripulante_solicitud->id_tripulante,tripulante_solicitud->id_patota,'T');
				int totalDeTareas=0;
					if(strcmp(esquemaMemoria,"PAGINACION")==0){
						//log_info(logger,"Entre al if de la busqueda en la paginacion");
						totalDeTareas = contarTareas(tripulante_solicitud->id_patota);
						//log_info(logger,"Cantidad de tareas: %d",totalDeTareas);
						if(tripulanteATraer->proxTarea==totalDeTareas){
							char* fault = strdup("fault");
							uint32_t tamanio_fault = strlen(fault)+1;
							send(socketCliente,&tamanio_fault,sizeof(uint32_t),0);
							send(socketCliente, fault,tamanio_fault,0);
							free(fault);
							log_info(logger, "Mande la tarea fault\n");
						}else{
							//log_info(logger, "Voy a buscar la tarea %d",tripulanteATraer->proxTarea);
							char *tarea = string_new();

							tarea = buscar_en_memoria_general(tripulanteATraer->proxTarea,tripulante_solicitud->id_patota,'A');
							log_info(logger,"Tarea que voy a mandar: %s",tarea);
							int tamanio_tarea = strlen(tarea)+1;
							send(socketCliente, &tamanio_tarea,sizeof(uint32_t),0);
							send(socketCliente, tarea,tamanio_tarea,0);
							log_info(logger, "Mande la tarea %s (Numero %d) al tripulante %d",tarea,tripulanteATraer->proxTarea,tripulanteATraer->id);

						}
						pthread_mutex_lock(&mutexMemoria);
						actualizar_indice_paginacion(tripulante_solicitud->id_tripulante,tripulante_solicitud->id_patota);
						pthread_mutex_unlock(&mutexMemoria);
					}else if (strcmp(esquemaMemoria,"SEGMENTACION")==0){
						char*tareas = string_new();
						tareas = buscar_en_memoria_general(tripulante_solicitud->id_patota,tripulante_solicitud->id_patota,'A');
						char **arrayTareas = string_split(tareas,"|");
						free(tareas);
						int i=0;
						while(arrayTareas[i]!=NULL){
							totalDeTareas++;
							i++;
						}
						if(tripulanteATraer->proxTarea==totalDeTareas){
							char* fault = strdup("fault");
							uint32_t tamanio_fault = strlen(fault)+1;
							send(socketCliente,&tamanio_fault,sizeof(uint32_t),0);
							send(socketCliente, fault,tamanio_fault,0);
							free(fault);
							for (int j=0;j<totalDeTareas;j++){
								free(arrayTareas[j]);
							}
							free(arrayTareas);
							log_info(logger, "Mande la tarea fault\n");
						}else{
							//log_info(logger, "Soy el tripulante %d, estoy en x=%d y=%d\n",tripulanteATraer->id,tripulanteATraer->posX,tripulanteATraer->posY);
							int tamanio_tarea = strlen(arrayTareas[tripulanteATraer->proxTarea])+1;
							send(socketCliente, &tamanio_tarea,sizeof(uint32_t),0);
							send(socketCliente, arrayTareas[tripulanteATraer->proxTarea],tamanio_tarea,0);
							log_info(logger, "Mande la tarea %s (Numero %d) al tripulante %d\n",arrayTareas[tripulanteATraer->proxTarea],tripulanteATraer->proxTarea,tripulanteATraer->id);
							for (int j=0;j<totalDeTareas;j++){
								free(arrayTareas[j]);
							}
							free(arrayTareas);
						}
						pthread_mutex_lock(&mutexMemoria);
						actualizar_indice_segmentacion(tripulante_solicitud->id_tripulante,tripulante_solicitud->id_patota);
						pthread_mutex_unlock(&mutexMemoria);

					}
					//free(tripulanteATraer->estado);
					//free(tripulanteATraer);
					liberar_conexion(socketCliente);
					liberar_t_tripulante(tripulante_solicitud);
					break;


		case ACTUALIZAR_POS:;
			//printf("ACTUALIZAR POS/n");
				t_tripulante* tripulante_a_mover = deserializar_tripulante(paquete_recibido);
				//imprimir_paquete_tripulante(tripulante_a_mover);
				tcb *tripulanteAMover = malloc(sizeof(tcb));
				tripulanteAMover = buscar_en_memoria_general(tripulante_a_mover->id_tripulante,tripulante_a_mover->id_patota, 'T');
				tripulanteAMover->posX = tripulante_a_mover->posicion_x;
				tripulanteAMover->posY = tripulante_a_mover->posicion_y;
				if(strcmp(esquemaMemoria,"PAGINACION")==0){
					pthread_mutex_lock(&mutexMemoria);
					actualizar_posicion_paginacion(tripulante_a_mover->id_tripulante,tripulante_a_mover->id_patota,tripulante_a_mover->posicion_x,tripulante_a_mover->posicion_y);
					pthread_mutex_unlock(&mutexMemoria);
				}else if (strcmp(esquemaMemoria,"SEGMENTACION")==0){
					pthread_mutex_lock(&mutexMemoria);
					actualizar_posicion_segmentacion(tripulante_a_mover->id_tripulante,tripulante_a_mover->id_patota,tripulante_a_mover->posicion_x,tripulante_a_mover->posicion_y);
					pthread_mutex_unlock(&mutexMemoria);
				}
				id_and_pos *tripulanteEnMapa = malloc(sizeof(id_and_pos));
				tripulanteEnMapa->idTripulante = tripulanteAMover->id;
				tripulanteEnMapa->posX = tripulante_a_mover->posicion_x;
				tripulanteEnMapa->posY = tripulante_a_mover->posicion_y;

				for(int i =0; i<94;i++){
					if (vectorIdTripulantes[i]==tripulanteEnMapa->idTripulante){
						pthread_mutex_lock(&mutexMapa);
						actualizarPosicion(tripulanteEnMapa,i+33);
						pthread_mutex_unlock(&mutexMapa);
						break;
					}
				}
				liberar_conexion(socketCliente);
				liberar_t_tripulante(tripulante_a_mover);
				break;

		case ACTUALIZAR_ESTADO:;
			//printf("ACTUALIZAR_ESTADO /n");
				t_cambio_estado* tripulante_a_actualizar = deserializar_cambio_estado(paquete_recibido);
				log_info(logger2,"Voy a actualizar el estado del tripulante: %d, de la patota: %d",tripulante_a_actualizar->id_tripulante,tripulante_a_actualizar->id_patota);
				//imprimir_paquete_cambio_estado(tripulante_a_actualizar);
				if(strcmp(esquemaMemoria,"PAGINACION")==0){
					pthread_mutex_lock(&mutexMemoria);
					actualizar_estado_paginacion(tripulante_a_actualizar->id_tripulante,tripulante_a_actualizar->id_patota,tripulante_a_actualizar->estado);
					pthread_mutex_unlock(&mutexMemoria);
					//printf("CAMBIO ESTADO: %c\n",tcbDePrueba->estado);
				}else{
					pthread_mutex_lock(&mutexMemoria);
					actualizar_estado_segmentacion(tripulante_a_actualizar->id_tripulante,tripulante_a_actualizar->id_patota,tripulante_a_actualizar->estado);
					pthread_mutex_unlock(&mutexMemoria);
				}
				log_info(logger, "Actualice el estado del tripulante %d a %c\n",tripulante_a_actualizar->id_tripulante, tripulante_a_actualizar->estado);
				liberar_conexion(socketCliente);
				liberar_t_cambio_estado(tripulante_a_actualizar);
				break;

		case FINALIZAR:;
			t_tripulante* tripulante_a_liberar = deserializar_tripulante(paquete_recibido);
			terminar_programa();
			liberar_t_tripulante(tripulante_a_liberar);
			break;
		case FIN_PATOTA:;
			t_tripulante* patota_a_eliminar = deserializar_tripulante(paquete_recibido);
			//log_info(logger,"Se va a borrar la patota: %d", patota_a_eliminar->id_patota);
			borrar_de_memoria_general(patota_a_eliminar->id_patota, patota_a_eliminar->id_patota,'P');
			if(strcmp(esquemaMemoria,"PAGINACION")==0){
				int cantTareas = contarTareas(patota_a_eliminar->id_patota);
				for(int i =0;i<cantTareas;i++){
					borrar_de_memoria_general(i, patota_a_eliminar->id_patota,'A');
				}
			}else{
				borrar_de_memoria_general(patota_a_eliminar->id_patota, patota_a_eliminar->id_patota,'A');
			}
			log_info(logger,"Se borro de memoria la patota: %d", patota_a_eliminar->id_patota);
			liberar_conexion(socketCliente);
			liberar_t_tripulante(patota_a_eliminar);
			break;

		default:;

			break;


	}
}

char intAChar(int numero){
	return numero + '0';
}

void actualizarPosicion(id_and_pos* nuevaPos,char id){
	item_mover(nivel, id, nuevaPos-> posX,nuevaPos->posY);
	nivel_gui_dibujar(nivel);
}


void dibujarTripulante(tcb* tripulante, char id){
	int err;
	//printf("el id en dibu tripu es: %c",id);
	//char* id[3] = '0';
	//char id = intAChar(tripulante->id);
	err = personaje_crear(nivel, id, tripulante->posX, tripulante->posY);
	ASSERT_CREATE(nivel, id, err);

	/*if(err) {
		//printf("Error: %s\n", nivel_gui_string_error(err));
	}*/
	nivel_gui_dibujar(nivel);

	//free (id);
}
void borrarTripulante( char id){
	item_borrar(nivel, id);
	nivel_gui_dibujar(nivel);
}

NIVEL *crear_mapa(){
		NIVEL *nivel;
		int cols=9, rows=9;
		int err;

		nivel_gui_inicializar();

		nivel_gui_get_area_nivel(&cols, &rows);

		nivel = nivel_crear("Mapa de la nave");
		nivel_gui_dibujar(nivel);
		//printf("Ya Dibuje \n");
		return nivel;
}

void dumpDeMemoria(){
	char* timestamp = (char*) temporal_get_string_time("%d-%m-%y_%H:%M:%S");
	char* nombreArchivo = string_new();
	//string_append(&nombreArchivo,"/home/utnso/TP/tp-2021-1c-Cebollitas-subcampeon/mi_ram_hq/");
	string_append(&nombreArchivo,"Dump_");
	string_append(&nombreArchivo,timestamp);
	string_append(&nombreArchivo,".dmp");
	tablaEnLista_struct *tablaAEvaluar = malloc(sizeof(tablaEnLista_struct));
	paginaEnTabla_struct* paginaBuscada = malloc(sizeof(paginaEnTabla_struct));
	FILE* dmp = fopen (nombreArchivo, "w+");
	//FILE* dmp = fopen ("Dump_", "w+");
	free(nombreArchivo);
	char* lineaAAgregar=string_new();
	fprintf(dmp,"Dump: %s \n",timestamp);
	if(strcmp(esquemaMemoria,"PAGINACION")==0){
		for(int i =0;i<(tamMemoria/tamPagina);i++){
			if(bitarrayMemoria[i] == 0){
				fprintf(dmp,"Marco: %d  Estado:Libre  Proceso:-  Pagina:- \n",i);
			}else{
				for(int j=0;j<list_size(listaDeTablasDePaginas);j++){
					tablaAEvaluar = list_get(listaDeTablasDePaginas,j);
					for(int k = 0; k<list_size(tablaAEvaluar->tablaDePaginas);k++){
						paginaBuscada = list_get(tablaAEvaluar->tablaDePaginas,k);
						if(paginaBuscada->frame == i && paginaBuscada->presencia==1){
							fprintf(dmp,"Marco:%d  Estado:Ocupado  Proceso:%d  Pagina:%d espacio ocupado: %d\n",i,tablaAEvaluar->idPatota,k,paginaBuscada->espacioOcupado);
						}
					}
				}

			}
		}
	}else{
		tablaEnLista_struct *tablaDePaginas = malloc(sizeof(tablaEnLista_struct));
		segmentoEnTabla_struct* segmentoIterante = malloc(sizeof(segmentoEnTabla_struct));
		for(int i=0; i < list_size(listaDeTablasDePaginas);i++){
			tablaDePaginas = list_get(listaDeTablasDePaginas,i);
			for(int k=0; k<list_size(tablaDePaginas->tablaDePaginas);k++){
				segmentoIterante = list_get(tablaDePaginas->tablaDePaginas,k);
				fprintf(dmp,"Proceso:%d  Segmento:%d  Inicio:%d  Tamanio:%dB \n",tablaDePaginas->idPatota,k,segmentoIterante->inicio,segmentoIterante->tamanio);
			}
		}
	}

	fclose(dmp);
}

void loggearTablaDeFrames(){
	log_info(logger2, "El puntero de reemplazo  esta en el frame : %d" ,punteroReemplazo);
	for (int i = 0;i<list_size(tablaDeFrames->elements);i++){
		paginaParaReemplazar_struct *framePrinteable = malloc(sizeof(paginaParaReemplazar_struct));
		framePrinteable = list_get(tablaDeFrames->elements,i);
		log_info(logger2,"PID: %d, Nro frame: %d, Nro Pagina: %d, Uso: %d",framePrinteable->PID,framePrinteable->nroFrame,framePrinteable->nroPagina,framePrinteable->uso);
	}
	log_info(logger2,"\n");
}

void manejoDump(int signal){
	dumpDeMemoria();
}

void manejoCompactacion(int signal){
	compactacion();
}

terminar_programa(){
	funcionando = false;
	for(int i=0;i<list_size(listaDeTablasDePaginas);i++){
		tablaEnLista_struct *tablaABorrar = malloc(sizeof(tablaEnLista_struct));
		tablaABorrar = list_get(listaDeTablasDePaginas,i);
		list_destroy_and_destroy_elements(tablaABorrar->tablaDePaginas,free);
	}
	list_destroy_and_destroy_elements(listaDeTablasDePaginas,free);
	list_destroy_and_destroy_elements(listaElementos,free);
	if(strcmp(esquemaMemoria,"PAGINACION")==0){
		free(bitarrayMemoria);
		free(bitarraySwap);
		list_destroy_and_destroy_elements(tablaDeFrames->elements,free);
		free(memoriaSwap);
	}else{
		list_destroy_and_destroy_elements(listaGlobalDeSegmentos,free);
		list_destroy_and_destroy_elements(listaHuecosLibres,free);

	}
	free(memoria);
	nivel_destruir(nivel);                                                          \
	nivel_gui_terminar();
	log_info(logger,"EJECUCION FINALIZADA, GRACIAS POR TODO!");
	log_destroy(logger);
	log_destroy(logger2);
}
