// Experimentación del Sistema Geom/Geom/m/N
// Análisis de 3 escenarios de prueba con múltiples ejecuciones
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include "../SimulacionDeColas/lcgrand.cpp"

// Parámetros máximos
#define MAX_CAPACITY 1000
#define MAX_SERVERS 10

// Estructura para almacenar resultados de una ejecución
struct ResultadoEjecucion {
    std::vector<double> prob_estados;
    double prob_bloqueo;
    long long clientes_llegados;
    long long clientes_perdidos;
    double utilizacion;
    double num_promedio_clientes;
    double throughput;
};

// Estructura para parámetros del escenario
struct EscenarioPrueba {
    int N, m, num_slots, num_ejecuciones;
    double p, s;
    std::string nombre;
};

// Variables globales del simulador
int N, m, num_slots;
double p, s;
int seed_base = 42;

// Estado del sistema
int estado_actual, servidores_ocupados, cola_length;
std::vector<long long> tiempo_en_estado;
long long clientes_perdidos, clientes_llegados, slot_actual;

// Declaración de funciones
void inicializar_simulador(void);
void simular_slot(void);
ResultadoEjecucion ejecutar_simulacion(int seed_offset);
std::vector<ResultadoEjecucion> ejecutar_escenario(const EscenarioPrueba& escenario);
void analizar_resultados_escenario(const EscenarioPrueba& escenario, 
                                   const std::vector<ResultadoEjecucion>& resultados);
void generar_reporte_completo(void);

// Funciones matemáticas auxiliares
int generar_bernoulli(double prob);
int generar_binomial(int n, double prob);
double factorial(int n);
double potencia(double base, int exponente);
int min_integer(int a, int b);
long double comb(int n, int k);
long double combEq0(int upper, int lower);
long double a_func(int n, int b, long double p, long double s, int m, bool same_slot);
std::vector<long double> geom_geom_m_N_p_calc(bool same_slot = false);
long double calcular_prob_teorica_geo_geo_m_N(int i, bool same_slot = false);
double calcular_pb_teorico_geo_geo_m_N(void);

// Archivos de salida
FILE *f_reporte;

int main(void) {
    // Leer parámetros base desde archivo
    FILE *fparam = fopen("paramGeo.txt", "r");
    if (fparam == NULL) {
        printf("Error: No se pudo abrir 'paramGeo.txt'\n");
        printf("El archivo debe contener: N m p s num_ejecuciones\n");
        printf("Ejemplo: 5 5 0.3 0.4 30\n");
        return 1;
    }

    int N_base, m_base, num_ejecuciones_base;
    double p_base, s_base;
    
    if (fscanf(fparam, "%d %d %lf %lf %d", &N_base, &m_base, &p_base, &s_base, &num_ejecuciones_base) != 5) {
        printf("Error: Formato incorrecto en 'paramGeo.txt'\n");
        printf("Formato esperado: N m p s num_ejecuciones\n");
        printf("Ejemplo: 5 5 0.3 0.4 30\n");
        fclose(fparam);
        return 1;
    }
    fclose(fparam);

    // Definir los 3 escenarios de prueba (variando número de slots)
    std::vector<EscenarioPrueba> escenarios = {
        // Escenario 1: Simulación corta
        {N_base, m_base, 100000, num_ejecuciones_base, p_base, s_base, "Escenario 1: Simulación Corta (100K slots)"},
        
        // Escenario 2: Simulación media
        {N_base, m_base, 500000, num_ejecuciones_base, p_base, s_base, "Escenario 2: Simulación Media (500K slots)"},
        
        // Escenario 3: Simulación larga
        {N_base, m_base, 2000000, num_ejecuciones_base, p_base, s_base, "Escenario 3: Simulación Larga (2M slots)"}
    };

    printf("=== EXPERIMENTACION SISTEMA GEOM/GEOM/M/N ===\n");
    printf("Parámetros leídos: N=%d, m=%d, p=%.2f, s=%.2f\n", N_base, m_base, p_base, s_base);
    printf("Número de ejecuciones por escenario: %d\n", num_ejecuciones_base);
    printf("Variando número de slots para analizar convergencia...\n");
    printf("Iniciando análisis de %zu escenarios...\n\n", escenarios.size());

    // Abrir archivo de reporte
    f_reporte = fopen("reporte_experimentacion.txt", "w");
    if (!f_reporte) {
        printf("Error: No se pudo crear archivo de reporte.\n");
        return 1;
    }

    fprintf(f_reporte, "=== REPORTE DE EXPERIMENTACION SISTEMA GEOM/GEOM/M/N ===\n");
    fprintf(f_reporte, "OBJETIVO: Analizar convergencia variando número de slots\n");
    fprintf(f_reporte, "PARAMETROS LEIDOS: N=%d, m=%d, p=%.2f, s=%.2f\n", N_base, m_base, p_base, s_base);
    fprintf(f_reporte, "EJECUCIONES POR ESCENARIO: %d\n", num_ejecuciones_base);
    fprintf(f_reporte, "Fecha: %s\n", __DATE__);
    fprintf(f_reporte, "Hora: %s\n\n", __TIME__);

    // Ejecutar cada escenario
    for (size_t i = 0; i < escenarios.size(); i++) {
        printf("Ejecutando %s...\n", escenarios[i].nombre.c_str());
        
        // Establecer parámetros globales
        N = escenarios[i].N;
        m = escenarios[i].m;
        p = escenarios[i].p;
        s = escenarios[i].s;
        num_slots = escenarios[i].num_slots;
        tiempo_en_estado.resize(N + 1);

        // Ejecutar múltiples corridas del escenario
        std::vector<ResultadoEjecucion> resultados = ejecutar_escenario(escenarios[i]);
        
        // Analizar y reportar resultados
        analizar_resultados_escenario(escenarios[i], resultados);
        
        printf("Completado.\n\n");
    }

    fclose(f_reporte);
    printf("Experimentación completada. Resultados en 'reporte_experimentacion.txt'\n");
    return 0;
}

void inicializar_simulador(void) {
    estado_actual = servidores_ocupados = cola_length = 0;
    clientes_perdidos = clientes_llegados = slot_actual = 0;
    std::fill(tiempo_en_estado.begin(), tiempo_en_estado.end(), 0);
}

void simular_slot(void) {
    int llegada = generar_bernoulli(p);
    int salidas = generar_binomial(servidores_ocupados, s);

    tiempo_en_estado[estado_actual]++;

    if (llegada) clientes_llegados++;

    estado_actual -= salidas;
    if (estado_actual < 0) estado_actual = 0;

    if (llegada) {
        if (estado_actual < N) {
            estado_actual++;
        } else {
            clientes_perdidos++;
        }
    }

    servidores_ocupados = (estado_actual < m) ? estado_actual : m;
    cola_length = (estado_actual > m) ? (estado_actual - m) : 0;
}

ResultadoEjecucion ejecutar_simulacion(int seed_offset) {
    inicializar_simulador();
    
    // Usar semilla diferente para cada ejecución
    seed_base = 42 + seed_offset;
    
    for (slot_actual = 0; slot_actual < num_slots; slot_actual++) {
        simular_slot();
    }
    
    ResultadoEjecucion resultado;
    resultado.clientes_llegados = clientes_llegados;
    resultado.clientes_perdidos = clientes_perdidos;
    resultado.prob_bloqueo = (double)clientes_perdidos / clientes_llegados;
    
    // Calcular probabilidades de estado
    resultado.prob_estados.resize(N + 1);
    for (int i = 0; i <= N; i++) {
        resultado.prob_estados[i] = (double)tiempo_en_estado[i] / num_slots;
    }
    
    // Calcular métricas adicionales
    resultado.utilizacion = (double)(num_slots - tiempo_en_estado[0]) / num_slots;
    
    resultado.num_promedio_clientes = 0.0;
    for (int i = 0; i <= N; i++) {
        resultado.num_promedio_clientes += i * resultado.prob_estados[i];
    }
    
    resultado.throughput = (double)(clientes_llegados - clientes_perdidos) / num_slots;
    
    return resultado;
}

std::vector<ResultadoEjecucion> ejecutar_escenario(const EscenarioPrueba& escenario) {
    std::vector<ResultadoEjecucion> resultados;
    resultados.reserve(escenario.num_ejecuciones);
    
    for (int i = 0; i < escenario.num_ejecuciones; i++) {
        resultados.push_back(ejecutar_simulacion(i));
        if ((i + 1) % 10 == 0) {
            printf("  Completadas %d/%d ejecuciones\n", i + 1, escenario.num_ejecuciones);
        }
    }
    
    return resultados;
}

void analizar_resultados_escenario(const EscenarioPrueba& escenario, 
                                   const std::vector<ResultadoEjecucion>& resultados) {
    fprintf(f_reporte, "========================================\n");
    fprintf(f_reporte, "%s\n", escenario.nombre.c_str());
    fprintf(f_reporte, "========================================\n");
    fprintf(f_reporte, "Parámetros: N=%d, m=%d, p=%.2f, s=%.2f\n", 
            escenario.N, escenario.m, escenario.p, escenario.s);
    fprintf(f_reporte, "Slots por ejecución: %d\n", escenario.num_slots);
    fprintf(f_reporte, "Número de ejecuciones: %d\n\n", escenario.num_ejecuciones);
    
    // Calcular estadísticas de probabilidades de estado
    std::vector<double> prob_promedio(N + 1, 0.0);
    std::vector<double> prob_varianza(N + 1, 0.0);
    
    // Promedio
    for (const auto& resultado : resultados) {
        for (int i = 0; i <= N; i++) {
            prob_promedio[i] += resultado.prob_estados[i];
        }
    }
    for (int i = 0; i <= N; i++) {
        prob_promedio[i] /= escenario.num_ejecuciones;
    }
    
    // Varianza
    for (const auto& resultado : resultados) {
        for (int i = 0; i <= N; i++) {
            double diff = resultado.prob_estados[i] - prob_promedio[i];
            prob_varianza[i] += diff * diff;
        }
    }
    for (int i = 0; i <= N; i++) {
        prob_varianza[i] /= (escenario.num_ejecuciones - 1);
    }
    
    // Obtener valores teóricos
    std::vector<long double> prob_teoricas = geom_geom_m_N_p_calc(false);
    double pb_teorico = calcular_pb_teorico_geo_geo_m_N();
    
    // Reporte de función de densidad
    fprintf(f_reporte, "I. FUNCIÓN DE DENSIDAD DE ESTADOS\n");
    fprintf(f_reporte, "Estado\tP_simulada\tDesv_Std\tP_teorica\tError_Abs\tError_Rel(%%)\n");
    fprintf(f_reporte, "------\t----------\t--------\t---------\t---------\t----------\n");
    
    for (int i = 0; i <= N; i++) {
        double desv_std = sqrt(prob_varianza[i]);
        double error_abs = fabs(prob_promedio[i] - (double)prob_teoricas[i]);
        double error_rel = ((double)prob_teoricas[i] > 0) ? 
                          (error_abs / (double)prob_teoricas[i]) * 100 : 0;
        
        fprintf(f_reporte, "%d\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\n", 
                i, prob_promedio[i], desv_std, (double)prob_teoricas[i], 
                error_abs, error_rel);
    }
    
    // Estadísticas de probabilidad de bloqueo
    double pb_promedio = 0.0, pb_varianza = 0.0;
    for (const auto& resultado : resultados) {
        pb_promedio += resultado.prob_bloqueo;
    }
    pb_promedio /= escenario.num_ejecuciones;
    
    for (const auto& resultado : resultados) {
        double diff = resultado.prob_bloqueo - pb_promedio;
        pb_varianza += diff * diff;
    }
    pb_varianza /= (escenario.num_ejecuciones - 1);
    
    fprintf(f_reporte, "\nII. PROBABILIDAD DE BLOQUEO\n");
    fprintf(f_reporte, "P_b simulada:     %.8f ± %.8f\n", pb_promedio, sqrt(pb_varianza));
    fprintf(f_reporte, "P_b teórica:      %.8f\n", pb_teorico);
    fprintf(f_reporte, "Error absoluto:   %.8f\n", fabs(pb_promedio - pb_teorico));
    fprintf(f_reporte, "Error relativo:   %.2f%%\n", 
            (pb_teorico > 0) ? (fabs(pb_promedio - pb_teorico) / pb_teorico) * 100 : 0);
    
    // Métricas adicionales del sistema
    double util_promedio = 0.0, clientes_promedio = 0.0, throughput_promedio = 0.0;
    for (const auto& resultado : resultados) {
        util_promedio += resultado.utilizacion;
        clientes_promedio += resultado.num_promedio_clientes;
        throughput_promedio += resultado.throughput;
    }
    util_promedio /= escenario.num_ejecuciones;
    clientes_promedio /= escenario.num_ejecuciones;
    throughput_promedio /= escenario.num_ejecuciones;
    
    fprintf(f_reporte, "\nIII. MÉTRICAS ADICIONALES DEL SISTEMA\n");
    fprintf(f_reporte, "Utilización:                 %.4f (%.1f%%)\n", 
            util_promedio, util_promedio * 100);
    fprintf(f_reporte, "Número promedio de clientes: %.4f\n", clientes_promedio);
    fprintf(f_reporte, "Throughput:                  %.6f clientes/slot\n", throughput_promedio);
    
    // Intervalos de confianza para P_b (95%)
    double t_value = 2.045; // t-student para 29 grados de libertad, 95% confianza
    double ic_inferior = pb_promedio - t_value * sqrt(pb_varianza) / sqrt(escenario.num_ejecuciones);
    double ic_superior = pb_promedio + t_value * sqrt(pb_varianza) / sqrt(escenario.num_ejecuciones);
    
    fprintf(f_reporte, "\nIV. INTERVALO DE CONFIANZA P_b (95%%)\n");
    fprintf(f_reporte, "[%.8f, %.8f]\n", ic_inferior, ic_superior);
    fprintf(f_reporte, "¿Teórico dentro del IC? %s\n", 
            (pb_teorico >= ic_inferior && pb_teorico <= ic_superior) ? "SÍ" : "NO");
    
    // Análisis de convergencia
    fprintf(f_reporte, "\nV. ANÁLISIS DE CONVERGENCIA (%d slots)\n", escenario.num_slots);
    fprintf(f_reporte, "Coeficiente de variación P_b: %.4f\n", 
            sqrt(pb_varianza) / pb_promedio);
    fprintf(f_reporte, "Error estándar P_b:          %.8f\n", 
            sqrt(pb_varianza) / sqrt(escenario.num_ejecuciones));
    
    fprintf(f_reporte, "\n");
}

// FUNCIONES AUXILIARES (copiadas del código original)

int generar_bernoulli(double prob) {
    return lcgrand(seed_base) < prob ? 1 : 0;
}

int generar_binomial(int n, double prob) {
    int count = 0;
    for (int i = 0; i < n; i++)
        if (generar_bernoulli(prob))
            count++;
    return count;
}

double factorial(int n) {
    if (n <= 1) return 1.0;
    double r = 1.0;
    for (int i = 2; i <= n; i++)
        r *= i;
    return r;
}

double potencia(double base, int exponente) {
    if (exponente == 0) return 1.0;
    if (exponente < 0) return 1.0 / potencia(base, -exponente);
    double result = 1.0;
    for (int i = 0; i < exponente; i++)
        result *= base;
    return result;
}

int min_integer(int a, int b) {
    return (a < b) ? a : b;
}

long double comb(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    
    k = min_integer(k, n - k);
    long double res = 1;
    for (int i = 1; i <= k; ++i) {
        res *= (n - k + i);
        res /= i;
    }
    return res;
}

long double combEq0(int upper, int lower) {
    return ((upper < lower) || (lower < 0)) ? 0 : comb(upper, lower);
}

long double a_func(int n, int b, long double p, long double s, int m, bool same_slot) {
    auto a_N_N = [](int n, long double p, long double s, int m, bool same_slot) {
        long double term1 = p * (comb(m, 1) * (s * potencia(1 - s, m - 1)) + potencia(1 - s, m));
        long double term2 = (1 - p) * potencia(1 - s, m);
        return term1 + term2;
    };

    auto a_n_n_minus_l = [](int n, int l, long double p, long double s, int m, bool same_slot) {
        long double term1, term2;
        
        if (same_slot) {
            term1 = p * combEq0(min_integer(n + 1, m), l + 1) * potencia(s, l + 1) * potencia(1 - s, min_integer(n + 1, m) - 1 - l);
            term2 = (1 - p) * combEq0(min_integer(n, m), l) * potencia(s, l) * potencia(1 - s, min_integer(n, m) - l);
        } else {
            term1 = p * combEq0(min_integer(n, m), l + 1) * potencia(s, l + 1) * potencia(1 - s, min_integer(n, m) - 1 - l);
            term2 = (1 - p) * combEq0(min_integer(n, m), l) * potencia(s, l) * potencia(1 - s, min_integer(n, m) - l);
        }
        return term1 + term2;
    };

    if (n == b) {
        return a_N_N(n, p, s, m, same_slot);
    } else {
        int l = n - b;
        return a_n_n_minus_l(n, l, p, s, m, same_slot);
    }
}

std::vector<long double> geom_geom_m_N_p_calc(bool same_slot) {
    std::vector<long double> P(N + 1, 0.0);
    P[N] = 1.0;

    for (int i = N - 1; i >= 0; --i) {
        long double sum_prob = 0.0;
        for (int n = i + 1; n <= i + m; ++n) {
            for (int j = n - m; j <= i; ++j) {
                if (j >= 0 && j <= N) {
                    sum_prob += a_func(n, j, p, s, m, same_slot) * ((n > N) ? 0 : P[n]);
                }
            }
        }
        P[i] = (1.0 / a_func(i, i + 1, p, s, m, same_slot)) * sum_prob;
    }

    long double sum_P = 0.0;
    for (auto prob : P) {
        sum_P += prob;
    }
    for (auto &prob : P) {
        prob /= sum_P;
    }

    return P;
}

long double calcular_prob_teorica_geo_geo_m_N(int i, bool same_slot) {
    std::vector<long double> result = geom_geom_m_N_p_calc(same_slot);
    if (i < 0 || i >= result.size()) {
        return 0.0;
    }
    return result[i];
}

double calcular_pb_teorico_geo_geo_m_N(void) {
    double pN = calcular_prob_teorica_geo_geo_m_N(N, false);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}