/*
 * SPDX-License-Identifier: GPL-2.0-only
 * -----------------------------------------------------------
 * Tutorial 4 – ejemplo mínimo para mostrar cómo “enganchar” una traza (TraceSource).
 */

/*
A grandes rasgos, este ejemplo ilustra cómo funciona el sistema de trazas de ns-3: se define una 
clase personalizada MyObject que hereda de ns3::Object, registra un TraceSource llamado MyInteger 
dentro de su TypeId, conecta en tiempo de ejecución una función callback (IntTrace) a ese origen 
de trazas y, finalmente, asigna un nuevo valor a la variable trazada m_myInt. Al cambiar el valor, 
el callback se activa y escribe en la consola la transición del valor antiguo al nuevo, demostrando 
la mecánica básica de instrumentación y monitoreo en ns-3.
*/

/*
El sistema de trazas de ns-3 es el mecanismo central para extraer información interna de una 
simulación sin modificar el modelo: cada componente (pila IP, dispositivos, colas, 
aplicaciones, etc.) expone uno o más TraceSource, objetos que “disparan” eventos cuando 
algo interesante ocurre (por ejemplo, la llegada de un paquete o el cambio de un valor). 
Los usuarios o módulos externos actúan como sinks (receptores) y se conectan dinámicamente 
a esos TraceSource mediante callbacks; así obtienen los datos crudos o los reenvían a 
probes/archivos pcap sin alterar la lógica del simulador.

Se registra en el TypeId de la clase con AddTraceSource y va unido a una variable 
(p.ej. un TracedValue<int>) o a un disparador explícito (m_rxTrace (pkt, from) en un NetDevice).
*/

#include "ns3/object.h"                 // Clase base Object y utilidades de tipos
#include "ns3/simulator.h"              // Motor de simulación (no se usa directamente aquí)
#include "ns3/trace-source-accessor.h"  // Helpers para registrar fuentes de traza
#include "ns3/traced-value.h"           // Contenedor que dispara callbacks al cambiar
#include "ns3/uinteger.h"               // Tipos enteros con atributos (no utilizado)
#include <iostream>                     // Para imprimir en consola

using namespace ns3;                    // Evita escribir ns3:: en cada referencia

/* --------------------------------------------------------------------------
 * 1. Declaración de MyObject   →  hereda de ns3::Object y expone un TraceSource
 * --------------------------------------------------------------------------*/
class MyObject : public Object
{
  public:
    /* 1.1 Registro de metadatos (TypeId)  ------------------------------- */
    static TypeId GetTypeId()
    {
        /*  • Crea un TypeId único ("MyObject")
         *  • Declara su padre (`Object`)
         *  • Añade el grupo “Tutorial” (para documentación)
         *  • Registra un constructor por defecto
         *  • Añade un TraceSource llamado "MyInteger"
         *      - Descripción: “An integer value to trace.”
         *      - Accessor: puntero al miembro TracedValue m_myInt
         *      - Tipo de callback esperado: Int32 (ver más abajo)
         */
        static TypeId tid = TypeId("MyObject")
                                .SetParent<Object>()
                                .SetGroupName("Tutorial")
                                .AddConstructor<MyObject>()
                                .AddTraceSource("MyInteger",
                                                "An integer value to trace.",
                                                MakeTraceSourceAccessor(&MyObject::m_myInt),
                                                "ns3::TracedValueCallback::Int32");
        return tid;
    }

    /* 1.2 Constructor simple (no hace nada especial) ------------------- */
    MyObject() {}

    /* 1.3 Variable que será observada ---------------------------------- */
    TracedValue<int32_t> m_myInt;  //!< Al modificar este valor se emite la traza.
};

/* --------------------------------------------------------------------------
 * 2. Callback que se dispara cuando cambia m_myInt
 * --------------------------------------------------------------------------*/
void
IntTrace(int32_t oldValue, int32_t newValue)
{
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
}

/* --------------------------------------------------------------------------
 * 3. Función principal
 * --------------------------------------------------------------------------*/
int
main(int argc, char* argv[])
{

    NS_LOG_UNCOND ("Fourth Example");   // Mensaje que siempre se imprime

    // 3.1 Crear una instancia del objeto mediante el sistema de memoria de ns-3
    Ptr<MyObject> myObject = CreateObject<MyObject>();

    // 3.2 Conectar el TraceSource “MyInteger” al callback IntTrace
    //     • TraceConnectWithoutContext: no añade prefijo de ruta al callback
    myObject->TraceConnectWithoutContext("MyInteger", MakeCallback(&IntTrace));

    // 3.3 Cambiar el valor trazado → dispara el callback y escribe en consola
    myObject->m_myInt = 1234;

    return 0;   // Fin
}
