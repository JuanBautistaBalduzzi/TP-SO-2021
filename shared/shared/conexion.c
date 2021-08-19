#include "conexion.h"

//#define PUERTO "6667"
#define BACKLOG 5			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

/*========================== SERVIDOR =================================*/
int crear_server(char* puerto){
	  int socket_server;
	  struct addrinfo mi_info;

	  memset(&mi_info, 0, sizeof(struct addrinfo));

	  mi_info.ai_family = AF_INET;
	  mi_info.ai_socktype = SOCK_STREAM;
	  mi_info.ai_protocol = IPPROTO_TCP;
	  mi_info.ai_flags = AI_PASSIVE;

	  struct addrinfo *resultado;

	  if (getaddrinfo(NULL, puerto, &mi_info, &resultado) != 0) {
//	    printf("Error al parsear la direccion del socket\n");
	    exit(EXIT_FAILURE);
	  }
//	  printf("Direccion parseada correctamente\n");

	  struct addrinfo *aux_resultado;

	  for (aux_resultado = resultado; aux_resultado != NULL; aux_resultado = aux_resultado->ai_next) {
	    socket_server = socket(aux_resultado->ai_family, aux_resultado->ai_socktype, aux_resultado->ai_protocol);

	    if (socket_server != -1) {
//	      printf("Socket creado correctamente\n");
	      if (bind(socket_server, aux_resultado->ai_addr, aux_resultado->ai_addrlen) != -1) {
	        break;
	      } else {
	    	  printf("\nEl puerto ya esta siendo utilizado\n");
	        exit(EXIT_FAILURE);
	        close(socket_server);
	      }
	    }
	  }
	  if (aux_resultado == NULL) {
//	    printf("No se pudo crear el socket\n");
	    exit(EXIT_FAILURE);
	  }

	  freeaddrinfo(resultado);
	  if (listen(socket_server, BACKLOG) != -1)
//	      printf("El servidor esta \"escuchando\" la informacion \n");

	  printf("Servidor creado y bindeado correctamente\n");


	  return socket_server;
}

int esperar_cliente(int socket_server, int backlog){
	int socket_cliente;

	struct sockaddr direccion_cliente;
	socklen_t tamanio_direccion_cliente = sizeof(direccion_cliente);
//	puts("Esperando nuevo cliente...");
	socket_cliente = accept(socket_server, (struct sockaddr *)&direccion_cliente, &tamanio_direccion_cliente);
	if (socket_cliente == -1) {
//		puts("El cliente no se pudo conectar");
		return (-1);
}

//	printf("Nuevo cliente aceptado con el socket: %d \n",socket_cliente);
	return socket_cliente;


}

/*========================== CLIENTE =================================*/
int crear_conexion(char* ip, char* puerto)
{
	int socket_cliente;

	  struct addrinfo* mi_info = malloc(sizeof(struct addrinfo));

	  memset(mi_info, 0, sizeof(struct addrinfo));
	  mi_info->ai_family = AF_INET;
	  mi_info->ai_socktype = SOCK_STREAM;
	  mi_info->ai_protocol = IPPROTO_TCP;
	  mi_info->ai_flags = AI_PASSIVE;

	  struct addrinfo* resultado;

	  if (getaddrinfo(ip, puerto, mi_info, &resultado) != 0) {
//	   printf("Error al parsear la direccion del socket\n");
	    return (-1);
	  }
//	  printf("Direccion parseada correctamente\n");

	  struct addrinfo* aux_resultado;

	  for (aux_resultado = resultado; aux_resultado != NULL; aux_resultado = aux_resultado->ai_next) {
	    socket_cliente = socket(aux_resultado->ai_family, aux_resultado->ai_socktype, aux_resultado->ai_protocol);

	    if (socket_cliente != -1) {
//	      printf("Socket creado correctamente\n");
	      if (connect(socket_cliente, aux_resultado->ai_addr, aux_resultado->ai_addrlen) != -1) {
	        break;
	      } else {
//	        printf("No se pudo crear la conexion al socket\n");
	        close(socket_cliente);
			freeaddrinfo(mi_info);
			freeaddrinfo(resultado);
	        return (-1);
	      }
	    }
	  }
	  if (aux_resultado == NULL) {
		freeaddrinfo(mi_info);
		freeaddrinfo(resultado);
//	    printf("No se pudo crear el socket\n");
	    return (-1);
	  }
	  freeaddrinfo(mi_info);
	  freeaddrinfo(resultado);
//	  free(resultado);
//	  free(mi_info);
	  return socket_cliente;
}

/*========================== TODOS =================================*/
t_paquete* recibir_paquete(int socket_cliente, int* respuesta) {
	t_paquete* aux_paquete = crear_paquete(-1);

	uint8_t aux_cod;
	uint32_t aux_tamanio;
	void* aux_stream = NULL;

	//puts("tratando de recibir el paquete\n");
	if (recv(socket_cliente, &(aux_cod), sizeof(uint8_t), 0) <= 0) {
		*respuesta = ERROR;
		send(socket_cliente, &respuesta, sizeof(uint8_t), 0);
		return aux_paquete;
	}

	if (recv(socket_cliente, &(aux_tamanio), sizeof(uint32_t), 0) <= 0) {
		*respuesta = ERROR;
		send(socket_cliente, &respuesta, sizeof(uint8_t), 0);
		return aux_paquete;
	}

	aux_stream = malloc(aux_tamanio);
	if (recv(socket_cliente, aux_stream, aux_tamanio, 0) <= 0) {
		*respuesta = ERROR;
		send(socket_cliente, &respuesta, sizeof(uint8_t), 0);
		return aux_paquete;
	}
	*respuesta = OK;

	send(socket_cliente, respuesta, sizeof(uint8_t), 0); // aca le enviamos un mensaje (un uno) al cliente, informandole que recibimos la info correctamente

	aux_paquete->codigo_operacion = (uint8_t) aux_cod;
	aux_paquete->buffer->size = (uint32_t) aux_tamanio;

	aux_paquete->buffer->stream = aux_stream;
	return aux_paquete;
}

t_paquete* crear_paquete(CODE_OP codigo) {
  t_paquete* paquete = malloc(sizeof(t_paquete));
  paquete->codigo_operacion = (uint8_t) codigo;
  crear_buffer(paquete);
  return paquete;
}

void crear_buffer(t_paquete* paquete) {
  paquete->buffer = malloc(sizeof(t_buffer));
  paquete->buffer->size = 0;
  paquete->buffer->stream = NULL;
}

int enviar_paquete(t_paquete* paquete, int socket_cliente) {
//	printf("paquete a enviar de tipo: %d\n",paquete->codigo_operacion);
	int bytes = paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, bytes);
	int respuesta = ERROR;

	if (send(socket_cliente, a_enviar, bytes, 0) > 0) {
//		puts("Paquete enviado");
		recv(socket_cliente, &respuesta, sizeof(uint8_t), 0); // Recordar que aca se puede recibir cualquier estructura que se necesite, en este caso recibimos un int con el que verificamos que se envio \todo correctamente
		if (respuesta) {
//			puts("Respuesta recibida\n");
		} else {
			printf("No recibimos respuesta del servidor %d \n",paquete->codigo_operacion);
		}
	} else {
		printf("No se pudo enviar el paquete %d \n",paquete->codigo_operacion);
	}
		free(a_enviar);
		liberar_conexion(socket_cliente);
		eliminar_paquete(paquete);
	return respuesta;
}

char* enviar_paquete_respuesta_string(t_paquete* paquete,int socket){
//	printf("paquete a enviar de tipo: %d\n",paquete->codigo_operacion);
	int bytes = paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);
	void* a_enviar = serializar_paquete(paquete, bytes);
	uint8_t respuesta=0;

	uint32_t tamanio_tarea = 0;
	char* tarea = NULL;

	if (send(socket, a_enviar, bytes, 0) > 0) {
//		puts("Paquete enviado");

		if(recv(socket, &respuesta, sizeof(uint8_t), 0) <=0){
			liberar_conexion(socket);
			eliminar_paquete(paquete);
			return NULL;
		}

		if(recv(socket, &tamanio_tarea, sizeof(uint32_t), 0) <=0){
			liberar_conexion(socket);
			eliminar_paquete(paquete);
			return NULL;
		}

		tarea = malloc(tamanio_tarea);
		if(recv(socket, tarea, tamanio_tarea, 0) <= 0){
			free(tarea);
			respuesta = NULL;
			liberar_conexion(socket);
			eliminar_paquete(paquete);
			return NULL;
		}

//		puts("Respuesta recibida\n");

		} else {
			puts("No se pudo enviar el paquete\n");
		}
		free(a_enviar);
//		liberar_conexion(socket);
		eliminar_paquete(paquete);
	return tarea;
}

void* serializar_paquete(t_paquete* paquete, int bytes) { // Los bytes serian sizeof( int cod_op) + sizeof(int tamanio_stream) + el tamaño real del buffer
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(uint8_t));
	desplazamiento += sizeof(uint8_t);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, paquete->buffer->stream,paquete->buffer->size);

	return magic;
}

void eliminar_paquete(t_paquete* paquete) {
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente) {
//	printf("liberando socket: %d\n",socket_cliente);
	close(socket_cliente);
}


