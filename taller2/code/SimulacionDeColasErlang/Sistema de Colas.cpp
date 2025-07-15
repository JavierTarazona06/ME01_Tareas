/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp"  /* Encabezado para el generador de numeros aleatorios */

#define LIMITE_COLA 100  /* Capacidad maxima de la cola */
#define ERLANG_B 0
#define ERLANG_C 1
#define INF 1.0e+30

int   sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
      num_entra_cola, estado_servidor;
float area_num_entra_cola, *area_estado_servidores, media_entre_llegadas, media_atencion,
      tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento,
      total_de_esperas, erlangB, erlangC;
float* tiempo_sig_evento;
FILE  *parametros, *resultados;


int num_servidores; // número de servidores
int servidores_ocupados = 0;
int clientes_bloqueados = 0;      // para Erlang B
int clientes_que_esperaron = 0;   // para Erlang C
int total_clientes_llegaron = 0;
float tiempo_servidores_todos_ocupados = 0.0;

int modo = ERLANG_B; 

void  inicializar(void);
void  controltiempo(void);
void  llegada(void);
void  salida(void);
void  reportes(void);
void  actualizar_estad_prom_tiempo(void);
float expon(float mean);


int main(void)  /* Funcion Principal */
{
    /* Abre los archivos de entrada y salida */

    parametros  = fopen("param.txt",  "r");
    resultados = fopen("result.txt", "w");

    /* Lee los parametros de enrtrada. */

    fscanf(parametros, "%f %f %d %d %d", &media_entre_llegadas, &media_atencion,
           &num_esperas_requerido, &modo, &num_servidores);


    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Sistema de Colas Simple\n\n");
    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n\n",
            media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n\n", media_atencion);
    fprintf(resultados, "Numero de clientes%14d\n\n", num_esperas_requerido);
    fprintf(resultados, "Modo de simulación:              %s\n", (modo == ERLANG_B) ? "Erlang B (sin cola)" : "Erlang C (con cola)");
    fprintf(resultados, "Número de servidores (m):        %d\n", num_servidores);

    /* iInicializa la simulacion. */

    inicializar();

    /* Corre la simulacion mientras no se llegue al numero de clientes especificaco en el archivo de entrada*/

    while (num_clientes_espera < num_esperas_requerido) {

        /* Determina el siguiente evento */

        controltiempo();

        /* Actualiza los acumuladores estadisticos de tiempo promedio */

        actualizar_estad_prom_tiempo();

        /* Invoca la funcion del evento adecuado. */

        if (sig_tipo_evento == 1) {
            llegada();
        } else {
            salida();  // aplica a cualquier servidor (2..m+1)
        }
    }

    /* Invoca el generador de reportes y termina la simulacion. */

    reportes();
    delete[] area_estado_servidores;
    fclose(parametros);
    fclose(resultados);

    return 0;
}

void inicializar(void)  /* Funcion de inicializacion. */
{
    /* Inicializa el reloj de la simulacion. */

    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */

    num_entra_cola        = 0;
    tiempo_ultimo_evento = 0.0;
    servidores_ocupados = 0;

    /* Inicializa los contadores estadisticos. */

    num_clientes_espera  = 0;
    total_de_esperas    = 0.0;
    area_num_entra_cola      = 0.0;
    area_estado_servidores = new float[num_servidores];
    clientes_bloqueados = 0;
    clientes_que_esperaron = 0;
    erlangC = 0;
    erlangB = 0;

    // Inicializar eventos: 0 no se usa, [1] = llegada, [2..m+1] = salidas de servidores
    num_eventos = num_servidores + 1;
    tiempo_sig_evento = (float*) malloc((num_eventos + 1) * sizeof(float));
    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    for (int i = 2; i <= num_servidores + 1; ++i) {
        tiempo_sig_evento[i] = 1.0e+30; // sin salida programada aún
        area_estado_servidores[i] = 0;
    }
    
}


void controltiempo(void)  /* Funcion controltiempo */
{
    int   i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    /*  Determina el tipo de evento del evento que debe ocurrir. */

    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento) {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento     = i;
        }

    /* Revisa si la lista de eventos esta vacia. */

    if (sig_tipo_evento == 0) {
        
        /* La lista de eventos esta vacia, se detiene la simulacion. */

        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* TLa lista de eventos no esta vacia, adelanta el reloj de la simulacion. */

    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void)  /* Funcion de llegada */
{
    float espera;
    ++total_clientes_llegaron;

    /* Programa la siguiente llegada. */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);

    // Hay servidores disponibles
    if (servidores_ocupados < num_servidores) {
        /*  hay un servidor libre, por lo tanto el cliente que llega tiene tiempo de eespera=0
           (Las siguientes dos lineas del programa son para claridad, y no afectan
           el reultado de la simulacion ) */

        espera            = 0.0;
        total_de_esperas += espera;

        /* Incrementa el numero de clientes en espera, aumentar la cantidad de servidores ocupados*/
        ++num_clientes_espera;
        ++servidores_ocupados;

        /* Programa una salida ( servicio terminado ). */     

        tiempo_sig_evento[servidores_ocupados + 1] = tiempo_simulacion + expon(media_atencion);
        
    }

    else {
        // No hay servidores disponibles
        if (modo == ERLANG_B) {
            // En Erlang B, se bloquea el cliente
            ++clientes_bloqueados;
        }
        else if (modo == ERLANG_C) {
            // En Erlang C, el cliente entra a cola
            /* Todos los servidores están ocupados, aumenta el numero de clientes en cola */
            ++num_entra_cola;
            ++clientes_que_esperaron;
            /* Verifica si hay condici�n de desbordamiento */

            if (num_entra_cola > LIMITE_COLA) {

                /* Se ha desbordado la cola, detiene la simulacion */

                fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
                fprintf(resultados, "%f", tiempo_simulacion);
                exit(2);
            }

            // Registrar su hora de llegada en la cola
            tiempo_llegada[num_entra_cola] = tiempo_simulacion;
        }
    }
    
}


void salida(void)  /* Funcion de Salida. */
{
    int   i;
    float espera;


    /* Revisa si la cola esta vacia */
    if (num_entra_cola == 0) {
        // No hay clientes esperando, liberar el tiempo del evento de salida actual
        tiempo_sig_evento[sig_tipo_evento] = 1.0e+30;
        servidores_ocupados--;
    }

    else {

        /* La cola no esta vacia, disminuye el numero de clientes en cola. */
        --num_entra_cola;

        /*Calcula la espera del cliente que esta siendo atendido y
        actualiza el acumulador de espera */

        espera            = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        /*Incrementa el numero de clientes en espera, y programa la salida. */   
        ++num_clientes_espera;
        tiempo_sig_evento[sig_tipo_evento] = tiempo_simulacion + expon(media_atencion);

        /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante */

        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

void reportes(void)  /* Funcion generadora de reportes. */
{
    /* Calcula y estima los estimados de las medidas deseadas de desempe�o */  
    fprintf(resultados, "\n\nEspera promedio en la cola%11.3f minutos\n\n",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "Numero promedio en cola%10.3f\n\n",
            area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "Uso de Servidores\n\n");
    for(int i=0; i<num_servidores; i++){
        fprintf(resultados, "Uso del servidor %d : %15.3f\n", i+1,
            area_estado_servidores[i]/tiempo_simulacion);
    }
    fprintf(resultados, "Tiempo de terminacion de la simulacion%12.3f minutos", tiempo_simulacion);
    if (modo == ERLANG_B) {  // Erlang B
        float prob_bloqueo = (float)erlangB/tiempo_simulacion;;
        fprintf(resultados, ">>> Modo Erlang B (sin cola)\n");
        fprintf(resultados, "Clientes bloqueados (rechazados):           %d\n", clientes_bloqueados);
        fprintf(resultados, "Probabilidad de bloqueo estimada (P_b):     %11.4f\n", prob_bloqueo);
    }
    else if (modo == ERLANG_C) {  // Erlang C
        float prob_espera = (float)erlangC/tiempo_simulacion;
        fprintf(resultados, ">>> Modo Erlang C (con cola)\n");
        fprintf(resultados, "Clientes que debieron esperar:              %d\n", clientes_que_esperaron);
        fprintf(resultados, "Probabilidad de espera estimada (P_w):      %11.4f\n", prob_espera);
    }
}


void actualizar_estad_prom_tiempo(void)  /* Actualiza los acumuladores de
														area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
    	del ultimo evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento       = tiempo_simulacion;

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola      += num_entra_cola * time_since_last_event;

    /*Actualiza el area bajo la funcion indicadora de servidor ocupado*/
    for (int s = 0; s < num_servidores; ++s) {
        int idx = s + 2;                       /* posición en tiempo_sig_evento   */
        int ocupado = (tiempo_sig_evento[idx] < INF) ? 1 : 0;

        /* Acumula el área solo para ese servidor */
        area_estado_servidores[s] += ocupado * time_since_last_event;
    }
    
    // Erlang C: tiempo donde todos los servidores están ocupados
    if (servidores_ocupados == num_servidores && modo == ERLANG_C) {
        erlangC += time_since_last_event;
    }

    // Erlang B: todos ocupados Y sin cola (clientes perdidos)
    if (servidores_ocupados == num_servidores && modo == ERLANG_B) {
        erlangB += time_since_last_event;
    }
}


float expon(float media)  /* Funcion generadora de la exponencias */
{
    /* Retorna una variable aleatoria exponencial con media "media"*/

    return -media * log(lcgrand(1));
}

