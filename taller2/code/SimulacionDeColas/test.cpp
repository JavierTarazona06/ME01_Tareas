#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Función para generar números exponenciales
float expon(float media) {
    float u = (float)rand() / (RAND_MAX + 1.0);
    return -media * log(1.0 - u);
}

int main() {
    // Configuración inicial
    const int NUM_DATOS = 1000;  // Cantidad de números a generar
    const float MEDIA = 6.0;      // Media de la distribución exponencial
    const char* NOMBRE_ARCHIVO = "test.csv";

    // Inicializar semilla aleatoria
    //std::srand(std::time(0));

    // Abrir archivo CSV
    std::ofstream archivo(NOMBRE_ARCHIVO);
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo " << NOMBRE_ARCHIVO << std::endl;
        return 1;
    }

    // Escribir encabezado
    archivo << "Numero,Valor\n";

    // Generar y guardar los datos
    for (int i = 0; i < NUM_DATOS; ++i) {
        float valor = expon(MEDIA);
        archivo << i + 1 << "," << valor << "\n";
    }

    // Cerrar archivo
    archivo.close();

    std::cout << "Se generaron " << NUM_DATOS << " números y se guardaron en " << NOMBRE_ARCHIVO << std::endl;

    return 0;
}