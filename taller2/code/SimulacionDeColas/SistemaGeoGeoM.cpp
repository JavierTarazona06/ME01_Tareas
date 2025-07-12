/* Simulador del sistema de colas Geom/Geom/m/N 
   Adaptado para seguir la arquitectura de eventos discretos */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>

#define MAX_CAPACITY 1000    /* Capacidad maxima del sistema */
#define MAX_SERVERS 10       /* Numero maximo de servidores */
#define MAX_EVENTOS 10000    /* Tamaño maximo de la lista de eventos */

/* Tipos de eventos */
typedef enum {
    LLEGADA = 1,
    SALIDA = 2,
    FIN_SIMULACION = 3
} TipoEvento;

/* Estructura para un evento */
typedef struct {
    double tiempo;
    TipoEvento tipo;
    int servidor_id;  /* Para eventos de salida */
} Evento;

/* Lista de eventos futuros */
typedef struct {
    Evento eventos[MAX_EVENTOS];
    int num_eventos;
} ListaEventos;

/* Variables globales del sistema - Parámetros (P) */
int N;                     /* Capacidad del sistema */
int m;                     /* Numero de servidores */
double p;                  /* Probabilidad de llegada en cada slot */
double s;                  /* Probabilidad de completar servicio en cada slot */
double tiempo_simulacion;  /* Tiempo total de simulacion */

/* Variables de estado del sistema (X) */
int estado_actual;         /* Numero de clientes en el sistema */
int servidores_ocupados;   /* Numero de servidores ocupados */
int cola_length;           /* Longitud de la cola */
double tiempo_actual;      /* Tiempo actual de simulacion (T) */

/* Variables de características del sistema (C) */
long long tiempo_en_estado[MAX_CAPACITY + 1];  /* Tiempo acumulado en cada estado */
long long clientes_perdidos;                   /* Clientes que no pudieron entrar */
long long clientes_llegados;                   /* Total de clientes que intentaron llegar */
long long clientes_atendidos;                  /* Clientes completamente atendidos */
double ultimo_cambio_estado;                   /* Tiempo del último cambio de estado */

/* Lista de eventos (L) */
ListaEventos L;

/* Código de error */
int CodError;

/* Declaración de funciones según la arquitectura */
void Inicializacion(void);
int ManejoTiempoEspacio(void);
void Evento_Llegada(void);
void Evento_Salida(int servidor_id);
void Evento_FinSimulacion(void);
void ActualizarEstudioSistema(void);
void ActualizarCalculoCaracteristicas(void);
void ActualizarListaEventos(TipoEvento tipo, double tiempo, int servidor_id);
double PercentilContinuaGeneral(double lambda);
void GeneradorReporte(void);
int SimuladorPrincipal(void);

/* Funciones auxiliares para la lista de eventos */
void InsertarEvento(TipoEvento tipo, double tiempo, int servidor_id);
Evento ExtraerProximoEvento(void);
void InicializarListaEventos(void);

/* Funciones matemáticas y de generación de números aleatorios */
double generar_exponencial(double lambda);
double generar_geometrica(double p);
double factorial(int n);
double potencia(double base, int exponente);
double calcular_prob_teorica(int n);
double calcular_pb_teorico(void);

/* Función principal */
int main(void)
{
    /* Inicializacion del generador de numeros aleatorios */
    srand(time(NULL));
    
    printf("Simulador del sistema Geom/Geom/m/N con Arquitectura de Eventos Discretos\n");
    printf("=================================================================\n\n");
    
    /* Configuración de parámetros del sistema */
    N = 5;                    /* Capacidad del sistema */
    m = 2;                    /* Numero de servidores */
    p = 0.3;                  /* Probabilidad de llegada */
    s = 0.4;                  /* Probabilidad de servicio */
    tiempo_simulacion = 10000.0; /* Tiempo total de simulacion */
    
    printf("Parametros del sistema:\n");
    printf("- Capacidad (N): %d\n", N);
    printf("- Servidores (m): %d\n", m);
    printf("- Prob. llegada (p): %.3f\n", p);
    printf("- Prob. servicio (s): %.3f\n", s);
    printf("- Tiempo simulacion: %.2f\n\n", tiempo_simulacion);
    
    /* Ejecutar simulacion */
    CodError = SimuladorPrincipal();
    
    if (CodError == 0) {
        printf("Simulacion completada exitosamente.\n\n");
        GeneradorReporte();
    } else {
        printf("Error durante la simulacion. Codigo: %d\n", CodError);
    }
    
    return CodError;
}

/* PROCEDIMIENTO INICIALIZACIÓN */
void Inicializacion(void)
{
    int i;
    
    /* T - Valores iniciales tiempo-espacio */
    tiempo_actual = 0.0;
    ultimo_cambio_estado = 0.0;
    
    /* X - Valores iniciales estudio del sistema */
    estado_actual = 0;
    servidores_ocupados = 0;
    cola_length = 0;
    
    /* C - Valores iniciales características del sistema */
    clientes_perdidos = 0;
    clientes_llegados = 0;
    clientes_atendidos = 0;
    
    /* Inicializar tiempo en estados */
    for (i = 0; i <= N; i++) {
        tiempo_en_estado[i] = 0;
    }
    
    /* L - Valores iniciales lista de eventos */
    InicializarListaEventos();
    
    /* Programar primera llegada */
    double tiempo_primera_llegada = generar_geometrica(p);
    InsertarEvento(LLEGADA, tiempo_primera_llegada, -1);
    
    /* Programar fin de simulacion */
    InsertarEvento(FIN_SIMULACION, tiempo_simulacion, -1);
    
    printf("Inicializacion completada.\n");
    printf("Primera llegada programada en t = %.4f\n", tiempo_primera_llegada);
    printf("Fin de simulacion programado en t = %.2f\n\n", tiempo_simulacion);
}

/* FUNCIÓN ManejoTiempoEspacio */
int ManejoTiempoEspacio(void)
{
    if (L.num_eventos == 0) {
        return -1; /* No hay eventos */
    }
    
    /* Extraer próximo evento */
    Evento proximo = ExtraerProximoEvento();
    
    /* Actualizar tiempo-espacio (reloj) */
    tiempo_actual = proximo.tiempo;
    
    /* Retornar tipo de evento */
    return (int)proximo.tipo;
}

/* FUNCIÓN EVENTO_LLEGADA */
void Evento_Llegada(void)
{
    /* Actualizar estudio del sistema */
    ActualizarEstudioSistema();
    
    clientes_llegados++;
    
    /* Verificar si hay espacio en el sistema */
    if (estado_actual < N) {
        /* Cliente entra al sistema */
        estado_actual++;
        
        /* Si hay servidores disponibles, iniciar servicio */
        if (servidores_ocupados < m) {
            servidores_ocupados++;
            /* Programar salida de este cliente */
            double tiempo_servicio = generar_geometrica(s);
            InsertarEvento(SALIDA, tiempo_actual + tiempo_servicio, servidores_ocupados);
        }
    } else {
        /* Cliente bloqueado */
        clientes_perdidos++;
    }
    
    /* Actualizar características del sistema */
    ActualizarCalculoCaracteristicas();
    
    /* Programar próxima llegada */
    double tiempo_proxima_llegada = generar_geometrica(p);
    InsertarEvento(LLEGADA, tiempo_actual + tiempo_proxima_llegada, -1);
}

/* FUNCIÓN EVENTO_SALIDA */
void Evento_Salida(int servidor_id)
{
    /* Actualizar estudio del sistema */
    ActualizarEstudioSistema();
    
    if (estado_actual > 0) {
        estado_actual--;
        clientes_atendidos++;
        
        /* Si hay clientes en cola, uno pasa a servicio */
        if (estado_actual >= m) {
            /* Programar salida del próximo cliente */
            double tiempo_servicio = generar_geometrica(s);
            InsertarEvento(SALIDA, tiempo_actual + tiempo_servicio, servidor_id);
        } else {
            /* Servidor queda libre */
            servidores_ocupados--;
        }
    }
    
    /* Actualizar características del sistema */
    ActualizarCalculoCaracteristicas();
}

/* FUNCIÓN EVENTO_FIN_SIMULACION */
void Evento_FinSimulacion(void)
{
    ActualizarEstudioSistema();
    printf("Simulacion terminada en t = %.2f\n", tiempo_actual);
}

/* Actualizar estudio del sistema */
void ActualizarEstudioSistema(void)
{
    /* Acumular tiempo en el estado actual */
    if (tiempo_actual > ultimo_cambio_estado) {
        double tiempo_transcurrido = tiempo_actual - ultimo_cambio_estado;
        tiempo_en_estado[estado_actual] += (long long)(tiempo_transcurrido * 1000); /* Multiplicar por 1000 para precisión */
        ultimo_cambio_estado = tiempo_actual;
    }
    
    /* Actualizar longitud de cola */
    cola_length = (estado_actual > m) ? (estado_actual - m) : 0;
}

/* Actualizar cálculo de características */
void ActualizarCalculoCaracteristicas(void)
{
    /* Las características se actualizan automáticamente con los contadores */
    /* Esta función se puede usar para cálculos más complejos si es necesario */
}

/* Insertar evento en la lista */
void InsertarEvento(TipoEvento tipo, double tiempo, int servidor_id)
{
    if (L.num_eventos >= MAX_EVENTOS) {
        printf("Error: Lista de eventos llena\n");
        return;
    }
    
    /* Crear nuevo evento */
    Evento nuevo_evento;
    nuevo_evento.tiempo = tiempo;
    nuevo_evento.tipo = tipo;
    nuevo_evento.servidor_id = servidor_id;
    
    /* Insertar en orden cronológico */
    int i = L.num_eventos;
    while (i > 0 && L.eventos[i-1].tiempo > tiempo) {
        L.eventos[i] = L.eventos[i-1];
        i--;
    }
    L.eventos[i] = nuevo_evento;
    L.num_eventos++;
}

/* Extraer próximo evento */
Evento ExtraerProximoEvento(void)
{
    Evento evento = L.eventos[0];
    
    /* Mover eventos hacia adelante */
    for (int i = 0; i < L.num_eventos - 1; i++) {
        L.eventos[i] = L.eventos[i + 1];
    }
    L.num_eventos--;
    
    return evento;
}

/* Inicializar lista de eventos */
void InicializarListaEventos(void)
{
    L.num_eventos = 0;
}

/* Generar número aleatorio con distribución geométrica */
double generar_geometrica(double p)
{
    /* Tiempo hasta el próximo evento en distribución geométrica */
    double u = (double)rand() / RAND_MAX;
    return -log(1.0 - u) / (-log(1.0 - p));
}

/* Función percentil continua general */
double PercentilContinuaGeneral(double lambda)
{
    double u = (double)rand() / RAND_MAX;
    return -log(1.0 - u) / lambda;
}

/* PROCEDIMIENTO GeneradorReporte */
void GeneradorReporte(void)
{
    printf("=== REPORTE DE RESULTADOS DE LA SIMULACION ===\n\n");
    
    /* Calcular probabilidades de estado */
    printf("Distribucion de estados:\n");
    printf("Estado n\tTiempo relativo\tProbabilidad\n");
    
    double tiempo_total = tiempo_simulacion * 1000; /* Ajustar por factor de precisión */
    
    for (int i = 0; i <= N; i++) {
        double prob_simulada = (double)tiempo_en_estado[i] / tiempo_total;
        printf("%d\t\t%.4f\t\t%.6f\n", i, prob_simulada, prob_simulada);
    }
    
    printf("\n=== ESTADISTICAS GENERALES ===\n");
    printf("Tiempo total de simulacion: %.2f\n", tiempo_simulacion);
    printf("Clientes llegados: %lld\n", clientes_llegados);
    printf("Clientes atendidos: %lld\n", clientes_atendidos);
    printf("Clientes perdidos: %lld\n", clientes_perdidos);
    
    if (clientes_llegados > 0) {
        printf("Probabilidad de bloqueo: %.6f\n", (double)clientes_perdidos / clientes_llegados);
    }
    
    /* Número promedio en el sistema */
    double promedio_sistema = 0.0;
    for (int i = 0; i <= N; i++) {
        promedio_sistema += i * ((double)tiempo_en_estado[i] / tiempo_total);
    }
    printf("Numero promedio en el sistema: %.4f\n", promedio_sistema);
    
    /* Utilización del sistema */
    double utilizacion = 1.0 - ((double)tiempo_en_estado[0] / tiempo_total);
    printf("Utilizacion del sistema: %.2f%%\n", utilizacion * 100);
    
    /* Throughput */
    if (tiempo_simulacion > 0) {
        double throughput = (double)clientes_atendidos / tiempo_simulacion;
        printf("Throughput: %.6f clientes/unidad de tiempo\n", throughput);
    }
    
    /* Comparación con teoría */
    printf("\n=== COMPARACION CON TEORIA ===\n");
    printf("Estado n\tP_n simulada\tP_n teorica\tDiferencia\n");
    
    for (int i = 0; i <= N; i++) {
        double prob_simulada = (double)tiempo_en_estado[i] / tiempo_total;
        double prob_teorica = calcular_prob_teorica(i);
        double diferencia = fabs(prob_simulada - prob_teorica);
        
        printf("%d\t\t%.6f\t\t%.6f\t\t%.6f\n", 
               i, prob_simulada, prob_teorica, diferencia);
    }
    
    if (clientes_llegados > 0) {
        double pb_simulada = (double)clientes_perdidos / clientes_llegados;
        double pb_teorica = calcular_pb_teorico();
        
        printf("\nProbabilidad de bloqueo:\n");
        printf("P_b simulada: %.6f\n", pb_simulada);
        printf("P_b teorica:  %.6f\n", pb_teorica);
        printf("Diferencia:   %.6f\n", fabs(pb_simulada - pb_teorica));
    }
}

/* FUNCIÓN SimuladorPrincipal */
int SimuladorPrincipal(void)
{
    CodError = 0;
    
    /* Inicialización */
    Inicializacion();
    
    printf("Iniciando simulacion...\n\n");
    
    /* Bucle principal de simulación */
    while (CodError == 0) {
        /* Manejo del tiempo-espacio */
        int tipo_evento = ManejoTiempoEspacio();
        
        if (tipo_evento == -1) {
            CodError = -1; /* No hay más eventos */
            break;
        }
        
        /* Seleccionar y ejecutar evento */
        switch (tipo_evento) {
            case LLEGADA:
                Evento_Llegada();
                break;
            case SALIDA:
                Evento_Salida(1); /* Pasar ID del servidor */
                break;
            case FIN_SIMULACION:
                Evento_FinSimulacion();
                CodError = 0; /* Terminación normal */
                return 0;
            default:
                printf("Tipo de evento desconocido: %d\n", tipo_evento);
                CodError = -2;
                break;
        }
        
        /* Verificar condición de terminación */
        if (tiempo_actual >= tiempo_simulacion) {
            break;
        }
    }
    
    return CodError;
}

/* Funciones auxiliares matematicas (mantenidas del código original) */
double factorial(int n)
{
    if (n <= 1) return 1.0;
    double result = 1.0;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double potencia(double base, int exponente)
{
    if (exponente == 0) return 1.0;
    if (exponente < 0) return 1.0 / potencia(base, -exponente);
    
    double result = 1.0;
    for (int i = 0; i < exponente; i++) {
        result *= base;
    }
    return result;
}

double calcular_prob_teorica(int n)
{
    if (n < 0 || n > N) return 0.0;
    
    double rho = p * (1.0 - s) / (s * (1.0 - p));
    double prob;
    
    if (n <= m) {
        prob = potencia(rho, n) / factorial(n);
    } else {
        prob = potencia(rho, n) / (factorial(m) * potencia(m, n - m));
    }
    
    /* Normalización */
    double suma = 0.0;
    for (int i = 0; i <= N; i++) {
        double temp;
        if (i <= m) {
            temp = potencia(rho, i) / factorial(i);
        } else {
            temp = potencia(rho, i) / (factorial(m) * potencia(m, i - m));
        }
        suma += temp;
    }
    
    return prob / suma;
}

double calcular_pb_teorico(void)
{
    double pN = calcular_prob_teorica(N);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}