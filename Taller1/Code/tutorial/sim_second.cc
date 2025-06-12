/* SPDX-License-Identifier: GPL-2.0-only */

/*
El programa crea dos segmentos de red distintos: un enlace punto-a-punto de 5 Mb/s y 2 ms de retardo entre el 
nodo 0 y el nodo 1, y una LAN Ethernet (CSMA) de 100 Mb/s que incluye al nodo 1 y a tres nodos adicionales;
 instala la pila IPv4 en todos ellos, asigna las redes 10.1.1.0/24 al enlace y 10.1.2.0/24 a la LAN, 
 genera rutas estáticas para que los paquetes puedan atravesar ambos segmentos, y coloca un servidor UDP 
 Echo (puerto 9) en el último nodo de la LAN y un cliente en el nodo 0; el cliente envía un único datagrama 
 de 1024 bytes en el segundo 2, el servidor lo recibe y lo devuelve, y la simulación se detiene en el 
 segundo 10, mientras se guardan archivos pcap que permiten inspeccionar el tráfico resultante en Wireshark.
*/

#include "ns3/applications-module.h"     // Helpers para instalar aplicaciones (UDP Echo)
#include "ns3/core-module.h"             // Núcleo del simulador: Time, Log, CommandLine…
#include "ns3/csma-module.h"             // Helper para enlaces Ethernet (CSMA)
#include "ns3/internet-module.h"         // Pila IP (IPv4/IPv6, UDP, TCP, routing)
#include "ns3/ipv4-global-routing-helper.h" // Routing estático global
#include "ns3/network-module.h"          // Definición de Node, NetDevice, Packet…
#include "ns3/point-to-point-module.h"   // Helper para enlaces punto-a-punto

// Topología objetivo:
//
//      10.1.1.0/24                 10.1.2.0/24 (LAN)
//    n0 -------- n1   n2   n3   n4
//         P2P      |    |    |    |
//                  ================= CSMA 100 Mb/s
//
// n0 ↔ n1: point-to-point 5 Mb/s, 2 ms de retardo
// n1, n2, n3, n4 comparten una LAN Ethernet (CSMA)

using namespace ns3;                     // Evita ns3:: en cada referencia

NS_LOG_COMPONENT_DEFINE ("SecondScriptExample"); // Etiqueta de logging

int
main (int argc, char* argv[])
{
    bool      verbose = true;            // ¿Activar logs INFO de las apps?
    uint32_t  nCsma  = 3;                // Nº de nodos extra en la LAN (n2, n3, n4)

    NS_LOG_UNCOND ("Second Example");   // Mensaje que siempre se imprime

    CommandLine cmd (__FILE__);          // Permite cambiar parámetros al ejecutar
    cmd.AddValue ("nCsma",  "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue ("verbose","Tell echo applications to log if true",  verbose);
    cmd.Parse (argc, argv);              // Procesa los argumentos

    if (verbose)                         // Activa LOG_LEVEL_INFO en Cliente y Servidor
    {
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    nCsma = nCsma == 0 ? 1 : nCsma;      // Garantiza al menos un nodo CSMA

    /* **** 1. Creación de nodos **** */
    NodeContainer p2pNodes;              // Contendrá n0 y n1
    p2pNodes.Create (2);

    NodeContainer csmaNodes;             // n1 + los extras de la LAN
    csmaNodes.Add   (p2pNodes.Get (1));  // Añade n1
    csmaNodes.Create(nCsma);             // Crea n2…n_(nCsma+1)

    /* **** 2. Configuración del enlace punto-a-punto (n0<-->n1) **** */
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay",    StringValue ("2ms"));

    NetDeviceContainer p2pDevices = p2p.Install (p2pNodes);

    /* **** 3. Configuración de la LAN CSMA (Ethernet) **** */
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay",    TimeValue (NanoSeconds (6560))); // ~propagación bus

    NetDeviceContainer csmaDevices = csma.Install (csmaNodes);

    /* **** 4. Instalación de la pila TCP/IP **** */
    InternetStackHelper stack;
    stack.Install (p2pNodes.Get (0));    // n0
    stack.Install (csmaNodes);           // n1, n2, n3, …

    /* **** 5. Direccionamiento IPv4 **** */
    Ipv4AddressHelper address;

    // Red 10.1.1.0/24 para el enlace P2P
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pIfs = address.Assign (p2pDevices); // n0=10.1.1.1, n1=10.1.1.2

    // Red 10.1.2.0/24 para la LAN CSMA
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaIfs = address.Assign (csmaDevices); // n1=10.1.2.1, n2=… etc.

    /* **** 6. Aplicaciones UDP Echo **** */
    // – Servidor en el ÚLTIMO nodo CSMA (n_(nCsma+1))
    UdpEchoServerHelper echoServer (9);  // puerto 9 = “echo”
    ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
    serverApps.Start (Seconds (1));      // Arranca en t = 1 s
    serverApps.Stop  (Seconds (10));     // Termina en t = 10 s

    // – Cliente situado en n0 que envía al servidor
    UdpEchoClientHelper echoClient (csmaIfs.GetAddress (nCsma), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));     // Solo 1 datagrama
    echoClient.SetAttribute ("Interval",   TimeValue (Seconds (1)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));  // 1 KiB

    ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0)); // n0
    clientApps.Start (Seconds (2));      // Envía en t = 2 s
    clientApps.Stop  (Seconds (10));

    /* **** 7. Rutas: PopulateRoutingTables **** */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); // Genera rutas estáticas válidas

    /* **** 8. Trazas pcap **** */
    p2p.EnablePcapAll ("second");            // Captura tráfico en n0/n1
    csma.EnablePcap   ("second", csmaDevices.Get (1), true); // Captura en n1 o n2

    /* **** 9. Ejecución de la simulación **** */
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
