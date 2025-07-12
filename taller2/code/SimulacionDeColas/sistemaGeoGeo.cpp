/* Simulador del sistema de colas Geom/Geom/m/N */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_CAPACITY 1000  /* Capacidad maxima del sistema */
#define MAX_SERVERS 10     /* Numero maximo de servidores */

/* Variables globales del sistema */
int N;                     /* Capacidad del sistema */
int m;                     /* Numero de servidores */
double p;                  /* Probabilidad de llegada en cada slot */
double s;                  /* Probabilidad de completar servicio en cada slot */
int num_slots;             /* Numero total de slots a simular */

/* Variables de estado */
int estado_actual;         /* Numero de clientes en el sistema */
int servidores_ocupados;   /* Numero de servidores ocupados */
int cola_length;           /* Longitud de la cola */

/* Contadores estadisticos */
long long tiempo_en_estado[MAX_CAPACITY + 1];  /* Tiempo en cada estado */
long long clientes_perdidos;                   /* Clientes que no pudieron entrar */
long long clientes_llegados;                   /* Total de clientes que intentaron llegar */
long long slot_actual;                         /* Slot actual de tiempo */

/* Funciones del simulador */
void inicializar(void);
void simular_slot(void);
void calcular_estadisticas(void);
void comparar_teoria(void);
double calcular_prob_teorica(int n);
double calcular_pb_teorico(void);
int generar_bernoulli(double prob);
int generar_binomial(int n, double prob);
void imprimir_resultados(void);

/* Funciones auxiliares matematicas */
double factorial(int n);
double potencia(double base, int exponente);
double min_double(double a, double b);

int main(void)
{
    /* Inicializacion del generador de numeros aleatorios */
    srand(time(NULL));
    
    /* Parametros del sistema */
    N = 5;              /* Capacidad del sistema */
    m = 5;              /* Numero de servidores */
    p = 0.3;            /* Probabilidad de llegada */
    s = 0.4;            /* Probabilidad de servicio */
    num_slots = 10000000; /* Numero de slots a simular */
    
    printf("Simulador del sistema Geom/Geom/m/N\n");
    printf("Parametros: N=%d, m=%d, p=%.2f, s=%.2f\n", N, m, p, s);
    printf("Numero de slots: %d\n\n", num_slots);
    
    /* Ejecutar simulacion */
    inicializar();
    
    printf("Ejecutando simulacion...\n");
    for (slot_actual = 0; slot_actual < num_slots; slot_actual++) {
        simular_slot();
        
        /* Mostrar progreso cada 100,000 slots */
        if ((slot_actual + 1) % 100000 == 0) {
            printf("Progreso: %lld/%d slots completados\n", slot_actual + 1, num_slots);
        }
    }
    
    printf("Simulacion completada.\n\n");
    
    /* Calcular y mostrar resultados */
    calcular_estadisticas();
    comparar_teoria();
    imprimir_resultados();
    
    return 0;
}

void inicializar(void)
{
    int i;
    
    /* Inicializar estado del sistema */
    estado_actual = 0;
    servidores_ocupados = 0;
    cola_length = 0;
    
    /* Inicializar contadores */
    clientes_perdidos = 0;
    clientes_llegados = 0;
    slot_actual = 0;
    
    /* Inicializar tiempo en estados */
    for (i = 0; i <= N; i++) {
        tiempo_en_estado[i] = 0;
    }
}

void simular_slot(void)
{
    int llegada, salidas, nuevos_en_servicio;
    
    /* Registrar tiempo en estado actual */
    tiempo_en_estado[estado_actual]++;
    
    /* Generar llegada (proceso Bernoulli) */
    llegada = generar_bernoulli(p);
    if (llegada) {
        clientes_llegados++;
    }
    
    /* Generar salidas (cada servidor ocupado puede completar servicio) */
    salidas = generar_binomial(servidores_ocupados, s);
    
    /* Actualizar estado del sistema */
    
    /* Primero, procesar salidas */
    estado_actual -= salidas;
    if (estado_actual < 0) estado_actual = 0;
    
    /* Luego, procesar llegadas */
    if (llegada) {
        if (estado_actual < N) {
            estado_actual++;
        } else {
            /* Cliente bloqueado */
            clientes_perdidos++;
        }
    }
    
    /* Actualizar numero de servidores ocupados */
    servidores_ocupados = (estado_actual < m) ? estado_actual : m;
    
    /* Actualizar longitud de cola */
    cola_length = (estado_actual > m) ? (estado_actual - m) : 0;
}

void calcular_estadisticas(void)
{
    printf("=== RESULTADOS DE LA SIMULACION ===\n\n");
    printf("Distribucion de estados (simulada):\n");
    printf("Estado n\tProbabilidad\tTiempo relativo\n");
    
    for (int i = 0; i <= N; i++) {
        double prob_simulada = (double)tiempo_en_estado[i] / num_slots;
        printf("%d\t\t%.6f\t\t%.2f%%\n", i, prob_simulada, prob_simulada * 100);
    }
    
    printf("\nClientes llegados: %lld\n", clientes_llegados);
    printf("Clientes perdidos: %lld\n", clientes_perdidos);
    printf("Probabilidad de bloqueo simulada: %.6f\n", 
           (double)clientes_perdidos / clientes_llegados);
}

void comparar_teoria(void)
{
    printf("\n=== COMPARACION CON TEORIA ===\n\n");
    printf("Estado n\tP_n simulada\tP_n teorica\tDiferencia\n");
    
    for (int i = 0; i <= N; i++) {
        double prob_simulada = (double)tiempo_en_estado[i] / num_slots;
        double prob_teorica = calcular_prob_teorica(i);
        double diferencia = fabs(prob_simulada - prob_teorica);
        
        printf("%d\t\t%.6f\t\t%.6f\t\t%.6f\n", 
               i, prob_simulada, prob_teorica, diferencia);
    }
    
    double pb_simulada = (double)clientes_perdidos / clientes_llegados;
    double pb_teorica = calcular_pb_teorico();
    
    printf("\nProbabilidad de bloqueo:\n");
    printf("P_b simulada: %.6f\n", pb_simulada);
    printf("P_b teorica:  %.6f\n", pb_teorica);
    printf("Diferencia:   %.6f\n", fabs(pb_simulada - pb_teorica));
}

double calcular_prob_teorica(int n)
{
    /* Implementacion simplificada basada en las ecuaciones del libro */
    /* Para el caso general Geom/Geom/m/N */
    
    if (n < 0 || n > N) return 0.0;
    
    double rho = p * (1.0 - s) / (s * (1.0 - p));
    double prob;
    
    if (n <= m) {
        /* Estados con servidores no saturados */
        prob = potencia(rho, n) / factorial(n);
    } else {
        /* Estados con servidores saturados */
        prob = potencia(rho, n) / (factorial(m) * potencia(m, n - m));
    }
    
    /* Normalizacion (simplificada) */
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
    /* P_b = P_N * s_0, donde s_0 = (1-s)^m */
    double pN = calcular_prob_teorica(N);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}

int generar_bernoulli(double prob)
{
    return (rand() / (double)RAND_MAX) < prob ? 1 : 0;
}

int generar_binomial(int n, double prob)
{
    int count = 0;
    for (int i = 0; i < n; i++) {
        if (generar_bernoulli(prob)) {
            count++;
        }
    }
    return count;
}

void imprimir_resultados(void)
{
    printf("\n=== RESUMEN DE RESULTADOS ===\n");
    printf("Numero total de slots simulados: %d\n", num_slots);
    printf("Utilizacion del sistema: %.2f%%\n", 
           ((double)(num_slots - tiempo_en_estado[0]) / num_slots) * 100);
    
    /* Numero promedio en el sistema */
    double promedio_sistema = 0.0;
    for (int i = 0; i <= N; i++) {
        promedio_sistema += i * ((double)tiempo_en_estado[i] / num_slots);
    }
    printf("Numero promedio en el sistema: %.4f\n", promedio_sistema);
    
    /* Throughput */
    double throughput = (double)(clientes_llegados - clientes_perdidos) / num_slots;
    printf("Throughput: %.6f clientes/slot\n", throughput);
}

/* Funciones auxiliares matematicas */
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

double min_double(double a, double b)
{
    return (a < b) ? a : b;
}