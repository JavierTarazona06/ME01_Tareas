/* SPDX-License-Identifier: GPL-2.0-only */

/*
El programa “first.cc” de ns-3 crea dos nodos conectados por un enlace punto-a-punto, instala la pila IPv4, 
coloca un servidor UDP Echo en el nodo 1 y un cliente UDP Echo en el nodo 0, hace que el cliente envíe un único 
paquete de 1024 bytes al segundo 2 y termina la simulación en el segundo 10. El ejemplo sirve para ilustrar la 
mecánica básica de ns-3: creación de nodos, configuración de enlaces, direccionamiento IP, instalación de aplicaciones y 
ejecución del motor de eventos.
*/

#include "ns3/applications-module.h"   // Helpers para instalar apps (UdpEcho, etc.)
#include "ns3/core-module.h"           // Núcleo del simulador: Time, Log, Simulator
#include "ns3/internet-module.h"       // Pila de protocolos: IPv4/IPv6, UDP/TCP
#include "ns3/network-module.h"        // Abstracciones de Nodo, NetDevice, Paquete
#include "ns3/point-to-point-module.h" // Helper para enlaces punto-a-punto

// Topología resultante:
// n0 ---5 Mb/s, 2 ms--- n1   (red 10.1.1.0/24)

using namespace ns3;                   // Evita escribir ns3:: en cada tipo

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample"); // Etiqueta para el sistema de logs

int
main (int argc, char* argv[])
{
    NS_LOG_UNCOND ("First Example!");   // Mensaje que siempre se imprime

    CommandLine cmd (__FILE__);        // Permite pasar argumentos al ejecutar
    cmd.Parse (argc, argv);            //   (no definimos ninguno)

    Time::SetResolution (Time::NS);    // Usa nanosegundos en todo el script
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO); // Activa logs
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO); //   INFO

    NodeContainer nodes;               // Contenedor de nodos
    nodes.Create (2);                  // Crea n0 y n1

    PointToPointHelper p2p;            // Helper para dispositivos + canal P2P
    p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps")); // Velocidad de enlace
    p2p.SetChannelAttribute ("Delay",   StringValue ("2ms"));   // Retardo de propag.

    NetDeviceContainer devices;        // Mantendrá las NetDevices
    devices = p2p.Install (nodes);     // Instala una NetDevice en cada nodo y crea canal

    InternetStackHelper stack;         // Helper para la pila TCP/IP
    stack.Install (nodes);             // Añade IPv4, UDP, routing estático, etc.

    Ipv4AddressHelper address;         // Helper para asignar direcciones
    address.SetBase ("10.1.1.0", "255.255.255.0"); // Red /24

    Ipv4InterfaceContainer interfaces = address.Assign (devices); // 10.1.1.1 y 10.1.1.2

    UdpEchoServerHelper echoServer (9);     // Puerto 9 (echo)

    ApplicationContainer serverApps = echoServer.Install (nodes.Get (1)); // en n1
    serverApps.Start (Seconds (1));   // Servidor activo desde t=1 s
    serverApps.Stop  (Seconds (10));  // hasta t=10 s

    UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9); // Cliente hacia n1:9
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));     // Solo un paquete
    echoClient.SetAttribute ("Interval",   TimeValue (Seconds (1))); // (inútil aquí)
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));  // 1024 bytes

    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0)); // en n0
    clientApps.Start (Seconds (2));   // Envía a t=2 s
    clientApps.Stop  (Seconds (10));  // y termina a t=10 s

    Simulator::Run ();                // Ejecuta la cola de eventos
    Simulator::Destroy ();            // Libera memoria y cierra
    return 0;
}
