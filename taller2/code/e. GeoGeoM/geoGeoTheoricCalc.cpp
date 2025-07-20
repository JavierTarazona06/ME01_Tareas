#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

int N = 5;
long double p = 0.3;
long double s = 0.4;
int m = 5;

// bool same_slot = true;

// Calcula potencia base^exponente
double potencia(double base, int exponente)
{
    if (exponente == 0)
        return 1.0;
    if (exponente < 0)
        return 1.0 / potencia(base, -exponente);
    double result = 1.0;
    for (int i = 0; i < exponente; i++)
        result *= base;
    return result;
}

// Función para calcular combinaciones (n choose k)
long double comb(int n, int k)
{
    if (k < 0 || k > n)
        return 0;
    if (k == 0 || k == n)
        return 1;

    k = min(k, n - k); // Optimización
    long double res = 1;
    for (int i = 1; i <= k; ++i)
    {
        res *= (n - k + i);
        res /= i;
    }
    return res;
}

// Función auxiliar combEq0
// Funcion combinatoria que devuelve 0 si el valor superior es menor al valor inferior o si el valor inferior es menor a 0
long double combEq0(int upper, int lower)
{
    return ((upper < lower) || (lower < 0)) ? 0 : comb(upper, lower);
}

// Función a_N_N
/*long double a_N_N(int n, long double p, long double s, int m, bool same_slot) {
    long double term1 = p * (comb(m, 1) * (s * potencia(1-s, m-1)) + potencia(1-s, m));
    long double term2 = (1-p) * potencia(1-s, m);
    return term1 + term2;
}*/

// Función a_n_n_minus_l
/*long double a_n_n_minus_l(int n, int l, long double p, long double s, int m, bool same_slot) {
    long double term1, term2;

    if (same_slot) {
        term1 = p * combEq0(min(n+1, m), l+1) * potencia(s, l+1) * potencia(1-s, min(n+1, m)-1-l);
        term2 = (1-p) * combEq0(min(n, m), l) * potencia(s, l) * potencia(1-s, min(n, m)-l);
    } else {
        term1 = p * combEq0(min(n, m), l+1) * potencia(s, l+1) * potencia(1-s, min(n, m)-1-l);
        term2 = (1-p) * combEq0(min(n, m), l) * potencia(s, l) * potencia(1-s, min(n, m)-l);
    }
    return term1 + term2;
}*/

// funcion a de geom_geom_m_N_p_calc
long double a_func(int n, int b, long double p, long double s, int m, bool same_slot)
{
    auto a_N_N = [](int n, long double p, long double s, int m, bool same_slot)
    {
        long double term1 = p * (comb(m, 1) * (s * potencia(1 - s, m - 1)) + potencia(1 - s, m));
        long double term2 = (1 - p) * potencia(1 - s, m);
        return term1 + term2;
    };

    auto a_n_n_minus_l = [](int n, int l, long double p, long double s, int m, bool same_slot)
    {
        long double term1, term2;

        if (same_slot)
        {
            term1 = p * combEq0(min(n + 1, m), l + 1) * potencia(s, l + 1) * potencia(1 - s, min(n + 1, m) - 1 - l);
            term2 = (1 - p) * combEq0(min(n, m), l) * potencia(s, l) * potencia(1 - s, min(n, m) - l);
        }
        else
        {
            term1 = p * combEq0(min(n, m), l + 1) * potencia(s, l + 1) * potencia(1 - s, min(n, m) - 1 - l);
            term2 = (1 - p) * combEq0(min(n, m), l) * potencia(s, l) * potencia(1 - s, min(n, m) - l);
        }
        return term1 + term2;
    };

    if (n == b)
    {
        return a_N_N(n, p, s, m, same_slot);
    }
    else
    {
        int l = n - b;
        return a_n_n_minus_l(n, l, p, s, m, same_slot);
    }
}

// REaliza el calculo de P_n con la funcion presentada en la seccion 6.4 The Geom/Geom/m/N
// Queing system del libro Computer Networks and Systems: Queueing Theory and Performance
// Evaluation Third Edition escrito por Thomas G. Robertazzi
vector<long double> geom_geom_m_N_p_calc(bool same_slot = true)
{
    // Inicializar probabilidades (no normalizadas)
    vector<long double> P(N + 1, 0.0);
    P[N] = 1.0; // Paso 1: Sea PN = 1.0

    // Pasos 3-6: Calcular Pi recursivamente
    for (int i = N - 1; i >= 0; --i)
    {
        long double sum_prob = 0.0;
        for (int n = i + 1; n <= i + m; ++n)
        {
            for (int j = n - m; j <= i; ++j)
            {
                if (j >= 0 && j <= N)
                {
                    sum_prob += a_func(n, j, p, s, m, same_slot) * ((n > N) ? 0 : P[n]);
                }
            }
        }
        P[i] = (1.0 / a_func(i, i + 1, p, s, m, same_slot)) * sum_prob;
    }

    // Paso 7-8: Normalizar las probabilidades
    long double sum_P = 0.0;
    for (auto prob : P)
    {
        sum_P += prob;
    }
    for (auto &prob : P)
    {
        prob /= sum_P;
    }

    return P;
}

// Función para imprimir el vector (opcional)
void printVector(const vector<long double> &vec)
{
    cout << "[";
    for (size_t i = 0; i < vec.size(); ++i)
    {
        cout << vec[i];
        if (i != vec.size() - 1)
            cout << ", ";
    }
    cout << "]" << endl;
}

long double calcular_prob_teorica_geo_geo_m_N(int i, bool same_slot = true)
{
    // Obtener todas las probabilidades
    vector<long double> result = geom_geom_m_N_p_calc();

    // Verificar que el índice esté dentro del rango válido
    if (i < 0 || i >= result.size())
    {
        throw out_of_range("Índice fuera de rango. Debe estar entre 0 y " + to_string(N));
    }

    // Devolver solo el valor en la posición i
    return result[i];
}

// Calcula la probabilidad teórica de bloqueo
double calcular_pb_teorico_geo_geo_m_N(void)
{
    double pN = calcular_prob_teorica_geo_geo_m_N(N);
    double s0 = potencia(1.0 - s, m);
    return pN * s0;
}

// Ejemplo de uso
int main()
{

    vector<long double> result = geom_geom_m_N_p_calc();
    printVector(result);
    int indice = 4;
    long double probabilidad = calcular_prob_teorica_geo_geo_m_N(indice);
    double pb = calcular_pb_teorico_geo_geo_m_N();
    cout << "Probabilidad en el índice " << indice << ": " << probabilidad << endl;
    cout << "Pb " << pb << endl;
    return 0;
}