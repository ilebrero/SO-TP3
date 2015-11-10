#include "srv.h"

void servidor(int mi_cliente)
{
    MPI_Status status; int origen, tag;
    int hay_pedido_local = FALSE;
    int listo_para_salir = FALSE;
    int highest_sequence_number = 0; //número de orden del pedido más grande conocido
    int cantidad_servidores = cant_ranks / 2;
    //los id de los servidos son las posiciones pares
    int mi_pedido, pedido; 

    char request_aplazados[cant_ranks]; //para cada i, request_aplazados[i] = TRUE si mi pedido tiene más prioridad que el i-ésimo, FALSE si no
    char veo_gente_muerta[cant_ranks]; //para cada i, veo_gente_muerta[i] = TRUE si terminó el servidor 
    char replies[cant_ranks]; // para cada i, replies[i] = TRUE si el servidor respondió el REQUEST
    
    int i;
    for (i = 0; i < cant_ranks ; ++i) {
      request_aplazados[i] = FALSE;
    }
    for (i = 0; i < cant_ranks ; ++i) {
      veo_gente_muerta[i] = FALSE;
    }
    for (i = 0; i < cant_ranks ; ++i) {
      replies[i] = FALSE;
    }
    replies[mi_cliente-1] = TRUE;


    while( ! listo_para_salir ) {
        
        MPI_Recv(&pedido, 1, MPI_INT, ANY_SOURCE, ANY_TAG, COMM_WORLD, &status);
        origen = status.MPI_SOURCE;
        tag = status.MPI_TAG;

        if (tag == TAG_PEDIDO) {
          assert(origen == mi_cliente);
          assert(hay_pedido_local == FALSE);
          hay_pedido_local = TRUE;
            
          if (cantidad_servidores == 1) { //caso particular en el que hay un sólo servidor (no tengo que esperar respuesta de otros)
            MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
          } else {

            highest_sequence_number++; //aumento el número de orden del pedido 
            mi_pedido = highest_sequence_number; //es el número de orden del actual
           
            for (i = 0 ; i < cant_ranks; i+=2) { //pido acceso exclusivo a todos los servidores

              if (i != mi_cliente-1 && (veo_gente_muerta[i] == FALSE)) //si no soy yo mismo y el servidor no terminó
                MPI_Send(&mi_pedido, 1, MPI_INT, i, TAG_REQUEST, COMM_WORLD);
            }
          }
        }

        else if (tag == TAG_REPLY) {
          replies[origen] = TRUE; //seteo en el vector de replies TRUE ya que "origen" es el servidor que respondió
          char contestaron_todos = TRUE;
          for (i = 0 ; i < cant_ranks ; i+=2){ //chequeo que todos los servidores activos me hayan contestado
           
            if ((veo_gente_muerta[i] == FALSE) && (replies[i] == FALSE)) //si alguno no contesto, retorno FALSE
              contestaron_todos = FALSE;
          }
          if (contestaron_todos) { //si todos los servidores contestaron
            MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD); //le aviso al cliente que ya puede acceder
            for (i = 0 ; i < cant_ranks ; i+=2){ //restauro vector replies
              replies[i] = FALSE;
            }
            replies[mi_cliente-1] = TRUE;
          }
        }

        else if (tag == TAG_LIBERO) {
          assert(origen == mi_cliente);
          assert(hay_pedido_local == TRUE);
          hay_pedido_local = FALSE;

          for (i = 0 ; i < cant_ranks ; i+=2) { //mando REPLY a todos los servidores que había aplazado por tener menor prioridad
            if (request_aplazados[i] && (veo_gente_muerta[i] == FALSE)) {

              MPI_Send(NULL, 0, MPI_INT, i, TAG_REPLY , COMM_WORLD);
              request_aplazados[i] = FALSE; //restauro request_aplazados
            }
          }
        }

        else if (tag == TAG_REQUEST) {
          if (highest_sequence_number < pedido) 
            highest_sequence_number = pedido; //actualizo mayor número de orden según el valor del pedido entrante

          if (hay_pedido_local) {
          
            if (mi_pedido < pedido) { //tengo prioridad
              request_aplazados[origen] = TRUE;              
            } 
           
            if (mi_pedido == pedido) {
              if (origen < mi_cliente) { //defino prioridad segúm mayor id de servidor
                request_aplazados[origen] = TRUE;
              } else { //tiene prioridad el pedido entrante  
                MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
              }
            }
           
            if (mi_pedido > pedido) //tiene prioridad el pedido entrante 
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
         
          } else { //tiene prioridad el pedido entrante 
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
          }
        }
        
        else if (tag == TAG_TERMINE) { 
          assert(origen == mi_cliente);
          listo_para_salir = TRUE;
          for (i = 0 ; i < cant_ranks ; i+=2) { //le aviso a todos los servidores activos que voy a terminar
            if (veo_gente_muerta[i] == FALSE)
              MPI_Send(NULL, 0, MPI_INT, i, TAG_MORI , COMM_WORLD);
          }
  
        }

        else if (tag == TAG_MORI) { //un servidor avisa que terminó
          cantidad_servidores--; //decremento cantidad de servidores
          veo_gente_muerta[origen] = TRUE; //asigno al difunto en su respectiva posición TRUE
          veo_gente_muerta[origen+1] = TRUE;//asigno al cliente difunto en su respectiva posición TRUE
        }
        
    }
    
}

