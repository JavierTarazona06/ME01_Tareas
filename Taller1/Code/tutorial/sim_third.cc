/* SPDX-License-Identifier: GPL-2.0-only */   // Licencia GPL-2.0-only

/*
A grandes rasgos, este tercer ejemplo construye una red mixta que combina tres tecnologías: (1) un enlace punto-a-punto 
de 5 Mb/s entre los nodos n0 y n1; (2) una LAN Ethernet (CSMA) de 100 Mb/s donde participan el nodo n1 y tres nodos 
adicionales (n2 – n4); y (3) una red Wi-Fi con un punto de acceso alojado en n0 y varias estaciones móviles (n5 – n7). 
Se instala la pila IPv4 en todos los nodos y se asignan tres subredes (10.1.1.0/24 para el enlace P2P, 10.1.2.0/24 
para la LAN y 10.1.3.0/24 para la WLAN). Luego se coloca un servidor UDP Echo en el último nodo CSMA y un cliente UDP 
Echo en la última estación Wi-Fi; el cliente envía un único datagrama de 1024 bytes en el segundo 2, el servidor lo 
devuelve y se detiene todo a los 10 segundos. Opcionalmente, si se activa el flag --tracing, se generan archivos pcap 
para inspeccionar el tráfico en cada medio. Con este script el tutorial muestra cómo integrar simultáneamente 
enlaces P2P, LAN cableada y Wi-Fi, asignar movilidad aleatoria a las estaciones, parametrizar la topología 
desde la línea de comandos y habilitar rastreo de paquetes.
*/

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

#include "ns3/applications-module.h"         // Helpers para aplicaciones (Echo)
#include "ns3/core-module.h"                 // Núcleo de ns-3 (Time, Log, CmdLine)
#include "ns3/csma-module.h"                 // Helper para LAN Ethernet/CSMA
#include "ns3/internet-module.h"             // Pila IP (IPv4/IPv6, UDP/TCP)
#include "ns3/mobility-module.h"             // Modelos de movilidad y posiciones
#include "ns3/network-module.h"              // Node, NetDevice, Packet…
#include "ns3/point-to-point-module.h"       // Helper para enlaces punto-a-punto
#include "ns3/ssid.h"                        // Clase Ssid (identificador Wi-Fi)
#include "ns3/yans-wifi-helper.h"            // Helpers PHY/MAC Wi-Fi (modelo YANS)

// Topología dibujada en comentario (P2P + CSMA + Wi-Fi) --------------

using namespace ns3;                         // Evita escribir ns3:: en cada tipo

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample"); // Nombre del componente de log

int
main (int argc, char* argv[])
{
    bool      verbose = true;                // ¿Activar logs INFO para apps?
    uint32_t  nCsma  = 3;                    // Nº de nodos extra en la LAN CSMA
    uint32_t  nWifi  = 3;                    // Nº de estaciones Wi-Fi
    bool      tracing = false;               // ¿Generar archivos pcap?

    NS_LOG_UNCOND ("Third Example");   // Mensaje que siempre se imprime

    CommandLine cmd (__FILE__);              // Lee parámetros desde CLI
    cmd.AddValue ("nCsma",  "Number of \"extra\" CSMA nodes/devices", nCsma);
    cmd.AddValue ("nWifi",  "Number of wifi STA devices",            nWifi);
    cmd.AddValue ("verbose","Tell echo applications to log if true", verbose);
    cmd.AddValue ("tracing","Enable pcap tracing",                   tracing);
    cmd.Parse (argc, argv);                  // Aplica los valores

    /* Restricción práctica: el GridPositionAllocator sólo soporta 18 STA
       antes de salirse del rectángulo (-50,50). */
    if (nWifi > 18)
    {
        std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box\n";
        return 1;                            // Abortamos si supera el límite
    }

    if (verbose)                             // Habilita logs INFO para cliente/servidor
    {
        LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    /* ----------------- 1. CREACIÓN DE NODOS ----------------- */
    NodeContainer p2pNodes;                  // n0 y n1
    p2pNodes.Create (2);

    /* 2. Enlace P2P entre n0<-->n1 (5 Mb/s, 2 ms) */
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute  ("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute ("Delay",    StringValue("2ms"));
    NetDeviceContainer p2pDevices = p2p.Install (p2pNodes);

    /* 3. LAN CSMA: n1 + nodos extra (n2…n4) */
    NodeContainer csmaNodes;
    csmaNodes.Add      (p2pNodes.Get (1));   // Añade n1
    csmaNodes.Create   (nCsma);              // Crea n2…n(1+nCsma)

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
    csma.SetChannelAttribute ("Delay",    TimeValue (NanoSeconds (6560)));
    NetDeviceContainer csmaDevices = csma.Install (csmaNodes);

    /* 4. Red Wi-Fi: estación-es + punto de acceso */
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);             // n5…n(4+nWifi)
    NodeContainer wifiApNode = p2pNodes.Get (0); // n0 actúa como AP

    // Configuración PHY (YANS) y canal por defecto
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper     phy;
    phy.SetChannel (channel.Create ());

    // Configuración MAC común (SSID)
    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");
    WifiHelper wifi;                         // Helper “alto nivel” 802.11

    // Estaciones (STA)
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid",          SsidValue(ssid),
                 "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install (phy, mac, wifiStaNodes);

    // Punto de acceso (AP)
    mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices  = wifi.Install (phy, mac, wifiApNode);

    /* 5. Movilidad: STA en RandomWalk2d; AP fijo */
    MobilityHelper mobility;

    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                               "Bounds", RectangleValue(Rectangle (-50,50,-50,50)));
    mobility.Install (wifiStaNodes);         // STA móviles

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);           // AP inmóvil

    /* 6. Instalación de la pila IP */
    InternetStackHelper stack;
    stack.Install (csmaNodes);
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    /* 7. Asignación de direcciones IPv4 */
    Ipv4AddressHelper address;

    address.SetBase ("10.1.1.0", "255.255.255.0");          // Enlace P2P
    Ipv4InterfaceContainer p2pIfs = address.Assign (p2pDevices);

    address.SetBase ("10.1.2.0", "255.255.255.0");          // LAN CSMA
    Ipv4InterfaceContainer csmaIfs = address.Assign (csmaDevices);

    address.SetBase ("10.1.3.0", "255.255.255.0");          // WLAN
    address.Assign (staDevices);
    address.Assign (apDevices);

    /* 8. Aplicaciones UDP Echo (servidor en último CSMA, cliente en última STA) */
    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
    serverApps.Start (Seconds (1));
    serverApps.Stop  (Seconds (10));

    UdpEchoClientHelper echoClient (csmaIfs.GetAddress (nCsma), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute ("Interval",   TimeValue(Seconds(1)));
    echoClient.SetAttribute ("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps =
        echoClient.Install (wifiStaNodes.Get (nWifi - 1));
    clientApps.Start (Seconds (2));
    clientApps.Stop  (Seconds (10));

    /* 9. Ruteo global: construye tablas estáticas */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    Simulator::Stop (Seconds (10));         // Plan de parada

    /* 10. Trazas opcionales (pcap) */
    if (tracing)
    {
        phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
        p2p.EnablePcapAll ("third");       // n0, n1
        phy.EnablePcap           ("third", apDevices.Get (0)); // En el AP
        csma.EnablePcap          ("third", csmaDevices.Get (0), true); // En n1
    }

    /* 11. Arranca y destruye la simulación */
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
