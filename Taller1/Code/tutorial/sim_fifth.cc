/* SPDX-License-Identifier: GPL-2.0-only
 * -----------------------------------------------------------------
 * Ejemplo 5: medir en tiempo real la evolución de la ventana
 *            de congestión (cwnd) de un flujo TCP sencillo.
 * -----------------------------------------------------------------
 */

/*
El programa fifth.cc muestra cómo instrumentar y observar la evolución de la ventana de 
congestión de TCP en ns-3. Para ello crea un enlace punto-a-punto de 5 Mb/s y 2 ms entre 
dos nodos (10.1.1.1 y 10.1.1.2), configura la pila TCP NewReno con ventana inicial de 1 
segmento y “classic” fast-recovery, introduce un pequeño modelo de errores en el receptor, 
instala en n1 un Packet Sink que escucha el puerto 8080 y en n0 una aplicación personalizada 
(TutorialApp) que envía 1000 paquetes de 1040 bytes a 1 Mb/s. El socket TCP del emisor se 
conecta a un TraceSource llamado CongestionWindow: cada vez que cambia la cwnd el callback 
CwndChange imprime la marca de tiempo y el nuevo valor. Además se conecta una traza de 
descartes físicos (PhyRxDrop) en el receptor, de modo que cuando el modelo de error 
descarta un paquete se registra en consola. Con todo ello el ejemplo ilustra 
➊ cómo ajustar parámetros globales de TCP,
➋ cómo crear sockets y enlazar callbacks de traza antes del arranque de la aplicación y 
➌ cómo combinar un modelo de errores para disparar eventos que afectan al control de congestión.
*/

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//

#include "tutorial-app.h"                 // Aplicación personalizada que actúa como fuente

// ── Helpers de ns-3 ───────────────────────────────────────────────────────────────
#include "ns3/applications-module.h"      // PacketSink, SocketFactory, etc.
#include "ns3/core-module.h"              // Simulator, Config, Log
#include "ns3/internet-module.h"          // Pila TCP/IP
#include "ns3/network-module.h"           // Node, NetDevice, Packet
#include "ns3/point-to-point-module.h"    // Enlace P2P
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FifthScriptExample"); // Nombre del componente de logging

// ╔══════════════════════════════════════════════════════════════════════╗
//  Callback que se ejecuta cada vez que cambia la ventana de congestión
// ╚══════════════════════════════════════════════════════════════════════╝
static void
CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()   // tiempo actual
                   << "\t" << newCwnd);              // nuevo tamaño de cwnd (bytes)
}

// Callback para detectar paquetes descartados en la capa física del receptor
static void
RxDrop (Ptr<const Packet> /*p*/)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

// ╔══════════════════════════════════════════════════════════════════════╗
//  Función principal
// ╚══════════════════════════════════════════════════════════════════════╝
int
main (int argc, char* argv[])
{
    // ───────────────── 1. Procesar CLI (no se añade ningún parámetro) ───────────
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);

    // ───────────────── 2. Ajustes globales de TCP (solo a modo de demo) ─────────
    //    • SocketType → NewReno
    //    • ventana inicial → 1 MSS
    //    • mecanismo de recuperación → algoritmo clásico
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                        StringValue ("ns3::TcpNewReno"));
    Config::SetDefault ("ns3::TcpSocket::InitialCwnd",
                        UintegerValue (1));
    Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                        TypeIdValue (TypeId::LookupByName ("ns3::TcpClassicRecovery")));

    // ───────────────── 3. Topología: 2 nodos + enlace P2P ───────────────────────
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
    p2p.SetChannelAttribute ("Delay",    StringValue ("2ms"));
    NetDeviceContainer devices = p2p.Install (nodes);

    // ───────────────── 4. Añadir un modelo de errores al receptor ───────────────
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));       // 1e-5
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    // ───────────────── 5. Pila IP y direccionamiento ───────────────────────────
    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.252");             // /30
    Ipv4InterfaceContainer ifs = address.Assign (devices);

    // ───────────────── 6. Aplicación sumidero (nodo 1) ─────────────────────────
    uint16_t sinkPort = 8080;
    Address sinkAddr (InetSocketAddress (ifs.GetAddress (1), sinkPort));
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                                 InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = sinkHelper.Install (nodes.Get (1));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop  (Seconds (20.0));

    // ───────────────── 7. Crear socket TCP en nodo 0 y conectar traza cwnd ─────
    Ptr<Socket> tcpSocket = Socket::CreateSocket (nodes.Get (0),
                                                  TcpSocketFactory::GetTypeId ());
    tcpSocket->TraceConnectWithoutContext ("CongestionWindow",
                                           MakeCallback (&CwndChange));

    // ───────────────── 8. Fuente de tráfico: TutorialApp personalizada ─────────
    Ptr<TutorialApp> app = CreateObject<TutorialApp> ();
    app->Setup (tcpSocket,   // socket preconfigurado
                sinkAddr,    // dirección de destino
                1040,        // tamaño de paquete (bytes)
                1000,        // número de paquetes
                DataRate ("1Mbps")); // tasa de envío
    nodes.Get (0)->AddApplication (app);
    app->SetStartTime (Seconds (1.0));
    app->SetStopTime  (Seconds (20.0));

    // ───────────────── 9. Conectar traza de descartes en receptor ──────────────
    devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop",
                                                 MakeCallback (&RxDrop));

    // ───────────────── 10. Ejecutar simulación ─────────────────────────────────
    Simulator::Stop (Seconds (20));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
