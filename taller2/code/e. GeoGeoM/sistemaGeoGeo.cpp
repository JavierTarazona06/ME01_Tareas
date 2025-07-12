#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../SimulacionDeColas/lcgrand.cpp"  /* Encabezado para el generador de numeros aleatorios */

// Parámetros máximos
#define MAX_CAPACITY 1000   // Capacidad máxima del sistema (N)
#define MAX_SERVERS 10      // Número máximo de servidores (m)

// === Parámetros del sistema ===
int N;             // Capacidad máxima del sistema (número total de clientes que puede contener, incluyendo en servicio y en cola)
int m;             // Número de servidores (máximo número de clientes que pueden ser atendidos simultáneamente)
int num_slots;     // Número de slots de tiempo para la simulación (iteraciones)
double p;          // Probabilidad de llegada de un cliente en un slot
double s;          // Probabilidad de que un cliente termine su servicio en un slot

// Semilla para reproducibilidad
int seed = 42;

// Estado del sistema y estadísticas
int estado_actual, servidores_ocupados, cola_length;
long long tiempo_en_estado[MAX_CAPACITY + 1]; // Tiempo acumulado en cada estado
long long clientes_perdidos, clientes_llegados, slot_actual;

// Archivos de salida
FILE *f_estadisticas, *f_teorico, *f_resumen;

// Declaración de funciones principales
void inicializar(void);              // Inicializa las variables del sistema
void simular_slot(void);            // Simula un slot de tiempo
void calcular_estadisticas(void);   // Calcula métricas simuladas
void comparar_teoria(void);         // Compara métricas simuladas vs teóricas
void reportar_resultados(void);     // Imprime resumen general de resultados

// Funciones auxiliares matemáticas y de generación aleatoria
double calcular_prob_teorica(int n);     // P_n teórica
double calcular_pb_teorico(void);        // Probabilidad de bloqueo teórica
int generar_bernoulli(double prob);      // Genera 0 o 1 con probabilidad "prob"
int generar_binomial(int n, double prob);// Genera una variable binomial
double factorial(int n);                 
double potencia(double base, int exponente);
double min_double(double a, double b);     

// =============================
// FUNCIÓN PRINCIPAL DEL PROGRAMA
// =============================
int main(void) {
    // Leer parámetros desde archivo
    FILE *fparam = fopen("paramGeo.txt", "r");
    if (fparam == NULL) {
        printf("Error: No se pudo abrir 'parametros.txt'\n");
        return 1;
    }

    // Leer N, m, p, s, y número de slots desde el archivo
    if (fscanf(fparam, "%d %d %lf %lf %d", &N, &m, &p, &s, &num_slots) != 5) {
        printf("Error: Formato incorrecto en 'parametros.txt'\n");
        fclose(fparam);
        return 1;
    }
    fclose(fparam);

    // Imprimir parámetros de simulación
    printf("Simulador del sistema (Geom/Geom/m):(FIFO/N/+oo)\n");
    printf("Parametros: N=%d, m=%d, p=%.2f, s=%.2f\n", N, m, p, s);
    printf("Numero de slots: %d\n\n", num_slots);

    // Abrir archivos de salida
    f_estadisticas = fopen("estadisticas.txt", "w");
    f_teorico = fopen("comparacion_teorica.txt", "w");
    f_resumen = fopen("resumen_resultados.txt", "w");

    if (!f_estadisticas || !f_teorico || !f_resumen) {
        printf("Error: No se pudieron crear archivos de salida.\n");
        return 1;
    }

    // Inicializar variables
    inicializar();

    // Bucle principal de simulación
    for (slot_actual = 0; slot_actual < num_slots; slot_actual++) {
        simular_slot();  // Procesar un slot
    }

    // Cálculo de estadísticas y reporte final
    calcular_estadisticas();
    comparar_teoria();
    reportar_resultados();

    // Cierre de archivos
    fclose(f_estadisticas);
    fclose(f_teorico);
    fclose(f_resumen);

    printf("Simulacion completada. Resultados guardados en archivos de texto.\n");
    return 0;
}

// Inicializa el estado del sistema y contadores
void inicializar(void) {
    estado_actual = servidores_ocupados = cola_length = 0;
    clientes_perdidos = clientes_llegados = slot_actual = 0;
    for (int i = 0; i <= N; i++) tiempo_en_estado[i] = 0;
}

// Simula la evolución del sistema en un slot
void simular_slot(void) {
    int llegada = generar_bernoulli(p);               // ¿Hay llegada?
    int salidas = generar_binomial(servidores_ocupados, s); // ¿Cuántas salidas?

    // Registrar tiempo en estado actual
    tiempo_en_estado[estado_actual]++;

    // Contar llegada
    if (llegada) clientes_llegados++;

    // Actualizar número de clientes en el sistema (después de salidas)
    estado_actual -= salidas;
    if (estado_actual < 0) estado_actual = 0;

    // Procesar llegada si hubo
    if (llegada) {
        if (estado_actual < N) estado_actual++;  // Cliente entra
        else clientes_perdidos++;                // Cliente bloqueado
    }

    // Actualizar número de servidores ocupados y tamaño de cola
    servidores_ocupados = (estado_actual < m) ? estado_actual : m;
    cola_length = (estado_actual > m) ? (estado_actual - m) : 0;
}

// Imprime estadísticas simuladas en archivo
void calcular_estadisticas(void) {
    fprintf(f_estadisticas, "=== RESULTADOS DE LA SIMULACION ===\n\n");
    fprintf(f_estadisticas, "Estado\tProbabilidad\n");
    for (int i = 0; i <= N; i++) {
        double prob = (double)tiempo_en_estado[i] / num_slots;
        fprintf(f_estadisticas, "%d\t%.6f\n", i, prob);
    }

    // Calcular y reportar probabilidad de bloqueo simulada
    double pb_sim = (double)clientes_perdidos / clientes_llegados;
    fprintf(f_estadisticas, "\nClientes llegados: %lld\n", clientes_llegados);
    fprintf(f_estadisticas, "Clientes perdidos: %lld\n", clientes_perdidos);
    fprintf(f_estadisticas, "Probabilidad de bloqueo simulada: %.6f\n", pb_sim);
}

// Compara distribución simulada con distribución teórica
void comparar_teoria(void) {
    fprintf(f_teorico, "=== COMPARACION CON TEORIA ===\n\n");
    fprintf(f_teorico, "Estado\tP_simulada\tP_teorica\tDiferencia\n");

    for (int i = 0; i <= N; i++) {
        double sim = (double)tiempo_en_estado[i] / num_slots;
        double teor = calcular_prob_teorica(i);
        fprintf(f_teorico, "%d\t%.6f\t%.6f\t%.6f\n", i, sim, teor, fabs(sim - teor));
    }

    // Comparar probabilidad de bloqueo
    double pb_sim = (double)clientes_perdidos / clientes_llegados;
    double pb_teo = calcular_pb_teorico();
    fprintf(f_teorico, "\nP_b simulada: %.6f\n", pb_sim);
    fprintf(f_teorico, "P_b teorica:  %.6f\n", pb_teo);
    fprintf(f_teorico, "Diferencia:   %.6f\n", fabs(pb_sim - pb_teo));
}

// Imprime resumen general del comportamiento del sistema
void reportar_resultados(void) {
    fprintf(f_resumen, "=== RESUMEN DE RESULTADOS ===\n");
    fprintf(f_resumen, "Slots simulados: %d\n", num_slots);

    // Utilización = 1 - tiempo en estado 0
    fprintf(f_resumen, "Utilización: %.2f%%\n", 
            ((double)(num_slots - tiempo_en_estado[0]) / num_slots) * 100);

    // Número promedio de clientes en el sistema
    double promedio = 0.0;
    for (int i = 0; i <= N; i++) {
        promedio += i * ((double)tiempo_en_estado[i] / num_slots);
    }
    fprintf(f_resumen, "Promedio de clientes en el sistema: %.4f\n", promedio);

    // Throughput: clientes atendidos por slot
    fprintf(f_resumen, "Throughput: %.6f clientes/slot\n", 
            (double)(clientes_llegados - clientes_perdidos) / num_slots);
}

// FUNCIONES AUXILIARES

// Calcula la probabilidad teórica P_n
double calcular_prob_teorica(int n) {
    if (n < 0 || n > N) return 0.0;
    double rho = p * (1.0 - s) / (s * (1.0 - p));
    double prob;
    if (n <= m) prob = potencia(rho, n) / factorial(n);
    else prob = potencia(rho, n) / (factorial(m) * potencia(m, n - m));

    // Normalizar
    double suma = 0.0;
    for (int i = 0; i <= N; i++) {
        double temp = (i <= m) ? potencia(rho, i) / factorial(i) :
                                 potencia(rho, i) / (factorial(m) * potencia(m, i - m));
        suma += temp;
    }

    return prob / suma;
}

// Calcula la probabilidad teórica de bloqueo
double calcular_pb_teorico(void) {
    double pN = calcular_prob_teorica(N);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}

// Genera 1 con probabilidad "prob", 0 en otro caso
int generar_bernoulli(double prob) {
    return lcgrand(seed) < prob ? 1 : 0;
}


// Genera una variable binomial con n ensayos y probabilidad prob
int generar_binomial(int n, double prob) {
    int count = 0;
    for (int i = 0; i < n; i++) if (generar_bernoulli(prob)) count++;
    return count;
}

// Calcula factorial de n
double factorial(int n) {
    if (n <= 1) return 1.0;
    double r = 1.0;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}

// Calcula potencia base^exponente
double potencia(double base, int exponente) {
    if (exponente == 0) return 1.0;
    if (exponente < 0) return 1.0 / potencia(base, -exponente);
    double result = 1.0;
    for (int i = 0; i < exponente; i++) result *= base;
    return result;
}

// Devuelve el mínimo entre dos valores tipo double
double min_double(double a, double b) {
    return (a < b) ? a : b;
}
