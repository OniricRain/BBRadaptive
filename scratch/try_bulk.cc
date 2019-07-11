//           
// Network topology
//
//       n0 -------------------- n1
//    10.1.1.1                10.1.1.2 
//
// - Flow from n0 to n1 using BulkSendApplication.
//
// - Tracing of queues and packet receptions to file "*.tr" and
//   "*.pcap" when tracing is turned on.
//

// System includes.
#include <string>
#include <fstream>

// NS3 includes.
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

// Constants.
//#define ENABLE_PCAP      false     // Set to "true" to enable pcap
//#define ENABLE_TRACE     false     // Set to "true" to enable trace
//#define BIG_QUEUE        2000      // Packets
//#define QUEUE_SIZE       100       // Packets
#define START_TIME       0.0       // Seconds
#define STOP_TIME        50.0       // Seconds
//#define S_TO_C_BW        "10Mbps"   // Server to client
//#define S_TO_C_DELAY     "10ms" 

#define PACKET_SIZE      1000      // Bytes.

// Uncomment one of the below.
//#define TCP_PROTOCOL     "ns3::TcpBbr"
//#define TCP_PROTOCOL     "ns3::TcpNewReno"
bool firstRtt = true;
std::vector<std::string> datarates = {"100Kbps","200Kbps","500Kbps","1Mbps","5Mbps","10Mbps","20Mbps"};
float CasualProb = 0.25;

static void ChangeDataRate ()
{
  int r = rand();
  uint32_t j = r % datarates.size();

  if (double(r) / (double(RAND_MAX) + 1.0) > CasualProb)
  {
    Config::Set ("/NodeList/0/DeviceList/0/DataRate", StringValue (datarates[j]));
    Config::Set ("/NodeList/1/DeviceList/0/DataRate", StringValue (datarates[j]));
  }
}

Ptr<OutputStreamWrapper> inFlightStream;
static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
}

Ptr<OutputStreamWrapper> rttStream;
static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

// For logging. 

NS_LOG_COMPONENT_DEFINE ("main");
/////////////////////////////////////////////////

int main (int argc, char *argv[]) {

  std::string bandwidth = "10Mbps";
  std::string delay = "10ms";
  std::string transport_prot = "ns3::TcpBbr";
  srand (1);

  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,sizeof(buffer),"%d-%m-%Y-%I-%M-%S",timeinfo);
  std::string currentTime (buffer);

  /////////////////////////////////////////
  // Turn on logging for this script.
  // Note: for BBR', other components that may be
  // of interest include "TcpBbr" and "BbrState".
  LogComponentEnable("main", LOG_LEVEL_INFO);

  /////////////////////////////////////////
  // Setup environment
  Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                     StringValue(transport_prot));

  // Report parameters.
  NS_LOG_INFO("TCP protocol: " << transport_prot);
  NS_LOG_INFO("Server to Client Bwdth: " << bandwidth);
  NS_LOG_INFO("Server to Client Delay: " << delay);
  NS_LOG_INFO("Packet size (bytes): " << PACKET_SIZE);
  
  // Set segment size (otherwise, ns-3 default is 536).
  Config::SetDefault("ns3::TcpSocket::SegmentSize",
                     UintegerValue(PACKET_SIZE)); 

  // Turn off delayed ack (so, acks every packet).
  // Note, BBR' still works without this.
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
   
  /////////////////////////////////////////
  // Create nodes.
  NS_LOG_INFO("Creating nodes.");
  NodeContainer nodes;  // 0=source, 1=sink
  nodes.Create(2);

  /////////////////////////////////////////
  // Create links.
  NS_LOG_INFO("Creating links.");

  // Server to Client.
  int mtu = 1500;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue (bandwidth));
  p2p.SetChannelAttribute("Delay", StringValue (delay));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));

  NetDeviceContainer devices = p2p.Install(nodes);

  /////////////////////////////////////////
  // Install Internet stack.
  NS_LOG_INFO("Installing Internet stack.");
  InternetStackHelper internet;
  internet.Install(nodes);
  
  /////////////////////////////////////////
  // Add IP addresses.
  NS_LOG_INFO("Assigning IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

  /////////////////////////////////////////
  // Create apps.
  NS_LOG_INFO("Creating applications.");
  NS_LOG_INFO("  Bulk send.");

  // Well-known port for server.
  uint16_t port = 911;  

  // Source (at node 0).
  BulkSendHelper source("ns3::TcpSocketFactory",
                        InetSocketAddress(interfaces.GetAddress(1), port));
  // Set the amount of data to send in bytes (0 for unlimited).
  source.SetAttribute("MaxBytes", UintegerValue(0));
  source.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
  ApplicationContainer apps = source.Install(nodes.Get(0));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));

  // Sink (at node 1).
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), port));
  apps = sink.Install(nodes.Get(1));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));
  Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (apps.Get(0)); // 4 stats

  /* 
  /////////////////////////////////////////
  // Setup tracing (as appropriate).
  if (ENABLE_TRACE) {
    NS_LOG_INFO("Enabling trace files.");
    AsciiTraceHelper ath;
    p2p.EnableAsciiAll(ath.CreateFileStream("trace.tr"));
  }  
  if (ENABLE_PCAP) {
    NS_LOG_INFO("Enabling pcap files.");
    p2p.EnablePcapAll("shark", true);
  }
  */

  std::string dir = "results/" + currentTime + "/";
  std::string dirToSave = "mkdir -p " + dir;
  system (dirToSave.c_str ());
  Simulator::Schedule (Seconds (START_TIME + 0.000001), &TraceInFlight, dir + "inflight.data");
  Simulator::Schedule (Seconds (START_TIME + 0.000001), &TraceRtt, dir + "rtt.data");

  p2p.EnablePcapAll (dir + "p", nodes.Get (0));
  /////////////////////////////////////////
    for (size_t i = 1; i < 45; i++)
  {
      Simulator::Schedule (Seconds(i) , &ChangeDataRate);
  }
  // Run simulation.
  NS_LOG_INFO("Running simulation.");
  Simulator::Stop(Seconds(STOP_TIME));
  NS_LOG_INFO("Simulation time: [" << 
              START_TIME << "," <<
              STOP_TIME << "]");
  NS_LOG_INFO("---------------- Start -----------------------");
  Simulator::Run();
  NS_LOG_INFO("---------------- Stop ------------------------");

  /////////////////////////////////////////
  // Ouput stats.
  NS_LOG_INFO("Total bytes received: " << p_sink->GetTotalRx());
  double tput = p_sink->GetTotalRx() / (STOP_TIME - START_TIME);
  tput *= 8;          // Convert to bits.
  tput /= 1000000.0;  // Convert to Mb/s
  NS_LOG_INFO("Throughput: " << tput << " Mb/s");
  NS_LOG_INFO("Done.");

  // Done.
  Simulator::Destroy();
  return 0;
}
