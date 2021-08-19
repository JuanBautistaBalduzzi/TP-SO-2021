/*
 ============================================================================
 Name        : iMongoStore.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "iMongoStore.h"
#include <signal.h>
//#define PUERTO "6667"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024// Define cual va a ser el size maximo del paquete a enviar


struct t_bitarray{
	char *bitarray;
	size_t size;
	bit_numbering_t mode;
};

typedef struct
{
	char tipo;
	char recurso;
	int cantidad;
} recursos;

typedef struct
{
	int id;
	char* texto;
} escrbir_bitacora;

int blocks_sabot;


//-------------------------------------
//PARA EJECUTAR DESDE CONSOLA USAR:
#define PATH_CONFIG "../config/mongoStore.config"
#define PATH_LOG_G "../config/log_general.log"
#define PATH_LOG_B "../config/log_bitacoras.log"
#define PATH_LOG_SE "../config/log_server.log"
#define PATH_LOG_SA "../config/log_sabotaje.log"
//-------------------------------------
//PARA EJECUTAR DESDE ECLIPSE USAR:
//#define PATH_CONFIG "config/mongoStore.config"
//#define PATH_LOG_G "config/log_general.log"
//#define PATH_LOG_B "config/log_bitacoras.log"
//#define PATH_LOG_SE "config/log_server.log"
//#define PATH_LOG_SA "config/log_sabotaje.log"
////-------------------------------------


int main(void) {

	mongoStore_config = leer_config(PATH_CONFIG);

	printf("Checkpoint 1");

	//IP = config_get_string_value(conexion_config, "IP_MONGOSTORE");
//	logger_path_mongostore = config_get_string_value(mongoStore_config, "ARCHIVO_LOG");

	printf("Checkpoint 2");

	logger = iniciar_logger(PATH_LOG_G);
	log_bitacoras = iniciar_logger(PATH_LOG_B);
	log_mensaje = iniciar_logger(PATH_LOG_SE);
	log_sabotaje = iniciar_logger(PATH_LOG_SA);

	printf("Checkpoint 3");

	punto_montaje = config_get_string_value(mongoStore_config, "PUNTO_MONTAJE");

	//Generamos la conexion Mongo => Discordiador
	ipDiscordiador=config_get_string_value(mongoStore_config,"IP_DISCORDIADOR");
	puertoDicordiador=config_get_string_value(mongoStore_config,"PUERTO_DISCORDIADOR");
	puerto_mongostore = config_get_string_value(mongoStore_config, "PUERTO_MONGOSTORE");
	//int server_FS = iniciar_servidor(IP, puerto_mongostore);

	bloques = config_get_int_value(mongoStore_config, "BLOCKS");
	blocks_sabot=bloques;
	tamanio_bloque = config_get_int_value(mongoStore_config, "BLOCK_SIZE");
	tiempoSincro = config_get_int_value(mongoStore_config, "TIEMPO_SINCRONIZACION");
	//Inicializamos el File System, consultamos si ya existen los archivos o no
	char* ruta_superbloque = string_new();
	char* ruta_blocks = string_new();
	string_append(&ruta_superbloque, punto_montaje);
	string_append(&ruta_blocks, punto_montaje);
	string_append(&ruta_superbloque, "/Superbloque/Superbloque.ims");
	string_append(&ruta_blocks, "/Blocks/Blocks.ims");

	//Inicializamos File System
	if(verificar_existencia(ruta_blocks) == 0 && verificar_existencia(ruta_superbloque) == 0){
		inicializar_carpetas();
		crear_superbloque();
		inicializar_bloques();
		crear_archivo_files();
		log_info(logger,"Se inicializo el file System");
	}
	else
	{
		//RESTAURAMOS UN FILE YA CREADO
		restaurar_file_system();
		log_info(logger,"Se restauro el file System");
	}
	pthread_mutex_init(&mutexEscrituraBloques,NULL);
	pthread_mutex_init(&mutexBitacoras,NULL);

	// LEVANTAMOS LA SINCRO DE LOS BLOQUES

	pthread_create(&sincro,NULL,sincronizar_blocks,NULL);

	pthread_create(&sabo,NULL,atender_signal,NULL);

	int server_fs=crear_server(puerto_mongostore);
	while(correr_programa)
	{
		int socketTripulante= esperar_cliente(server_fs, 10);
		if (socketTripulante == -1)
			continue;

			pthread_t hiloTripulante;
			pthread_create(&hiloTripulante,NULL,(void*)atender_mensaje,(void*)socketTripulante);
			pthread_detach(hiloTripulante);

	}

	return EXIT_SUCCESS;
}


//--------------------------------FUNCIONES-----------------------------------------------------

void* atender_signal()
{
	signal(SIGUSR1,&interrupt_handler);
	pause();

	return NULL;
}
void restaurar_file_system()
{
	char* path_superbloque = string_new();

	string_append(&path_superbloque, punto_montaje);
	string_append(&path_superbloque, "/Superbloque/Superbloque.ims");
	int fd = open(path_superbloque, O_CREAT | O_RDWR);

	if (fd == -1) {
		close(fd);
		log_error(logger, "Error abriendo el Superbloque.ims");
		return;
	}
	if(bloques%8==0){
		tamanioBitmap = bloques/8;
	}
	else {
		tamanioBitmap =( bloques/8)+1;
	}

	int res =4+4+tamanioBitmap;
	superbloque = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	bitmap = bitarray_create_with_mode((void*) superbloque+2*sizeof(uint32_t), tamanioBitmap, MSB_FIRST);
	msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);

	//Sincronizas
	msync(superbloque, res, MS_SYNC);
	close(fd);

	char* ruta_blocks = string_new();
	string_append(&ruta_blocks, punto_montaje);
	string_append(&ruta_blocks, "/Blocks/Blocks.ims");

	 res = tamanio_bloque * bloques;

	//copiaBlock = malloc(res);

	int fdb = open(ruta_blocks, O_CREAT | O_RDWR);

	if (fd == -1) {
		close(fdb);
		log_error(logger, "Error abriendo el Blocks.ims");
		return;
	}

    copiaBlock = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fdb, 0);
	msync(copiaBlock,res,MS_SYNC);
	close(fdb);


}

char* leer_bloque(int nuro_bloque)
{
	char* retorno=malloc(tamanio_bloque+1);

	memcpy(retorno,(copiaBlock+nuro_bloque*tamanio_bloque),tamanio_bloque);
	return retorno;
}

char* leer_resto_bloque(int nuro_bloque,int resto)
{
	char* retorno=malloc(resto+1);

	memcpy(retorno,(copiaBlock+(nuro_bloque*tamanio_bloque)),resto);
	return retorno;
}

char* obtener_bitacora(int id_trip)
{

	char* ruta_bita=string_new();
	string_append(&ruta_bita,punto_montaje);
	string_append(&ruta_bita,"/Files/Bitacoras/Tripulante");
	string_append(&ruta_bita,string_itoa(id_trip));
	string_append(&ruta_bita,".ims");
	t_config* bita= config_create(ruta_bita);
	int zise=config_get_int_value(bita,"SIZE");
	char** bloquecitos=config_get_array_value(bita, "BLOCKS");
	int count_block_bita=config_get_int_value(bita,"BLOCK_COUNT");
	char* bitacora= malloc(zise+1);
	int offset=0;

	for(int auxilio=0; auxilio<count_block_bita ;auxilio++)
	{

		if(zise%tamanio_bloque!=0 && auxilio==(count_block_bita-1))
		{
			pthread_mutex_lock(&mutexEscrituraBloques);

			char* leido=leer_resto_bloque(atoi(bloquecitos[auxilio]),zise%tamanio_bloque);
			pthread_mutex_unlock(&mutexEscrituraBloques);
				leido[(zise%tamanio_bloque)]='\0';
				memcpy(bitacora+offset,leido,(zise%tamanio_bloque)+1);
				//offset+=zise%tamanio_bloque+1;
				free(leido);
		}
		else
		{
			pthread_mutex_lock(&mutexEscrituraBloques);
			 char* leido=leer_bloque(atoi(bloquecitos[auxilio]));
			 pthread_mutex_unlock(&mutexEscrituraBloques);
				leido[tamanio_bloque]='\0';
				memcpy(bitacora+offset,leido,(tamanio_bloque));
				offset+=tamanio_bloque;
				free(leido);

		}



	}
	free(bloquecitos);
	config_destroy(bita);
	return bitacora;
}
void* sincronizar_blocks()
{
	while(correr_programa){
			sleep(tiempoSincro);
			int res=tamanio_bloque*bloques;
			pthread_mutex_lock(&mutexEscrituraBloques);
			int resultadoSincro=msync(copiaBlock,res,MS_SYNC);
			pthread_mutex_unlock(&mutexEscrituraBloques);

			if(resultadoSincro == -1){
				log_error(logger, "Fallo en la sincronizacion con el bloque");
			}
			else
			{
				log_info(logger, "Sincronizacion exitosa con el bloque");
		    }
		}

	return NULL;
}
void borrar_bloque_entero(int bloqe)
{
	int ofset=0;
	int conti=0;
	char* guardar=" ";
	while(conti<tamanio_bloque)
	{
		memcpy(copiaBlock+bloqe*tamanio_bloque+ofset,guardar,sizeof(char));
		conti++;
		ofset++;
	}
}
void borrar_resto_bloque(int bloq, int resto)
{
	int ofset=0;
	int conti=0;
	char* guardar=" ";
	while(conti<resto)
	{
		memcpy(copiaBlock+bloq*tamanio_bloque+ofset,guardar,sizeof(char));
		conti++;
		ofset++;
	}
}
void borrar_parte_bloque(int blok, int cant)
{
	int ofset=1;
	int conti=0;
	char* guardar=" ";
	while(conti<cant)
	{
		memcpy(copiaBlock+((blok+1)*tamanio_bloque)-ofset,guardar,sizeof(char));
		conti++;
		ofset++;
	}
}
int leer_ultimo_bloque(int bloque,char recu)
{
	int retorno=0;
	int ofsat=0;
	char cmp;
	memcpy(&cmp,copiaBlock+(bloque*tamanio_bloque)+ofsat, sizeof(char));
	while(cmp==recu && ofsat<tamanio_bloque)
	{
		retorno++;
		ofsat++;
		memcpy(&cmp,copiaBlock+(bloque*tamanio_bloque)+ofsat, sizeof(char));
	}
	return retorno;
}

void* atender_mensaje (int socketTripulante){
	int respuesta;
	t_paquete* paquete_recibido = recibir_paquete(socketTripulante, &respuesta);
	if (paquete_recibido->codigo_operacion == -1 || respuesta == ERROR) {
		liberar_conexion(socketTripulante);
		eliminar_paquete(paquete_recibido);
	}

//	printf("PAQUETE DE TIPO %d RECIBIDO\n",paquete_recibido->codigo_operacion);

	switch(paquete_recibido->codigo_operacion) {
	case OBTENER_BITACORA:;
		t_pedido_mongo* bitacora = deserializar_pedido_mongo(paquete_recibido);
		log_info(log_mensaje,"Se PIDIO la bitacora del tripulante %d", bitacora->id_tripulante);
		char* devolver=obtener_bitacora((int) bitacora->id_tripulante);
		uint32_t tamanio_bitacora = strlen(devolver)+1;
		send(socketTripulante,&tamanio_bitacora,sizeof(uint32_t),0);
		send(socketTripulante,devolver,tamanio_bitacora,0);
		log_info(log_mensaje,"Se ENVIO la bitacora del tripulante %d", bitacora->id_tripulante);
		free(devolver);

		liberar_t_pedido_mongo(bitacora);
		liberar_conexion(socketTripulante);
		break;
	case MOVIMIENTO_MONGO:;
		t_movimiento_mongo* mov= deserializar_movimiento_mongo(paquete_recibido);
		log_info(log_mensaje,"Se se movio el tripulante %d", mov->id_tripulante);
		char* tiempo= temporal_get_string_time("%H:%M:%S:%MS");
		char* bitacorear=string_new();
		char* posxo=string_itoa((int)mov->origen_x);
		char* posyo=string_itoa((int)mov->origen_y);
		char* posxd=string_itoa((int)mov->destino_x);
		char* posyd=string_itoa((int)mov->destino_y);
		string_append(&bitacorear,tiempo);
		string_append(&bitacorear," Se mueve de ");
		string_append(&bitacorear,posxo);
		string_append(&bitacorear,"|");
		string_append(&bitacorear,posyo);
		string_append(&bitacorear," a ");
		string_append(&bitacorear,posxd);
		string_append(&bitacorear,"|");
		string_append(&bitacorear,posyd);

		//sprintf(bitacorear,"%s Se mueve de %d|%d a %d|%d",tiempo , mov->origen_x,mov->origen_y,mov->destino_x,mov->destino_y);
		//string_append(&tiempo,bitacorear);
		//printf("%s\n",bitacorear);
		//imprimir_movimiento_mongo(mov);
		log_info(log_bitacoras,"voy a escribir rn bitacora %d", mov->id_tripulante);
		escribir_en_bitacora((int) mov->id_tripulante,bitacorear);
		log_info(log_bitacoras,"se escribio bien la bitacora %d", mov->id_tripulante);
		free(bitacorear);
		free(tiempo);
		free(posxo);
		free(posxd);
		free(posyo);
		free(posyd);
		//free(tiempo);
		liberar_t_movimiento_mongo(mov);
		liberar_conexion(socketTripulante);
		break;
	case CONSUMIR_RECURSO:;
		t_consumir_recurso* consu= deserializar_consumir_recurso(paquete_recibido);
		log_info(log_mensaje,"Se pidio modificar el recurso %c", consu->consumible);
		if(consu->tipo=='C'|| consu->tipo == 'D')
		{
			eliminarCaracter((int)consu->cantidad, consu->consumible);
		}
		else
		{
			agregarCaracter((int)consu->cantidad,consu->consumible);
		}
		log_info(log_mensaje,"Se modifico el recurso %c", consu->consumible);
		liberar_t_consumir_recurso(consu);
		liberar_conexion(socketTripulante);
		break;
	case INICIO_TAREA:;
		t_pedido_mongo* inico = deserializar_pedido_mongo(paquete_recibido);
		log_info(logger,"INICIO de tarea del tripulante %d", inico->id_tripulante);
		char* guardar=string_new();
		char* agregar= temporal_get_string_time("%H:%M:%S:%MS");
		string_append(&guardar,agregar);
		string_append(&guardar,"SE_INICIO_LA_TAREA_");
		string_append(&guardar,inico->mensaje);
//		imprimir_pedido_mongo(inico);
		escribir_en_bitacora((int) inico->id_tripulante,guardar);
		free(guardar);
		free(agregar);
		liberar_t_pedido_mongo(inico);

		liberar_conexion(socketTripulante);
		break;
	case FIN_TAREA:;
		t_pedido_mongo* inicio = deserializar_pedido_mongo(paquete_recibido);
		log_info(logger,"FIN de tarea del tripulante %d", inicio->id_tripulante);
		char* agregare = temporal_get_string_time("%H:%M:%S:%MS");
		string_append(&agregare,"SE_FINALIZA_LA_TAREA_");
		string_append(&agregare,inicio->mensaje);
		escribir_en_bitacora((int) inicio->id_tripulante,agregare);
		free(agregare);
		liberar_t_pedido_mongo(inicio);
		liberar_conexion(socketTripulante);
		break;
	case FINALIZAR:;
		eliminar_paquete(paquete_recibido);
		pthread_exit(&sabo);
		pthread_exit(&sincro);
		free(copiaBlock);
		bitarray_destroy(bitmap);
		log_destroy(logger);
		log_destroy(log_bitacoras);
		log_destroy(log_mensaje);
		log_destroy(log_sabotaje);
		free(superbloque);
		correr_programa=false;
		break;
	}

	return NULL;
	}

int string_to_int(char* palabra)
{
	int ret;
	if(strlen(palabra)==3)
	{
		ret= (palabra[0]-'0')*100+(palabra[1]-'0')*10+palabra[2]-'0';
	}
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
int caracteres_en_bloque(int bl)
{
	return 0;
}

void validar_y_arreglar_file(char* rutinni)
{
	t_config* config_o2 = leer_config(rutinni);
	int tamanoSize = config_get_int_value(config_o2, "SIZE");
	char** bloquesOcupados = config_get_array_value(config_o2, "BLOCKS");
	int countBloques = config_get_int_value(config_o2, "BLOCK_COUNT");
	char* caracter =config_get_string_value(config_o2, "CARACTER_LLENADO");
	char* MD=config_get_string_value(config_o2,"MD5");
	int sizeCorrecto=0;
	char character=caracter[0];

	int cantidadReal;
	for(cantidadReal=0; bloquesOcupados[cantidadReal]!=NULL; cantidadReal++);
	if(countBloques!=cantidadReal)
	{
		countBloques=cantidadReal;
		char* bloquesBienOcupados=string_new();
		string_append(&bloquesBienOcupados, "[");

		//Armado del array con los bloques todavia ocupados

			//Se manda la lista con los bloques que quedaron
			for(int j = 0; j < (countBloques); j++){
				if(j==0)
				{
				 string_append(&bloquesBienOcupados, bloquesOcupados[j]);
				}
				else
				{
				string_append(&bloquesBienOcupados, ",");
				string_append(&bloquesBienOcupados, bloquesOcupados[j]);
				}
			}
			string_append(&bloquesBienOcupados, "]");
		actualizar_metadata(bloquesBienOcupados,string_itoa(tamanoSize),string_itoa(countBloques),rutinni,caracter);
		free(bloquesBienOcupados);
	}
	char* valor_md=string_new();
	for(int m=0;bloquesOcupados[m]!=NULL;m++)
	{
		string_append(&valor_md, bloquesOcupados[m]);
	}

	FILE* mf=fopen("/home/utnso/md5.txt","w+");
	char* md= string_new();
	string_append(&md,"echo -n " );
	string_append(&md,valor_md);
	string_append(&md," |md5sum");
	string_append(&md," > " );
	string_append(&md,"/home/utnso/md5.txt");
	system(md);
	char* mfive=malloc(33);
	mfive=fgets(mfive,33,mf);
	fclose(mf);
	if(!string_equals_ignore_case(MD,mfive))
	{
		char* ultimo_bloque=string_new();
		int bloque_ultimo;
		for(int c=0; bloquesOcupados[c]!=NULL;c++)
		{
			if(leer_ultimo_bloque(atoi(bloquesOcupados[c]),caracter[0])<tamanio_bloque)
			{
				ultimo_bloque=bloquesOcupados[c];
				bloque_ultimo=c;
			}
		}
//		bloquesOcupados[bloque_ultimo]=bloquesOcupados[countBloques-1];
//		bloquesOcupados[countBloques-1]=ultimo_bloque;
		char* bloquesBienOcupados=string_new();
		string_append(&bloquesBienOcupados, "[");

		//Armado del array con los bloques todavia ocupados

			//Se manda la lista con los bloques que quedaron
			for(int j = 0; j < (countBloques); j++){
				if(j==0)
				{
					if(j==bloque_ultimo)
					{
						string_append(&bloquesBienOcupados, bloquesOcupados[countBloques-1]);
					}
					else
					{
						string_append(&bloquesBienOcupados, bloquesOcupados[j]);
					}
				}
				else
				{
					string_append(&bloquesBienOcupados, ",");
					if(j==countBloques-1)
					{
						string_append(&bloquesBienOcupados, ultimo_bloque);
					}
					else {
						string_append(&bloquesBienOcupados, bloquesOcupados[j]);
					}
				}
			}
			string_append(&bloquesBienOcupados, "]");
			actualizar_metadata(bloquesBienOcupados,string_itoa(tamanoSize),string_itoa(countBloques),rutinni,caracter);
			char** bloquesbien=string_get_string_as_array(bloquesBienOcupados);
			free(bloquesBienOcupados);//si rompe al solucionar sabotaje es por estoooooooooo
			bloquesOcupados=bloquesbien;

	}
	if(countBloques>0)
	{
	int ultimo_bloque=atoi(bloquesOcupados[countBloques-1]);


	int cant_ultimo=leer_ultimo_bloque(ultimo_bloque,character);
	sizeCorrecto=(countBloques-1)*tamanio_bloque+cant_ultimo;
	}
	if(tamanoSize!=sizeCorrecto)
	{
		tamanoSize=sizeCorrecto;
		char* bloquesBienOcupados=string_new();
		string_append(&bloquesBienOcupados, "[");

		//Armado del array con los bloques todavia ocupados

			//Se manda la lista con los bloques que quedaron
			for(int j = 0; j < (countBloques); j++){
				if(j==0)
				{
				 string_append(&bloquesBienOcupados, bloquesOcupados[j]);
				}
				else
				{
				string_append(&bloquesBienOcupados, ",");
				string_append(&bloquesBienOcupados, bloquesOcupados[j]);
				}
			}
			string_append(&bloquesBienOcupados, "]");
		actualizar_metadata(bloquesBienOcupados,string_itoa(tamanoSize),string_itoa(countBloques),rutinni,caracter);
		free(bloquesBienOcupados);
	}
	free(mfive);
	free(md);
	free(valor_md);
	free(bloquesOcupados);;
	free(MD);
	free(caracter);
}
void agregar_a_lista(char*ruta,t_list* blocks_used)
{
	t_config* obtener_blocks=leer_config(ruta);
	char** bloquecitos=config_get_array_value(obtener_blocks,"BLOCKS");
	for(int bloq=0;bloquecitos[bloq]!=NULL;bloq++)
	{
		list_add(blocks_used,(int)atoi(bloquecitos[bloq]));
	}
}
void agregar_blocks_bitacoras(t_list* blocks_used)
{
	char* ruta_bita=string_new();
	string_append(&ruta_bita,punto_montaje);
	string_append(&ruta_bita,"/Files/Bitacoras/Tripulante");
	int tripulante=1;
	string_append(&ruta_bita,string_itoa(tripulante));
	string_append(&ruta_bita,".ims");

	while(tripulante <= 10)
	{
		if(verificar_existencia(ruta_bita)== 1)
		{
		agregar_a_lista(ruta_bita,blocks_used);
		tripulante++;

		}
		else{
			tripulante++;
		}
		ruta_bita=string_substring_until(ruta_bita,strlen(punto_montaje)+strlen("/Files/Bitacoras/Tripulante"));
		string_append(&ruta_bita,string_itoa(tripulante));
		string_append(&ruta_bita,".ims");
	}
	free(ruta_bita);
}
void agregar_blocks_recursos(t_list* blocks_used)
{
	char* ruta_oxigeno=string_new();
	string_append(&ruta_oxigeno,punto_montaje);
	string_append(&ruta_oxigeno,"/Files/Oxigeno.ims");
	agregar_a_lista(ruta_oxigeno,blocks_used);
	char* ruta_basura=string_new();
	string_append(&ruta_basura,punto_montaje);
	string_append(&ruta_basura,"/Files/Basura.ims");
	agregar_a_lista(ruta_basura,blocks_used);
	char* ruta_comida=string_new();
	string_append(&ruta_comida,punto_montaje);
	string_append(&ruta_comida,"/Files/Comida.ims");
	agregar_a_lista(ruta_comida,blocks_used);
	free(ruta_oxigeno);
	free(ruta_basura);
	free(ruta_comida);
}

_Bool esElMsmoBit(int t,  int b)
{
	return t==b;
}
void* minimum(void*y,void*x)
{
	if(x<y)
	{
		return x;
	}
	else {
		return y;
	}
}
void arreglar_bitMap()
{
	t_list* blocks_used=list_create();
	agregar_blocks_bitacoras(blocks_used);
	agregar_blocks_recursos(blocks_used);
	int bit_a_comparar;
	_Bool mismoBit(void* elemento)
	{
		return esElMsmoBit(bit_a_comparar,(int)elemento);
	}
	for(int bit=0; bit<bloques;bit++)
	{
		bit_a_comparar=bit;
		if(list_size(blocks_used)!=0 && bit==0 && (int)list_get_minimum(blocks_used,minimum)==0 )
		{
			bitarray_set_bit(bitmap,bit);
		}
		else
		{

			if(list_size(blocks_used)!=0 && list_find(blocks_used,mismoBit)!=0  )
			{
				bitarray_set_bit(bitmap,bit);
			}
			else
			{
				bitarray_clean_bit(bitmap,bit);
			}

		}


	}
	msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);
	msync(superbloque, 2*sizeof(uint32_t)+tamanioBitmap, MS_SYNC);

	list_destroy(blocks_used);
}

void arreglar_blocks()
{
	uint32_t bloquetotal;
	memcpy(&bloquetotal,superbloque+sizeof(uint32_t),sizeof(uint32_t));
	if(bloquetotal!=bloques)
	{
		memcpy(superbloque+sizeof(uint32_t),&bloques,sizeof(uint32_t));
		msync(superbloque, 2*sizeof(uint32_t)+tamanioBitmap, MS_SYNC);
	}
}

void arreglar_sabotaje(void)
{
	arreglar_blocks();
	char* ruta_oxigeno=string_new();
	string_append(&ruta_oxigeno,punto_montaje);
	string_append(&ruta_oxigeno,"/Files/Oxigeno.ims");
	validar_y_arreglar_file(ruta_oxigeno);
	char* ruta_basura=string_new();
	string_append(&ruta_basura,punto_montaje);
	string_append(&ruta_basura,"/Files/Basura.ims");
	validar_y_arreglar_file(ruta_basura);
	char* ruta_comida=string_new();
	string_append(&ruta_comida,punto_montaje);
	string_append(&ruta_comida,"/Files/Comida.ims");
	validar_y_arreglar_file(ruta_comida);
	arreglar_bitMap();

	free(ruta_oxigeno);
	free(ruta_basura);
	free(ruta_comida);
}
void interrupt_handler(int signal)
{
	log_info(log_sabotaje,"Se capto la señal del Sabotaje");
	/*
	 * aca tiramos un conectar al discordiador
	 * acale mandamos la posicion del sabotaje
	 * esperamos el mensaje de protocolo fsk
	 *
	 * */
	char** pocicion_sabotaje=config_get_array_value(mongoStore_config,"POSICIONES_SABOTAJE");
	//char* a mandar al discordaidor
	char* posicion_mandar=pocicion_sabotaje[sabotaje_actual];

	int socketCliente = crear_conexion(ipDiscordiador,puertoDicordiador);
	t_pedido_mongo* posSabo=malloc(sizeof(t_pedido_mongo));
	posSabo->mensaje=posicion_mandar;
	posSabo->tamanio_mensaje=strlen(posicion_mandar)+1;

	t_paquete* paquete_enviar= crear_paquete(SABOTAJE);
	agregar_paquete_pedido_mongo(paquete_enviar,posSabo);
	char* bitacorear_sabo= enviar_paquete_respuesta_string(paquete_enviar,socketCliente);
	int id;
	recv(socketCliente,&id,sizeof(uint8_t),0);
	log_info(log_mensaje,"recibi un %s",bitacorear_sabo);
	log_info(log_mensaje,"recibi un id %d",id);
	log_info(log_sabotaje,"tipulante ejecuta protocolo fsck %d", id);
	escribir_en_bitacora(id,bitacorear_sabo);
	free(bitacorear_sabo);
	// aca arregla el sabotaje
	log_info(log_sabotaje,"Procedo a arreglar el FileSystem");
	arreglar_sabotaje();
	log_info(log_sabotaje,"Se arreglo exitosamente el Sabotaje");

	// aca tendria que mandar discordiador que se arreglo
	int tamanio_fin_sabotaje;
	recv(socketCliente,&tamanio_fin_sabotaje,sizeof(uint32_t),0);
	char* finalizar_sabotaje = malloc(tamanio_fin_sabotaje);
	recv(socketCliente,finalizar_sabotaje,tamanio_fin_sabotaje,0);
	log_info(log_mensaje,"recibi un %s",finalizar_sabotaje);
	escribir_en_bitacora(id,finalizar_sabotaje);
	free(finalizar_sabotaje);

	sabotaje_actual++;
	free(pocicion_sabotaje);

}



void inicializar_carpetas(){
	//Se inicializan las carpetas FILES y BITACORAS
	nueva_carpeta("/Files");
	log_info(logger, "Se creo la carpeta de Files");

	nueva_carpeta("/Files/Bitacoras");
	log_info(logger ,"Se creo la capeta de las bitacoras");

	nueva_carpeta("/Superbloque");
	log_info(logger, "Se creo la carpeta del Superbloque");

	nueva_carpeta("/Blocks");
	log_info(logger, "Se creo la carpeta de los bloques");
}

void nueva_carpeta(char *nueva_carpeta){
	char *ruta_general = string_new();
	string_append(&ruta_general, punto_montaje);
	string_append(&ruta_general, nueva_carpeta);
	mkdir(ruta_general, 0777);
	free(ruta_general);
	//mkdir(): Genera un nuevo directorio con ese path.
	//El primer parametro es la ruta general, el segundo es el modo en el que
	//se crea
}

void crear_superbloque(){
	char* path_superbloque = string_new();

	string_append(&path_superbloque, punto_montaje);
	string_append(&path_superbloque, "/Superbloque/Superbloque.ims");

	//Creamos superbloque
	int fd = open(path_superbloque, O_CREAT | O_RDWR, 0664);

	if (fd == -1) {
		close(fd);
		log_error(logger, "Error abriendo el Superbloque.ims");
		return;
	}
	if(bloques%8==0){
		tamanioBitmap = bloques/8;
	}
	else {
		tamanioBitmap =( bloques/8)+1; //AGREGAR ceil()
	}
	int res = 4 + 4 + tamanioBitmap; //Tamanio a truncar el superbloque
	ftruncate(fd, res);

	//Creas el superbloque
	superbloque = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//Vas llenando la info del superbloque del superbloque
	void* tipo_variables = malloc(4); //Son los uint32 que vas metiendo en el superbloque
	tipo_variables = realloc(tipo_variables, sizeof(int));

	memcpy(tipo_variables, &tamanio_bloque, sizeof(uint32_t));
	memcpy(superbloque, (tipo_variables), sizeof(uint32_t));

	memcpy(tipo_variables, &bloques, sizeof(uint32_t));
	memcpy(superbloque + sizeof(uint32_t), tipo_variables, sizeof(uint32_t));

	//CREO EL BITMAP
	bitmap = bitarray_create_with_mode((void*) superbloque+2*sizeof(uint32_t), tamanioBitmap, MSB_FIRST);
	msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);

	//Sincronizas
	msync(superbloque, res, MS_SYNC);

	//Liberas la path
	free(path_superbloque);
	log_info(logger, "Se creo el archivo superbloque!");
	return;
}

void inicializar_bloques(){
	//CADA UN BYTE, UN CARACTER!!!
	char* ruta_blocks = string_new();
	string_append(&ruta_blocks, punto_montaje);
	string_append(&ruta_blocks, "/Blocks/Blocks.ims");

	int res = tamanio_bloque * bloques;

	copiaBlock = malloc(res);

	int fd = open(ruta_blocks, O_CREAT | O_RDWR, 0664);

	if (fd == -1) {
		close(fd);
		log_error(logger, "Error abriendo el Blocks.ims");
		return;
	}

	ftruncate(fd, res);

	copiaBlock = mmap(NULL, res, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	//memcpy(copiaBlock, block, res); //ACA ESTA EL PROBLEMA

//SINCRONIZACION ==> Chequear!
//	while(1){
//		sleep(tiempoSincro);
//		pthread_mutex_lock(&mutexEscrituraBloques);
//		memcpy(block,copiaBlock,res);
//		pthread_mutex_unlock(&mutexEscrituraBloques);
//
//		int resultadoSincro = msync(block, res, MS_SYNC);
//
//		if(resultadoSincro == -1){
//			log_error(logger, "Fallo en la sincronizacion con el bloque");
//		}
//		else
//		{
//			log_error(logger, "Sincronizacion exitosa con el bloque");
//		}
//	}

	close(fd);
	free(ruta_blocks);
	return;
}

void eliminarCaracter(int cantidad, char caracter){
	//SE INVOCA PARA CONSUMIR UN RECURSO
	char* rutita = string_new();
	string_append(&rutita, punto_montaje);
	pthread_mutex_lock(&mutexBitacoras);

	switch (caracter){
		case 'O':
			string_append(&rutita, "/Files/Oxigeno.ims");
			eliminarEnBloque(cantidad, caracter, rutita);
			break;

		case 'o':
			string_append(&rutita, "/Files/Oxigeno.ims");
			eliminarEnBloque(cantidad, 'O', rutita);
			break;

		case 'B':
			string_append(&rutita, "/Files/Basura.ims");
			remove(rutita);
			break;

		case 'b':
			string_append(&rutita, "/Files/Basura.ims");
			remove(rutita);
			break;

		case 'C':
			string_append(&rutita, "/Files/Comida.ims");
			eliminarEnBloque(cantidad, caracter, rutita);
			break;

		case 'c':
			string_append(&rutita, "/Files/Comida.ims");
			eliminarEnBloque(cantidad, 'C', rutita);
			break;

		default:
			printf("No se selecciono un caracter correcto");
			break;
	}
	pthread_mutex_unlock(&mutexBitacoras);
}


void eliminarEnBloque(int cantidad, char caracter, char* rutita){
	//Misma logica para cuando escribimos en el bloque

	bloquesDelSistema = bloques;

	//Se llama config_o2 porque originalmente estaba para Oxigeno.ims, pero ahora es global (el nombre no importa)
	//Para obtener la data directamente del metadata, hacemos:
	FILE* config=fopen(rutita,"r");
	int cantidadDeCaracteresRestantes;
	char** bloquesUsados;
	char* caracterLlenado = string_new();
	int cantBloques;
	for(int sep=0; sep<5; sep++)
	{
		char* var=malloc(250);
		fgets(var,250,config);
		if(string_contains(var,"SIZE"))
		{
			char**size=string_split(var,"=");
			char* correc=string_substring_until(size[1],strlen(size[1])-1);
			cantidadDeCaracteresRestantes=atoi(correc);
			free(size[0]);
			free(size[1]);
			free(size);
			free(correc);

		}
		if(string_contains(var,"BLOCKS"))
		{
			char** blocks=string_split(var,"=");
			char* correc=string_substring_until(blocks[1],strlen(blocks[1])-1);
			bloquesUsados=string_get_string_as_array(correc);
			free(blocks[0]);
			free(blocks[1]);
			free(blocks);
			free(correc);

		}
		if(string_contains(var,"BLOCK_COUNT"))
		{
			char** contadorb=string_split(var,"=");
			char* correc=string_substring_until(contadorb[1],strlen(contadorb[1])-1);
			cantBloques=atoi(correc);
			free(contadorb[0]);
			free(contadorb[1]);
			free(contadorb);
			free(correc);
		}
		if(string_contains(var,"CARACTER_LLENADO"))
		{
			char** character=string_split(var,"=");
			char* correc=string_substring_until(character[1],strlen(character[1])-1);
			caracterLlenado=strdup(correc);
			free(character[0]);
			free(character[1]);
			free(character);
			free(correc);

		}
		free(var);
	}
	fclose(config);

	char* bloquesNuevosPostBorrado = string_new();
	string_append(&bloquesNuevosPostBorrado, "[");

	//Si es 0, sugnifica que el caracter no tiene bloques asignados en el Blocks.ims
	//Por ende, tenes que buscar el proximo bloque libre ==> no hay bloques en uso

	if(cantBloques <= 0){
		log_error(logger, "El recurso todavia no se genero!");
		printf("No se genero el recurso!");
		return;

		//Si tiene 0 bloques ==> No esta ocupando ningun bloque ==> No tiene sentido consumir algo que no existe
	}

	/*
	 * YO HARIA ESTO
	 * Mira si el ultimo bloque no esta completo entero le resto esos caracteres ahora bien si lo que tengo que llenar no me
	 * elimina todo el bloque (es decir que si yo hago caracteres a borrar- los caracteres del bloque me da negativo) no hago nada
	 * sino limpio el bloque del bitmap y decremento la cantidad de bloques  y me fijo restando el tamaño del bloque si tengo que restar mas
	 * la parte del seteo todo en el config te lo dejo a vos
	 *
	 **/
	int cantidad_a_borrar=cantidad;
	if (cantidad >= cantidadDeCaracteresRestantes)
	{
		cantidad_a_borrar=cantidadDeCaracteresRestantes;
	}
	if(cantidadDeCaracteresRestantes%tamanio_bloque!=0)
	{
		cantidad_a_borrar-=cantidadDeCaracteresRestantes%tamanio_bloque;
		while(cantidad_a_borrar >= 0 && cantBloques>0)
		{
			int bloque_a_eliminar=atoi(bloquesUsados[cantBloques-1]);
			bitarray_clean_bit(bitmap,bloque_a_eliminar);
			msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);
			pthread_mutex_lock(&mutexEscrituraBloques);
			borrar_resto_bloque(bloque_a_eliminar,cantidadDeCaracteresRestantes%tamanio_bloque);
			pthread_mutex_unlock(&mutexEscrituraBloques);
			cantBloques=cantBloques-1;
			cantidad_a_borrar=cantidad_a_borrar-tamanio_bloque;
		}
	}
	else
	{
		cantidad_a_borrar-=tamanio_bloque;
		while(cantidad_a_borrar >= 0 && cantBloques>0)
		{
			int bloque_a_eliminar=atoi(bloquesUsados[cantBloques-1]);
			bitarray_clean_bit(bitmap,bloque_a_eliminar);
			msync(bitmap -> bitarray, tamanioBitmap, MS_SYNC);
			pthread_mutex_lock(&mutexEscrituraBloques);
			borrar_bloque_entero(bloque_a_eliminar);
			pthread_mutex_unlock(&mutexEscrituraBloques);
			cantBloques--;
			cantidad_a_borrar-=tamanio_bloque;
		}
	}
	if(cantidad_a_borrar+tamanio_bloque>0)
	{
		pthread_mutex_lock(&mutexEscrituraBloques);
		borrar_parte_bloque(atoi(bloquesUsados[cantBloques-1]),(cantidad_a_borrar+tamanio_bloque));
		pthread_mutex_unlock(&mutexEscrituraBloques);
	}


	//Se manda la lista con los bloques que quedaron
	for(int j = 0; j < cantBloques ; j++){
		if(j == 0){
			string_append(&bloquesNuevosPostBorrado, bloquesUsados[j]);
		}
		else
		{
			string_append(&bloquesNuevosPostBorrado, ",");
			string_append(&bloquesNuevosPostBorrado, bloquesUsados[j]);
		}
	}
	string_append(&bloquesNuevosPostBorrado, "]");


	char* actualizarCantidad=string_new();
	char* actualizarSize=string_new();
	cantidadDeCaracteresRestantes = cantidadDeCaracteresRestantes-cantidad;
	//Actualizas el metadata
	if(cantidadDeCaracteresRestantes>0)
	{
		char* cantdBloques=string_itoa(cantBloques);
		char* cantidaddecaracteres=string_itoa(cantidadDeCaracteresRestantes);
		string_append(&actualizarCantidad,cantdBloques);
		string_append(&actualizarSize,cantidaddecaracteres);
		free(cantdBloques);
		free(cantidaddecaracteres);

	}
	else
	{
		string_append(&actualizarCantidad,"0");
		string_append(&actualizarSize,"0");
	}


	actualizar_metadata(bloquesNuevosPostBorrado, actualizarSize, actualizarCantidad, rutita, caracterLlenado);

	free(bloquesUsados);
	free(bloquesNuevosPostBorrado);
	free(actualizarCantidad);
	free(caracterLlenado);
	free(actualizarSize);

	//log_info(logger, "Ya se consumieron todos los recursos posibles");
}


int existeEnArray(char** array, char contenido){
	int existe = 0;
	for(int i = 0; i < sizeof(array); i++){
		if(array[i] == string_itoa(contenido)){
			existe = 1;
		}
	}
	return existe;
}

void escribirEnBloque(int cantidad, char caracter, char* rutita){
	bloquesDelSistema = bloques;

	//Se llama config_o2 porque originalmente estaba para Oxigeno.ims, pero ahora es global (el nombre no importa)
	//Para obtener la data directamente del metadata, hacemos:s
	FILE* config=fopen(rutita,"r");
	int cantidadDeCaracteresEscritas;
	char** bloquesUsados;
	char* caracterLlenado = string_new();
	int cantBloques;
	int iterar=3;
	if(esMetadataRecurso(rutita))
		iterar=5;
	for(int sep=0; sep<iterar; sep++)
	{
		char* var=malloc(250);
		fgets(var,250,config);
		if(string_contains(var,"SIZE"))
		{
			char**size=string_split(var,"=");
			char* correc=string_substring_until(size[1],strlen(size[1])-1);
			cantidadDeCaracteresEscritas=atoi(correc);
			free(size[0]);
			free(size[1]);
			free(size);
			free(correc);

		}
		if(string_contains(var,"BLOCKS"))
		{
			char** blocks=string_split(var,"=");
			char* correc=string_substring_until(blocks[1],strlen(blocks[1])-1);
			bloquesUsados=string_get_string_as_array(correc);
			free(blocks[0]);
			free(blocks[1]);
			free(blocks);
			free(correc);

		}
		if(string_contains(var,"BLOCK_COUNT"))
		{
			char** contadorb=string_split(var,"=");
			char* correc=string_substring_until(contadorb[1],strlen(contadorb[1])-1);
			cantBloques=atoi(correc);
			free(contadorb[0]);
			free(contadorb[1]);
			free(contadorb);
			free(correc);
		}
		if(string_contains(var,"CARACTER_LLENADO"))
		{
			char** character=string_split(var,"=");
			char* correc=string_substring_until(character[1],strlen(character[1])-1);
			caracterLlenado=strdup(correc);
			free(character[0]);
			free(character[1]);
			free(character);
			free(correc);

		}
		free(var);
	}
	fclose(config);






	//Cantidad de caracteres escritos
	int cantidadEscrita = 0;
	//El bloque donde vas a escribir
	int bloqueAUsar;
	//La cantidad de bloques nuevos asociados al caracter
	int cantBloquesActualizacion = cantBloques;

	//Para hacer un nuevo string con los bloques nuevos del caracter
	char* actualizar_bloques = string_new();

	//Si es 0, sugnifica que el caracter no tiene bloques asignados en el Blocks.ims
	//Por ende, tenes que buscar el proximo bloque libre ==> no hay bloques en uso
	if(cantBloques == 0){
		for(int i = 0; i < bloquesDelSistema && cantidad>0; i++){
			if((bitarray_test_bit(bitmap, i) == 0) && cantidad > 0){ //Pregunta si el bloque esta libre (0 == libre)

				bloqueAUsar = i;
				cantBloquesActualizacion++; //Suma la cantidad de bloques ocupado por el caracter/recurso
				char* bloque_nuevo = string_itoa(i); //Para agregarlo en el metadata con los bloques que usas
				cantidadEscrita = 0;

				if(cantBloquesActualizacion==1){ //Empezas a armar el string que contiene la lista de los bloques NUEVOS!
					string_append(&actualizar_bloques, bloque_nuevo);	//Ej: [1,3,4] ==> Son los bloques
				}
				else	//Los agregas con la coma
				{
					string_append(&actualizar_bloques, ",");
					string_append(&actualizar_bloques, bloque_nuevo);
				}
				//Agregas un bloque por ciclo de for!

				bitarray_set_bit(bitmap, i); //Seteas que el bloque esta ocupado
				msync(&bitmap -> bitarray, tamanioBitmap, MS_SYNC);

				//Empezas a escribir en el bitmap hasta llenar ese bloque o hasta que no tengas mas caracteres
				//Se escribe un caracter por ciclo de while!
				while(cantidad > 0 && cantidadEscrita < tamanio_bloque){

					//ESCRIBO EN EL BLOQUE DEL BITMAP
					pthread_mutex_lock(&mutexEscrituraBloques);
					memcpy(copiaBlock + bloqueAUsar * tamanio_bloque + (cantidadDeCaracteresEscritas % tamanio_bloque), &caracter, sizeof(char));
					pthread_mutex_unlock(&mutexEscrituraBloques);

					//Sumas la cantida de caracteres escritos, disminuis la cantidad restante de caracteres
					//a escribir y aumentas el size futuro del metadata (que es cant de caracteres escritos)
					//Un byte por caracter
					cantidadEscrita++;
					cantidad--;
					cantidadDeCaracteresEscritas++;

				}
				free(bloque_nuevo);
			}
		}
	}
	else
	{
		//Si entras al else, ya existe un bloque en uso, entonces terminas de llenar ese bloque y vas a otro
		bloqueAUsar = atoi(bloquesUsados[cantBloques -1]);//atoi ==> convierte un array a int ==> Agarras el ultimo bloque que llega del metadata y lo transformas a int
		//Con esto basicamente accedes al bloque directamente

		// Este for (el de int j) lo que hace es escribir la cantidad de letras hasta llenar ese bloque, si lo llena y le falta
		// caracteres, va al otro for (el de int i) a buscar el proximo bloque libre para seguir llenando
		for(int j = 0; j < cantidad; j++){

			//Con este if, lo que pregunto es que si el bloque esta lleno o no
			if(cantidadDeCaracteresEscritas % tamanio_bloque == 0){

				for(int i = 0; i < bloquesDelSistema; i++){
					if(bitarray_test_bit(bitmap, i) == 0){
						bloqueAUsar = i;
						char* bloqueNuevo = string_itoa(i);
						string_append(&actualizar_bloques, ",");
						string_append(&actualizar_bloques, bloqueNuevo); //Agrega el bloque seguido de una "," al array a
						//reemplazar el que esta en el metadata
						cantBloquesActualizacion++;
						bitarray_set_bit(bitmap, i);
						msync(&bitmap -> bitarray, tamanioBitmap, MS_SYNC);
						i = bloquesDelSistema;
						free(bloqueNuevo);
					}
				}
			}
		pthread_mutex_lock(&mutexEscrituraBloques);
			memcpy(copiaBlock + (bloqueAUsar * tamanio_bloque) + (cantidadDeCaracteresEscritas % tamanio_bloque), &caracter, sizeof(char));
			pthread_mutex_unlock(&mutexEscrituraBloques);
			cantidadDeCaracteresEscritas++;

		}
	}
		//PROCEDEMOS A ACTUALIZAR EL METADATA CORRESPONDIENTE
		char* actualizarCantidad = string_itoa(cantBloquesActualizacion);
		char* actualizarBloques = string_new();
		char* actualizarSize = string_itoa(cantidadDeCaracteresEscritas);

		//Empiezo a generar el array que contiene los nuevos bloques actualizados
		string_append(&actualizarBloques, "[");

		for(int i = 0; i < cantBloques; i++){
			if(i==0){
				string_append(&actualizarBloques, bloquesUsados[i]);
			}
			else
			{
				string_append(&actualizarBloques, ",");
				string_append(&actualizarBloques, bloquesUsados[i]);
			}
		}

		string_append(&actualizarBloques, actualizar_bloques);
		string_append(&actualizarBloques, "]");

		//Actualizamos metadata o la bitacora
		if(esMetadataRecurso(rutita)){
			actualizar_metadata(actualizarBloques, actualizarSize, actualizarCantidad, rutita, caracterLlenado);
		}
		else
		{
			actualizar_bitacora(actualizarBloques, actualizarSize, actualizarCantidad, rutita);
		}
		free(bloquesUsados);
		free(caracterLlenado);
		free(actualizarCantidad);
		free(actualizarBloques);
		free(actualizarSize);
		free(actualizar_bloques);

}


void generar_bitacora(int idTripulante){

	char* id_trip = string_itoa(idTripulante);
	char* ruta_bitacora = string_new();
	string_append(&ruta_bitacora, punto_montaje);
	string_append(&ruta_bitacora, "/Files/Bitacoras/Tripulante");
	string_append(&ruta_bitacora, id_trip);
	string_append(&ruta_bitacora, ".ims");

//	FILE* metadata_fd = fopen(ruta_metadata, "rb");

	if (verificar_existencia(ruta_bitacora) == 1) {
		//fclose(metadata_fd);
		printf("Existe esa bitacora!");
		log_info(logger, "Bitacora encontrada");
		return;
	}

//	metadata_fd = fopen(ruta_metadata, "w");
	t_config* bitacora_config = malloc(sizeof(t_config));
	bitacora_config->properties=dictionary_create();
	bitacora_config->path=ruta_bitacora;
	dictionary_put(bitacora_config->properties, "SIZE", "0");
	dictionary_put(bitacora_config->properties, "BLOCK_COUNT","0");
	dictionary_put(bitacora_config->properties, "BLOCKS", "[]");

	config_save(bitacora_config);
	free(id_trip);
	dictionary_destroy(bitacora_config->properties);
	free(ruta_bitacora);

}


void escribir_en_bitacora(int idTripulante, char* texto){
	char* id_trip = string_itoa(idTripulante);
	char* ruta_bitacora = string_new();
	string_append(&ruta_bitacora, punto_montaje);
	string_append(&ruta_bitacora, "/Files/Bitacoras/Tripulante");
	string_append(&ruta_bitacora, id_trip);
	string_append(&ruta_bitacora, ".ims");
	log_info(log_bitacoras, "se qiere bitacorear %s en la bitiacora del tripulante %d", texto,idTripulante);
	pthread_mutex_lock(&mutexBitacoras);
	if(verificar_existencia(ruta_bitacora) != 1)
	{
		generar_bitacora(idTripulante);
		//char*log=string_new();

		log_info(logger, "Se creo exitosamente la bitacora %d", idTripulante);
//		free(log);
	}
		int longitud = strlen(texto);
		//Escribe en la bitacora!
		for (int i = 0; i < longitud; i++)
		{
			escribirEnBloque(1, texto[i], ruta_bitacora);
		}

		escribirEnBloque(1, '\n', ruta_bitacora);
		free(id_trip);
		free(ruta_bitacora);
		pthread_mutex_unlock(&mutexBitacoras);
}


void crear_archivo_files(){
	crear_metadata("Oxigeno", "O");
	crear_metadata("Comida", "C");
	crear_metadata("Basura", "B");
}

void crear_metadata(char* archivo, char* valor){
	char* ruta_metadata = string_new();
	string_append(&ruta_metadata, punto_montaje);
	string_append(&ruta_metadata, "/Files/");
	string_append(&ruta_metadata, archivo);
	string_append(&ruta_metadata, ".ims");

	FILE* metadata_fd = fopen(ruta_metadata, "rb");

	if (verificar_existencia(ruta_metadata) == 1) {
		fclose(metadata_fd);
		printf("Esa metadata ya existe!");
		log_info(logger, "Metadata encontrada");
		return;
	}

	metadata_fd = fopen(ruta_metadata, "w+");
	char* md= string_new();
	string_append(&md,"echo -n " );
	string_append(&md,"");
	string_append(&md," |md5sum");
	string_append(&md," > " );
	string_append(&md,ruta_metadata);
	system(md);
	char* mfive=malloc(33);
	mfive=fgets(mfive,33,metadata_fd);
	t_config* metadata_config = malloc(sizeof(t_config));
	metadata_config->properties=dictionary_create();
	metadata_config->path=ruta_metadata;

	dictionary_put(metadata_config->properties, "SIZE", "0");
	dictionary_put(metadata_config->properties, "BLOCK_COUNT", "0");
	dictionary_put(metadata_config->properties, "BLOCKS", "[]");
	dictionary_put(metadata_config->properties, "CARACTER_LLENADO", valor);
	dictionary_put(metadata_config->properties, "MD5", mfive);
	//FALTA MD5!

	config_save(metadata_config);
	fclose(metadata_fd);
	free(mfive);
//	config_destroy(metadata_config);
	dictionary_destroy(metadata_config->properties);
	free(metadata_config);
	free(ruta_metadata);
	free(md);
}

void actualizar_metadata(char* valorBlocks, char* valorSize, char* valorBlockCount, char* ruta, char* caracter){
	t_config* metadata_config=malloc(sizeof(t_config));
	metadata_config->properties=dictionary_create();
	dictionary_put(metadata_config->properties, "SIZE", valorSize);
	dictionary_put(metadata_config->properties, "BLOCK_COUNT", valorBlockCount);
	dictionary_put(metadata_config->properties, "BLOCKS", valorBlocks);
	dictionary_put(metadata_config->properties,"CARACTER_LLENADO", caracter);
	char** calculo_md=string_get_string_as_array(valorBlocks);
	char* valor_md=string_new();
	for(int m=0;calculo_md[m]!=NULL;m++)
	{
		string_append(&valor_md, calculo_md[m]);
	}
	FILE* metadata_fd=fopen(ruta,"w+");
	char* md= string_new();
	string_append(&md,"echo -n " );
	string_append(&md,valor_md);
	string_append(&md," |md5sum");
	string_append(&md," > " );
	string_append(&md,ruta);
	system(md);
	char* mfive=malloc(33);
	mfive=fgets(mfive,33,metadata_fd);
	fclose(metadata_fd);
	dictionary_put(metadata_config->properties, "MD5",mfive );
	metadata_config->path=ruta;
	config_save(metadata_config);
	free(mfive);
	dictionary_destroy(metadata_config->properties);
	free(metadata_config->path);
	free(metadata_config);
	free(calculo_md);
	free(valor_md);
	free(md);

}

void actualizar_bitacora(char* valorBlocks, char* valorSize, char* valorBlockCount, char* ruta){
//	FILE* bita=fopen(ruta,"w");
	t_config* bitacora_config = malloc(sizeof(t_config));
	bitacora_config->path=ruta;
	bitacora_config->properties=dictionary_create();
	dictionary_put(bitacora_config->properties, "SIZE", valorSize);
	dictionary_put(bitacora_config->properties, "BLOCK_COUNT", valorBlockCount);
	dictionary_put(bitacora_config->properties, "BLOCKS", valorBlocks);

	config_save(bitacora_config);
//	fclose(bita);
	//free(bitacora_config->path);
	dictionary_destroy(bitacora_config->properties);
	free(bitacora_config);

}

int verificar_existencia(char* nombre_archivo){
	FILE* file;
	int retorno = 0;

	if((file = fopen(nombre_archivo, "r"))){
		retorno = 1;
		fclose(file);
	}
	return retorno;
}

void agregarCaracter(int cantidad, char caracter){
	//ESTA FUNCION ES INVOCADA CUANDO GENERAMOS UN RECURSO

	char* rutita = string_new();
	string_append(&rutita, punto_montaje);
	pthread_mutex_lock(&mutexBitacoras);
	switch (caracter){
		case 'O':
			string_append(&rutita, "/Files/Oxigeno.ims");
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'o':
			string_append(&rutita, "/Files/Oxigeno.ims");
			escribirEnBloque(cantidad, 'O', rutita);
			break;

		case 'B':
			string_append(&rutita, "/Files/Basura.ims");
			if(verificar_existencia(rutita)!=1)
			{
				crear_metadata("Basura", "B");
			}
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'b':
			string_append(&rutita, "/Files/Basura.ims");
			if(verificar_existencia(rutita)!=1)
			{
				crear_metadata("Basura", "B");
			}
			escribirEnBloque(cantidad, 'B', rutita);
			break;

		case 'C':
			string_append(&rutita, "/Files/Comida.ims");
			escribirEnBloque(cantidad, caracter, rutita);
			break;

		case 'c':
			string_append(&rutita, "/Files/Comida.ims");
			escribirEnBloque(cantidad, 'C', rutita);
			break;

		default:
			printf("No se selecciono un caracter correcto");
			break;
	}
	pthread_mutex_unlock(&mutexBitacoras);
}

t_log* iniciar_logger(char* logger_path){
	t_log *logger= log_create(logger_path, "iMongoStore", 0, LOG_LEVEL_INFO);
	if(logger  == NULL){
		printf("No se puede leer el logger");
		exit(1);
	}
	return logger;
}

t_config* leer_config(char* config_path){
	t_config* config=config_create(config_path);
	if(config == NULL){
		printf("No se pudo leer la config");
		free(config);
		exit(2);
	}
	return config;
}

_Bool esMetadataRecurso(char* rutini)
{
	return string_contains(rutini,"Oxigeno")||string_contains(rutini,"Basura")||
			string_contains(rutini,"Comida");
}
