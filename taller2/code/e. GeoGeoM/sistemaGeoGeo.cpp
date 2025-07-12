#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_CAPACITY 1000
#define MAX_SERVERS 10

int N, m, num_slots;
double p, s;

int estado_actual, servidores_ocupados, cola_length;
long long tiempo_en_estado[MAX_CAPACITY + 1];
long long clientes_perdidos, clientes_llegados, slot_actual;

/* Archivos de salida */
FILE *f_estadisticas, *f_teorico, *f_resumen;

void inicializar(void);
void simular_slot(void);
void calcular_estadisticas(void);
void comparar_teoria(void);
double calcular_prob_teorica(int n);
double calcular_pb_teorico(void);
int generar_bernoulli(double prob);
int generar_binomial(int n, double prob);
void imprimir_resultados(void);

double factorial(int n);
double potencia(double base, int exponente);
double min_double(double a, double b);

int main(void) {
    FILE *fparam = fopen("paramGeo.txt", "r");  // Leer parámetros de archivo
    if (fparam == NULL) {
        printf("Error: No se pudo abrir 'parametros.txt'\n");
        return 1;
    }

    if (fscanf(fparam, "%d %d %lf %lf %d", &N, &m, &p, &s, &num_slots) != 5) {
        printf("Error: Formato incorrecto en 'parametros.txt'\n");
        fclose(fparam);
        return 1;
    }
    fclose(fparam);

    srand(time(NULL));

    printf("Simulador del sistema Geom/Geom/m/N\n");
    printf("Parametros: N=%d, m=%d, p=%.2f, s=%.2f\n", N, m, p, s);
    printf("Numero de slots: %d\n\n", num_slots);

    /* Abrir archivos de salida */
    f_estadisticas = fopen("estadisticas.txt", "w");
    f_teorico = fopen("comparacion_teorica.txt", "w");
    f_resumen = fopen("resumen_resultados.txt", "w");

    if (!f_estadisticas || !f_teorico || !f_resumen) {
        printf("Error: No se pudieron crear archivos de salida.\n");
        return 1;
    }

    inicializar();

    for (slot_actual = 0; slot_actual < num_slots; slot_actual++) {
        simular_slot();
        if ((slot_actual + 1) % 100000 == 0) {
            printf("Progreso: %lld/%d slots completados\n", slot_actual + 1, num_slots);
        }
    }

    calcular_estadisticas();
    comparar_teoria();
    imprimir_resultados();

    fclose(f_estadisticas);
    fclose(f_teorico);
    fclose(f_resumen);

    printf("Simulacion completada. Resultados guardados en archivos de texto.\n");
    return 0;
}

void inicializar(void) {
    estado_actual = servidores_ocupados = cola_length = 0;
    clientes_perdidos = clientes_llegados = slot_actual = 0;
    for (int i = 0; i <= N; i++) tiempo_en_estado[i] = 0;
}

void simular_slot(void) {
    int llegada = generar_bernoulli(p);
    int salidas = generar_binomial(servidores_ocupados, s);

    tiempo_en_estado[estado_actual]++;
    if (llegada) clientes_llegados++;

    estado_actual -= salidas;
    if (estado_actual < 0) estado_actual = 0;

    if (llegada) {
        if (estado_actual < N) estado_actual++;
        else clientes_perdidos++;
    }

    servidores_ocupados = (estado_actual < m) ? estado_actual : m;
    cola_length = (estado_actual > m) ? (estado_actual - m) : 0;
}

void calcular_estadisticas(void) {
    fprintf(f_estadisticas, "=== RESULTADOS DE LA SIMULACION ===\n\n");
    fprintf(f_estadisticas, "Estado\tProbabilidad\n");
    for (int i = 0; i <= N; i++) {
        double prob = (double)tiempo_en_estado[i] / num_slots;
        fprintf(f_estadisticas, "%d\t%.6f\n", i, prob);
    }

    double pb_sim = (double)clientes_perdidos / clientes_llegados;
    fprintf(f_estadisticas, "\nClientes llegados: %lld\n", clientes_llegados);
    fprintf(f_estadisticas, "Clientes perdidos: %lld\n", clientes_perdidos);
    fprintf(f_estadisticas, "Probabilidad de bloqueo simulada: %.6f\n", pb_sim);
}

void comparar_teoria(void) {
    fprintf(f_teorico, "=== COMPARACION CON TEORIA ===\n\n");
    fprintf(f_teorico, "Estado\tP_simulada\tP_teorica\tDiferencia\n");

    for (int i = 0; i <= N; i++) {
        double sim = (double)tiempo_en_estado[i] / num_slots;
        double teor = calcular_prob_teorica(i);
        fprintf(f_teorico, "%d\t%.6f\t%.6f\t%.6f\n", i, sim, teor, fabs(sim - teor));
    }

    double pb_sim = (double)clientes_perdidos / clientes_llegados;
    double pb_teo = calcular_pb_teorico();
    fprintf(f_teorico, "\nP_b simulada: %.6f\n", pb_sim);
    fprintf(f_teorico, "P_b teorica:  %.6f\n", pb_teo);
    fprintf(f_teorico, "Diferencia:   %.6f\n", fabs(pb_sim - pb_teo));
}

void imprimir_resultados(void) {
    fprintf(f_resumen, "=== RESUMEN DE RESULTADOS ===\n");
    fprintf(f_resumen, "Slots simulados: %d\n", num_slots);
    fprintf(f_resumen, "Utilización: %.2f%%\n", 
            ((double)(num_slots - tiempo_en_estado[0]) / num_slots) * 100);

    double promedio = 0.0;
    for (int i = 0; i <= N; i++) {
        promedio += i * ((double)tiempo_en_estado[i] / num_slots);
    }
    fprintf(f_resumen, "Promedio de clientes en el sistema: %.4f\n", promedio);
    fprintf(f_resumen, "Throughput: %.6f clientes/slot\n", 
            (double)(clientes_llegados - clientes_perdidos) / num_slots);
}

double calcular_prob_teorica(int n) {
    if (n < 0 || n > N) return 0.0;
    double rho = p * (1.0 - s) / (s * (1.0 - p));
    double prob;
    if (n <= m) prob = potencia(rho, n) / factorial(n);
    else prob = potencia(rho, n) / (factorial(m) * potencia(m, n - m));

    double suma = 0.0;
    for (int i = 0; i <= N; i++) {
        double temp = (i <= m) ? potencia(rho, i) / factorial(i) :
                                 potencia(rho, i) / (factorial(m) * potencia(m, i - m));
        suma += temp;
    }

    return prob / suma;
}

double calcular_pb_teorico(void) {
    double pN = calcular_prob_teorica(N);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}

int generar_bernoulli(double prob) {
    return (rand() / (double)RAND_MAX) < prob ? 1 : 0;
}

int generar_binomial(int n, double prob) {
    int count = 0;
    for (int i = 0; i < n; i++) if (generar_bernoulli(prob)) count++;
    return count;
}

double factorial(int n) {
    if (n <= 1) return 1.0;
    double r = 1.0;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}

double potencia(double base, int exponente) {
    if (exponente == 0) return 1.0;
    if (exponente < 0) return 1.0 / potencia(base, -exponente);
    double result = 1.0;
    for (int i = 0; i < exponente; i++) result *= base;
    return result;
}

double min_double(double a, double b) {
    return (a < b) ? a : b;
}
