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
    int replies = 0;
    int mi_pedido, pedido;

    char  request_aplazados[cantidad_servidores];
    
    int i;
    for (i = 0; i < cantidad_servidores ; ++i) {
      request_aplazados[i] = FALSE;
    }

    while( ! listo_para_salir ) {
        
        MPI_Recv(&pedido, 1, MPI_INT, ANY_SOURCE, ANY_TAG, COMM_WORLD, &status);
        origen = status.MPI_SOURCE;
        tag = status.MPI_TAG;
//        request_secuence_number = ????

        if (tag == TAG_PEDIDO) {
            assert(origen == mi_cliente);
            debug("Mi cliente solicita acceso exclusivo");
            assert(hay_pedido_local == FALSE);
            hay_pedido_local = TRUE;
            debug("Dándole permiso (frutesco por ahora)");
        
           highest_sequence_number++;
           mi_pedido = highest_sequence_number;
           
            debug("HARE1");
           for (i = 0 ; i < cantidad_servidores ; ++i) {
              if (i != mi_cliente)
                MPI_Send(&mi_pedido, 1, MPI_INT, i, TAG_REQUEST, COMM_WORLD);
            }
            debug("HARE2");
        }

        else if (tag == TAG_REPLY) {
            debug("HARE3");
          replies++;
          if (replies == cantidad_servidores) {
            MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            replies = 0;
          debug("HARE4");
          }
        }

        else if (tag == TAG_LIBERO) {
          debug("HARE5");
            assert(origen == mi_cliente);
            debug("Mi cliente libera su acceso exclusivo");
            assert(hay_pedido_local == TRUE);
            hay_pedido_local = FALSE;

            for (i = 0 ; i < cantidad_servidores ; ++i) {
              if (request_aplazados[i]) {
                MPI_Send(NULL, 0, MPI_INT, i, TAG_REPLY , COMM_WORLD);
                request_aplazados[i] = FALSE;
              }
            }
          debug("HARE6");
        }

        else if (tag == TAG_REQUEST) {
          debug("HARE7");
          if (highest_sequence_number < pedido) 
            highest_sequence_number = pedido;

          if (hay_pedido_local) {
            if (mi_pedido > pedido) {
          debug("HARE8");
              request_aplazados[pedido] = TRUE;              
            } 
            if (mi_pedido == pedido) {
              if (origen < mi_cliente) {
          debug("HARE9");
                request_aplazados[pedido] = TRUE;
              } else {   
          debug("HARE10");
                MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
              }
            }
            if (mi_pedido < pedido)
          debug("HARE11");
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
          } else {
          debug("HARE12");
              MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY , COMM_WORLD);
          }
        }
        
        else if (tag == TAG_TERMINE) {
          debug("HARE13");
            assert(origen == mi_cliente);
            debug("Mi cliente avisa que terminó");
            listo_para_salir = TRUE;
        }
        
    }
    
}

