/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

 /*
 Este ejemplo (“SixthScriptExample”) crea una topología punto-a-punto de dos nodos y muestra cómo 
 instrumentar una simulación TCP en ns-3 para registrar la evolución de la ventana de congestión 
 (cwnd) y las pérdidas de paquetes. Se construye un canal de 5 Mbps y 2 ms de retardo, se 
 introduce un modelo de errores en la interfaz receptora, se instala un PacketSink (servidor TCP) 
 en el nodo 1 y una aplicación personalizada TutorialApp en el nodo 0 que envía 1000 segmentos de 
 1040 bytes a 1 Mbps. Mediante trace sources se conectan dos callbacks: uno para volcar en un archivo 
 ASCII los cambios de la cwnd y otro para registrar en un archivo pcap los paquetes descartados a nivel 
 físico. Al finalizar (20 s) se destruye el simulador.
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

// ---------------------------- 1. Cabeceras propias ----------------------------
// Declara la aplicación personalizada definida en examples/tutorial/
#include "tutorial-app.h"

// ---------------------------- 2. Módulos de ns-3 ------------------------------
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>                       // Para escribir en archivos externos

using namespace ns3;                     // Evita escribir ns3:: en cada uso

NS_LOG_COMPONENT_DEFINE("SixthScriptExample");

// ============================================================================
//  Escenario: dos nodos conectados por un enlace P2P de 5 Mbps y 2 ms.
//  El nodo-0 (10.1.1.1) corre TutorialApp -> genera flujo TCP → nodo-1
//  (10.1.1.2) corre PacketSink. Se observan: (i) evolución de cwnd,
//  (ii) descartar paquetes en PHY del receptor.  Un pequeño RateErrorModel
//  introduce pérdidas (BER = 1e-5).
// ============================================================================

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

/**
 * Callback que se dispara cada vez que cambia la cwnd del socket.
 *
 * @param stream   Archivo ASCII donde registrar los valores
 * @param oldCwnd  Valor anterior de la ventana de congestión
 * @param newCwnd  Nuevo valor de la ventana de congestión
 */
static void
CwndChange (Ptr<OutputStreamWrapper> stream,
            uint32_t oldCwnd,
            uint32_t newCwnd)
{
    NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    *stream->GetStream () << Simulator::Now ().GetSeconds ()
                          << "\t" << oldCwnd
                          << "\t" << newCwnd << std::endl;
}

/**
 * Callback para paquetes descartados en PHY (traza RxDrop).
 *
 * @param file Archivo pcap donde se registra el paquete
 * @param p    Paquete descartado
 */
static void
RxDrop (Ptr<PcapFileWrapper> file, Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
    file->Write (Simulator::Now (), p);
}

int
main (int argc, char* argv[])
{
    // ------------------------- 3. Procesar CLI -------------------------------
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);

    // ------------------------- 4. Nodos y enlace P2P -------------------------
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices = pointToPoint.Install (nodes);

    // --------------- 4.a  Modelo de errores en la interfaz receptora ---------
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (0.00001));  // BER = 1e-5
    devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    // ------------------------- 5. Pila IP y direcciones ----------------------
    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.252");        // /30 para 2 hosts
    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    // ------------------------- 6. Aplicaciones TCP ---------------------------
    // 6.1  Sink TCP (servidor) en nodo-1
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
                                       InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop  (Seconds (20.0));

    // 6.2  Socket TCP en nodo-0
    Ptr<Socket> ns3TcpSocket =
        Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

    // 6.3  Instanciar TutorialApp (fuente) y configurarla
    Ptr<TutorialApp> app = CreateObject<TutorialApp> ();
    app->Setup (ns3TcpSocket, sinkAddress,
                1040,          // tamaño de paquete
                1000,          // número de paquetes
                DataRate ("1Mbps"));
    nodes.Get (0)->AddApplication (app);
    app->SetStartTime (Seconds (1.0));
    app->SetStopTime  (Seconds (20.0));

    // -------------------- 7. Conectar *TraceSources* -------------------------
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream =
        asciiTraceHelper.CreateFileStream ("sixth.cwnd");
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow",
                                              MakeBoundCallback (&CwndChange, stream));

    PcapHelper pcapHelper;
    Ptr<PcapFileWrapper> file =
        pcapHelper.CreateFile ("sixth.pcap", std::ios::out,
                               PcapHelper::DLT_PPP);
    devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop",
                                                 MakeBoundCallback (&RxDrop, file));

    // ------------------------ 8. Correr la simulación ------------------------
    Simulator::Stop (Seconds (20));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
