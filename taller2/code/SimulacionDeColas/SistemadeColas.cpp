/* Definiciones externas para el sistema de colas simple */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.cpp" /* Encabezado para el generador de numeros aleatorios */
#include <iostream>
#include <map>
#include <cstdlib>

#define LIMITE_COLA 100 /* Capacidad maxima de la cola */
#define OCUPADO 1       /* Indicador de Servidor Ocupado */
#define LIBRE 0         /* Indicador de Servidor Libre */

float *tiempos_entre_llegadas;
float *tiempos_atencion;
int total_atendidos = 0;
int total_llegadas = 0;

std::map<int, float> tiempos_ocupacion;

int sig_tipo_evento, num_clientes_espera, num_esperas_requerido, num_eventos,
    num_entra_cola, estado_servidor, semilla;
float area_num_entra_cola, area_estado_servidor, media_entre_llegadas, media_atencion,
    tiempo_simulacion, tiempo_llegada[LIMITE_COLA + 1], tiempo_ultimo_evento, tiempo_sig_evento[3],
    total_de_esperas;
FILE *parametros, *resultados, *csv_file;

void inicializar(void);
void controltiempo(void);
void llegada(void);
void salida(void);
void reportes(void);
void actualizar_estad_prom_tiempo(void);
void actualizar_ocupacion_sistema(void);
void actualizarMapa(std::map<int, int> &mapa, int clave, int n);

float expon(float mean);

int main(int argc, char* argv[]) /* Funcion Principal */
{

    /* Abre los archivos de entrada y salida */

    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");
    csv_file = fopen("resultados_simulacion.csv", "w"); // Abrir archivo CSV

    /* Especifica el numero de eventos para la funcion controltiempo. */

    num_eventos = 2;

    /* Lee los parametros de enrtrada. */

    if (argc >= 4) {
        media_entre_llegadas = atof(argv[1]);
        media_atencion = atof(argv[2]);
        num_esperas_requerido = atoi(argv[3]);
        semilla = atoi(argv[4]);
    }else{

        fscanf(parametros, "%f %f %d %d", &media_entre_llegadas, &media_atencion,
            &num_esperas_requerido, &semilla);
    }
    /* Escribe en el archivo de salida los encabezados del reporte y los parametros iniciales */

    fprintf(resultados, "Sistema de Colas Simple\n");
    fprintf(resultados, "Tiempo promedio de llegada%11.3f minutos\n",
            media_entre_llegadas);
    fprintf(resultados, "Tiempo promedio de atencion%16.3f minutos\n", media_atencion);
    fprintf(resultados, "Numero de clientes%14d\n", num_esperas_requerido);

    /* iInicializa la simulacion. */

    inicializar();

    /* Corre la simulacion mientras no se llegue al numero de clientes especificaco en el archivo de entrada*/

    while (num_clientes_espera < num_esperas_requerido)
    {

        /* Determina el siguiente evento */

        controltiempo();

        /* Actualiza los acumuladores estadisticos de tiempo promedio */

        actualizar_estad_prom_tiempo();

        /* Actualiza los valores de Ocupacion del sistema*/

        actualizar_ocupacion_sistema();

        /* Invoca la funcion del evento adecuado. */

        switch (sig_tipo_evento)
        {
        case 1:
            llegada();
            break;
        case 2:
            salida();
            break;
        }
    }

    /* Invoca el generador de reportes y termina la simulacion. */

    reportes();

    fclose(parametros);
    fclose(resultados);

    return 0;
}

void inicializar(void) /* Funcion de inicializacion. */
{

    // Escribir encabezados
    fprintf(csv_file, "Cliente,Tiempo entre llegadas (min),Tiempo de atencion (min)\n");

    /* Inicializa el reloj de la simulacion. */

    tiempo_simulacion = 0.0;

    /* Inicializa las variables de estado */

    estado_servidor = LIBRE;
    num_entra_cola = 0;
    tiempo_ultimo_evento = 0.0;

    /* Inicializa los contadores estadisticos. */

    num_clientes_espera = 0;
    total_de_esperas = 0.0;
    area_num_entra_cola = 0.0;
    area_estado_servidor = 0.0;

    tiempos_entre_llegadas = (float *)malloc((num_esperas_requerido + 1) * sizeof(float));
    tiempos_atencion = (float *)malloc((num_esperas_requerido + 1) * sizeof(float));

    /* Inicializa la lista de eventos. Ya que no hay clientes, el evento salida
       (terminacion del servicio) no se tiene en cuenta */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempo_sig_evento[2] = 1.0e+30;
}

void controltiempo(void) /* Funcion controltiempo */
{
    int i;
    float min_tiempo_sig_evento = 1.0e+29;

    sig_tipo_evento = 0;

    /*  Determina el tipo de evento del evento que debe ocurrir. */

    for (i = 1; i <= num_eventos; ++i)
        if (tiempo_sig_evento[i] < min_tiempo_sig_evento)
        {
            min_tiempo_sig_evento = tiempo_sig_evento[i];
            sig_tipo_evento = i;
        }

    /* Revisa si la lista de eventos esta vacia. */

    if (sig_tipo_evento == 0)
    {

        /* La lista de eventos esta vacia, se detiene la simulacion. */

        fprintf(resultados, "\nLa lista de eventos esta vacia %f", tiempo_simulacion);
        exit(1);
    }

    /* TLa lista de eventos no esta vacia, adelanta el reloj de la simulacion. */

    tiempo_simulacion = min_tiempo_sig_evento;
}

void llegada(void) /* Funcion de llegada */
{
    float espera;
    // std::cout << "llegada " << tiempo_simulacion << std::endl;
    // tiempos_entre_llegadas[total_llegadas] = tiempo_simulacion;
    // total_llegadas++;

    /* Programa la siguiente llegada. */

    tiempo_sig_evento[1] = tiempo_simulacion + expon(media_entre_llegadas);
    tiempos_entre_llegadas[total_llegadas] = tiempo_sig_evento[1] - tiempo_simulacion;
    total_llegadas++;
    /* Reisa si el servidor esta OCUPADO. */

    if (estado_servidor == OCUPADO)
    {

        /* Sservidor OCUPADO, aumenta el numero de clientes en cola */

        ++num_entra_cola;

        /* Verifica si hay condici�n de desbordamiento */

        if (num_entra_cola > LIMITE_COLA)
        {

            /* Se ha desbordado la cola, detiene la simulacion */

            fprintf(resultados, "\nDesbordamiento del arreglo tiempo_llegada a la hora");
            fprintf(resultados, "%f", tiempo_simulacion);
            exit(2);
        }

        /* Todavia hay espacio en la cola, se almacena el tiempo de llegada del
            cliente en el ( nuevo ) fin de tiempo_llegada */

        tiempo_llegada[num_entra_cola] = tiempo_simulacion;
    }

    else
    {

        /*  El servidor esta LIBRE, por lo tanto el cliente que llega tiene tiempo de eespera=0
           (Las siguientes dos lineas del programa son para claridad, y no afectan
           el reultado de la simulacion ) */

        espera = 0.0;
        total_de_esperas += espera;

        /* Incrementa el numero de clientes en espera, y pasa el servidor a ocupado */
        ++num_clientes_espera;
        estado_servidor = OCUPADO;

        /* Programa una salida ( servicio terminado ). */

        tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
        // std::cout << "genera tiempo: " << expon(media_atencion) << std::endl;
        tiempos_atencion[total_atendidos] = tiempo_sig_evento[2] - tiempo_simulacion;
        total_atendidos++;
    }
}

void salida(void) /* Funcion de Salida. */
{
    int i;
    float espera;
    // std::cout << "salida " << tiempo_simulacion << std::endl;
    /* Revisa si la cola esta vacia */

    if (num_entra_cola == 0)
    {

        /* La cola esta vacia, pasa el servidor a LIBRE y
        no considera el evento de salida*/
        estado_servidor = LIBRE;
        tiempo_sig_evento[2] = 1.0e+30;
    }

    else
    {

        /* La cola no esta vacia, disminuye el numero de clientes en cola. */
        --num_entra_cola;

        /*Calcula la espera del cliente que esta siendo atendido y
        actualiza el acumulador de espera */

        espera = tiempo_simulacion - tiempo_llegada[1];
        total_de_esperas += espera;

        /*Incrementa el numero de clientes en espera, y programa la salida. */
        ++num_clientes_espera;
        tiempo_sig_evento[2] = tiempo_simulacion + expon(media_atencion);
        // std::cout << "genera tiempo: " << expon(media_atencion) << std::endl;
        tiempos_atencion[total_atendidos] = tiempo_sig_evento[2] - tiempo_simulacion;
        total_atendidos++;
        /* Mueve cada cliente en la cola ( si los hay ) una posicion hacia adelante */
        for (i = 1; i <= num_entra_cola; ++i)
            tiempo_llegada[i] = tiempo_llegada[i + 1];
    }
}

void reportes(void) /* Funcion generadora de reportes. */
{

    float suma_llegadas = 0.0;
float suma_atencion = 0.0;

for (int i = 0; i < total_llegadas; ++i)
    suma_llegadas += tiempos_entre_llegadas[i];

for (int i = 0; i < total_atendidos; ++i)
    suma_atencion += tiempos_atencion[i];

fprintf(resultados, "\nPromedio real de tiempo entre llegadas: %.3f",
        suma_llegadas / total_llegadas);
fprintf(resultados, "\nPromedio real de tiempo de atención: %.3f",
        suma_atencion / total_atendidos);

         // Escribir datos de cada cliente
    for (int i = 0; i < total_llegadas; i++)
    {
        fprintf(csv_file, "%d,%.3f,%.3f\n",
                i,
                tiempos_entre_llegadas[i],
                tiempos_atencion[i]);
    }

    fclose(csv_file);

    free(tiempos_entre_llegadas);
    free(tiempos_atencion);

    /* Calcula y estima los estimados de las medidas deseadas de desempe�o */
    fprintf(resultados, "\nEspera promedio en la cola: %11.3f minutos",
            total_de_esperas / num_clientes_espera);
    fprintf(resultados, "\nNumero promedio en cola: %10.3f",
            area_num_entra_cola / tiempo_simulacion);
    fprintf(resultados, "\nUso del servidor: %15.3f",
            area_estado_servidor / tiempo_simulacion);
    fprintf(resultados, "\nTiempo de terminacion de la simulacion: %12.3f minutos", tiempo_simulacion);

        printf(
        "Tiempo promedio de llegada (parámetro):%.3f\n"
        "Tiempo promedio de atención (parámetro):%.3f\n"
        "Número de clientes (parámetro):%d\n"
        "Semilla (parámetro):%d\n"
        "Tiempo promedio real de llegada:%.3f\n"
        "Tiempo promedio real de atencion:%.3f\n"
        "Tiempo de terminacion de la simulacion:%.3f\n"
        "Espera promedio en la cola:%.3f\n"
        "Numero promedio en cola:%.3f\n"
        "Uso del servidor:%.3f\n",
        media_entre_llegadas,
        media_atencion,        
        num_esperas_requerido,                    // Número de clientes (parámetro)
        semilla,                                 // Semilla (parámetro)
        suma_llegadas / total_llegadas,           // Tiempo promedio de llegada (parámetro)
        suma_atencion / total_atendidos,          // Tiempo promedio de atención (parámetro)
        tiempo_simulacion,                        // Tiempo de terminación de la simulación
        total_de_esperas / num_clientes_espera,   // Espera promedio en la cola
        area_num_entra_cola / tiempo_simulacion,  // Número promedio en cola
        area_estado_servidor / tiempo_simulacion  // Uso del servidor
    );
}

void actualizar_estad_prom_tiempo(void) /* Actualiza los acumuladores de
                                                       area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
        del ultimo evento */

    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    // tiempo_ultimo_evento = tiempo_simulacion; se actualiza más adelante

    /* Actualiza el area bajo la funcion de numero_en_cola */
    area_num_entra_cola += num_entra_cola * time_since_last_event;

    /*Actualiza el area bajo la funcion indicadora de servidor ocupado*/
    area_estado_servidor += estado_servidor * time_since_last_event;
}

void actualizarMapa(std::map<int, float> &mapa, int clave, float n)
{
    if (mapa.find(clave) == mapa.end())
    {
        // La clave no existe, se registra con n
        mapa[clave] = n;
    }
    else
    {
        // La clave existe, se suma n al valor actual
        mapa[clave] += n;
    }
}

void actualizar_ocupacion_sistema(void) /* Actualiza los acumuladores de
                                                       area para las estadisticas de tiempo promedio. */
{
    float time_since_last_event;

    // se agregan la cantidad en la cola
    int n = num_entra_cola;

    // se agregan los usuarios en el servidor
    if (estado_servidor == OCUPADO)
        n++;

    /* Calcula el tiempo desde el ultimo evento, y actualiza el marcador
        del ultimo evento */
    time_since_last_event = tiempo_simulacion - tiempo_ultimo_evento;
    tiempo_ultimo_evento = tiempo_simulacion;

    // se guarda el valor
    actualizarMapa(tiempos_ocupacion, n, time_since_last_event);

    // std::cout << "n " << n << "tiempo" << time_since_last_event << std::endl;
    // for (const auto &par : tiempos_ocupacion)
    // {
    //     std::cout << par.first << ": " << par.second << std::endl;
    // }
}

float expon(float media)  /* Funcion generadora de la exponencias */
{
    /* Retorna una variable aleatoria exponencial con media "media"*/

    return -media * log(lcgrand(semilla));
}
