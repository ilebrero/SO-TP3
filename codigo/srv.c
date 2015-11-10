#include "srv.h"

/*
 *  Ejemplo de servidor que tiene el "sí fácil" para con su
 *  cliente y no se lleva bien con los demás servidores.
 *
 */

void servidor(int mi_cliente)
{
    MPI_Status status; int origen, tag;
    int hay_pedido_local = FALSE;
    int listo_para_salir = FALSE;
    int highest_sequence_number = 0;
    int cantidad_servidores = cant_ranks / 2;
    int mi_pedido, pedido;

    char request_aplazados[cant_ranks];
    char veo_gente_muerta[cant_ranks];
    char replies[cant_ranks];
    
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
//        printf("pedido: %d\n", pedido);
//        printf("\norigen: %d\n", origen);
//        printf("cliente: %d\n\n", mi_cliente);
//        request_secuence_number = ????

        if (tag == TAG_PEDIDO) {
              //printf("entre aca\n");
            assert(origen == mi_cliente);
//            debug("Mi cliente solicita acceso exclusivo");
            assert(hay_pedido_local == FALSE);
            hay_pedido_local = TRUE;
            
            if (cantidad_servidores == 1) {
            //  printf("me pedi a mi mismo\n");
              MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            } else {
//            debug("Dándole permiso (frutesco por ahora)");
        
               highest_sequence_number++;
               mi_pedido = highest_sequence_number;
           
               for (i = 0 ; i < cant_ranks; i+=2) {

                 if (i != mi_cliente-1 && (veo_gente_muerta[i] == FALSE))
                   MPI_Send(&mi_pedido, 1, MPI_INT, i, TAG_REQUEST, COMM_WORLD);
               }
             }
        }

        else if (tag == TAG_REPLY) {
          replies[origen] = TRUE;
          char contestaron_todos = TRUE;
          for (i = 0 ; i < cant_ranks ; i+=2){
           
            if ((veo_gente_muerta[i] == FALSE) && (replies[i] == FALSE))
              contestaron_todos = FALSE;
          }
   //       printf("replies%d\n", replies);
 //         printf("cantidad_servidores%d\n", cantidad_servidores);
          if (contestaron_todos) {
            //printf("pase! %d | replies %d | cantidad servidores %d", mi_cliente-1, replies, cantidad_servidores);
            //debug("aloha");
            MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            for (i = 0 ; i < cant_ranks ; i+=2){
              replies[i] = FALSE;
            }
            replies[mi_cliente-1] = TRUE;
          }
        }

        else if (tag == TAG_LIBERO) {
            assert(origen == mi_cliente);
//            debug("Mi cliente libera su acceso exclusivo");
            assert(hay_pedido_local == TRUE);
            hay_pedido_local = FALSE;

            for (i = 0 ; i < cant_ranks ; i+=2) {
              if (request_aplazados[i] && (veo_gente_muerta[i] == FALSE)) {

                MPI_Send(NULL, 0, MPI_INT, i, TAG_REPLY , COMM_WORLD);
                request_aplazados[i] = FALSE;
              }
            }
        }

        else if (tag == TAG_REQUEST) {
          if (highest_sequence_number < pedido) 
            highest_sequence_number = pedido;

          //printf("pedido %d\n", pedido);
          //printf("hay pedido local%d\n", hay_pedido_local);
          //printf("mi_pedido %d\n", mi_pedido);

          if (hay_pedido_local) {
          
            if (mi_pedido < pedido) {
              //printf("aplace por que soy mayor");
              request_aplazados[origen] = TRUE;              
            } 
           
            if (mi_pedido == pedido) {
              if (origen < mi_cliente) {
               //printf("aplace por que estoy antes en el lugar");
                request_aplazados[origen] = TRUE;
              } else {   
                MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
              }
            }
           
            if (mi_pedido > pedido)
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
         
          } else {
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
          }
        }
        
        else if (tag == TAG_TERMINE) {
            assert(origen == mi_cliente);
//            debug("Mi cliente avisa que terminó");
            listo_para_salir = TRUE;
            for (i = 0 ; i < cant_ranks ; i+=2) {
              if (veo_gente_muerta[i] == FALSE)
                MPI_Send(NULL, 0, MPI_INT, i, TAG_MORI , COMM_WORLD);
            }
  
        //    for (i = 0 ; i < cant_ranks ; i+=2) {
        //      if (request_aplazados[i] && (veo_gente_muerta[i] == 0))
        //        MPI_Send(NULL, 0, MPI_INT, i, TAG_REPLY, COMM_WORLD);
        //    }
        }

        else if (tag == TAG_MORI) {
          cantidad_servidores--;
            //if (replies == cantidad_servidores-1 || replies == cantidad_servidores)
             // replies--;
            //printf("replies%d\n", replies);
            //printf("cantidad_servidores%d\n", cantidad_servidores);
            veo_gente_muerta[origen] = TRUE;
            veo_gente_muerta[origen+1] = TRUE;
        }
        
    }
    
}

/*
        3 servers

        replies = 0

        server 1 me contesta
        replies = 1
        server 2 me contesta
        replies = 2
*/
