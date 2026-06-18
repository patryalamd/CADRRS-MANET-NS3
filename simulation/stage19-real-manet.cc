#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/aodv-module.h"
#include <vector>
#include <algorithm>
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Stage1BaselineAodv");

struct RouteInfo
{
    std::string routeId;

    double linkStability;

    double congestionLevel;

    int hopCount;

    double reliabilityScore;
};

double
CalculateReliability(
    double ls,
    double cong,
    int hop)
{
    return (0.5 * ls) +
           (0.3 * (1.0 - cong)) +
           (0.2 * (1.0 / hop));
}

int
main(int argc, char *argv[])
{
uint32_t nNodes = 50;

NodeContainer nodes;
nodes.Create(nNodes);

WifiHelper wifi;
wifi.SetStandard(WIFI_STANDARD_80211b);

YansWifiPhyHelper wifiPhy;
YansWifiChannelHelper wifiChannel =
    YansWifiChannelHelper::Default();

wifiPhy.SetChannel(wifiChannel.Create());

WifiMacHelper wifiMac;
wifiMac.SetType("ns3::AdhocWifiMac");

NetDeviceContainer devices =
    wifi.Install(wifiPhy, wifiMac, nodes);

MobilityHelper mobility;

mobility.SetPositionAllocator(
    "ns3::GridPositionAllocator",
    "MinX", DoubleValue(0.0),
    "MinY", DoubleValue(0.0),
    "DeltaX", DoubleValue(20.0),
    "DeltaY", DoubleValue(20.0),
    "GridWidth", UintegerValue(5),
    "LayoutType", StringValue("RowFirst"));

mobility.SetMobilityModel(
    "ns3::RandomWalk2dMobilityModel",
    "Bounds",
    RectangleValue(
        Rectangle(
            0.0,
            200.0,
            0.0,
            200.0)),
    "Speed",
    StringValue(
        "ns3::ConstantRandomVariable[Constant=5.0]"));

mobility.Install(nodes);

AodvHelper aodv;

InternetStackHelper internet;
internet.SetRoutingHelper(aodv);
internet.Install(nodes);

Ipv4AddressHelper address;

address.SetBase(
    "10.1.1.0",
    "255.255.255.0");

Ipv4InterfaceContainer interfaces;

interfaces =
    address.Assign(devices);

std::vector<RouteInfo> routes;

routes.push_back(
    {
        "Route-A",
        0.95,
        0.10,
        2,
        0.0
    });

routes.push_back(
    {
        "Route-B",
        0.85,
        0.20,
        3,
        0.0
    });

routes.push_back(
    {
        "Route-C",
        0.80,
        0.30,
        4,
        0.0
    });

for (auto& route : routes)
{
    route.reliabilityScore =
        CalculateReliability(
            route.linkStability,
            route.congestionLevel,
            route.hopCount);
}

std::sort(
    routes.begin(),
    routes.end(),
    [](const RouteInfo& a,
       const RouteInfo& b)
    {
        return a.reliabilityScore >
               b.reliabilityScore;
    });

std::cout
    << "\nCADRRS ACTIVE"
    << std::endl;

std::cout
    << "Primary Route : "
    << routes[0].routeId
    << std::endl;

std::cout
    << "Reliability Score : "
    << routes[0].reliabilityScore
    << std::endl
    << std::endl;

/*
 * Simulated degradation event
 */
routes[0].linkStability = 0.30;

for (auto& route : routes)
{
    route.reliabilityScore =
        CalculateReliability(
            route.linkStability,
            route.congestionLevel,
            route.hopCount);
}

std::sort(
    routes.begin(),
    routes.end(),
    [](const RouteInfo& a,
       const RouteInfo& b)
    {
        return a.reliabilityScore >
               b.reliabilityScore;
    });

int routeSwitchCount = 1;

std::cout
    << "=== DEGRADATION EVENT ==="
    << std::endl;

std::cout
    << "New Primary Route : "
    << routes[0].routeId
    << std::endl;

std::cout
    << "Route Switch Count : "
    << routeSwitchCount
    << std::endl
    << std::endl;

for (uint32_t i = 0; i < (nNodes - 1); i += 2)
{
    uint16_t port =
        5000 + i;

    OnOffHelper onoff(
        "ns3::UdpSocketFactory",
        Address(
            InetSocketAddress(
                interfaces.GetAddress(i + 1),
                port)));

    onoff.SetConstantRate(
        DataRate("100kbps"));

    onoff.SetAttribute(
        "PacketSize",
        UintegerValue(512));

    ApplicationContainer sender =
        onoff.Install(
            nodes.Get(i));

    sender.Start(
        Seconds(1.0));

    sender.Stop(
        Seconds(10.0));

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            port));

    ApplicationContainer receiver =
        sink.Install(
            nodes.Get(i + 1));

    receiver.Start(
        Seconds(0.0));

    receiver.Stop(
        Seconds(10.0));
}

FlowMonitorHelper flowHelper;

Ptr<FlowMonitor> flowMonitor =
    flowHelper.InstallAll();

std::cout
    << "Stage 2.0 Baseline AODV Metrics Ready"
    << std::endl;

std::cout
    << "Node 0 IP : "
    << interfaces.GetAddress(0)
    << std::endl;

std::cout
    << "Node 1 IP : "
    << interfaces.GetAddress(1)
    << std::endl;

std::cout
    << "UDP Traffic Running"
    << std::endl;

Simulator::Stop(Seconds(10.0));

Simulator::Run();

flowMonitor->CheckForLostPackets();

FlowMonitor::FlowStatsContainer stats =
    flowMonitor->GetFlowStats();

uint64_t totalTxPackets = 0;

uint64_t totalRxPackets = 0;

uint64_t totalLostPackets = 0;

double totalDelay = 0.0;

double totalThroughput = 0.0;

uint32_t validFlows = 0;

for (const auto& flow : stats)
{
    const auto& stat =
        flow.second;

    totalTxPackets +=
        stat.txPackets;

    totalRxPackets +=
        stat.rxPackets;

    totalLostPackets +=
        (stat.txPackets -
         stat.rxPackets);

    std::cout
        << "\nFlow ID: "
        << flow.first
        << std::endl;

    std::cout
        << "Tx Packets: "
        << stat.txPackets
        << std::endl;

    std::cout
        << "Rx Packets: "
        << stat.rxPackets
        << std::endl;

    double pdr = 0.0;

    if (stat.txPackets > 0)
    {
        pdr =
            (double)stat.rxPackets *
            100.0 /
            stat.txPackets;
    }

    std::cout
        << "PDR (%): "
        << pdr
        << std::endl;

    double throughput =
        stat.rxBytes *
        8.0 /
        (10.0 * 1000.0);

    totalThroughput +=
        throughput;

    std::cout
        << "Throughput (kbps): "
        << throughput
        << std::endl;

    if (stat.rxPackets > 0)
    {
        double averageDelay =
            stat.delaySum.GetSeconds() /
            stat.rxPackets;

        totalDelay +=
            averageDelay;

        validFlows++;

        std::cout
            << "Average Delay (s): "
            << averageDelay
            << std::endl;
    }
}

double aggregatePdr = 0.0;

if (totalTxPackets > 0)
{
    aggregatePdr =
        (double)totalRxPackets *
        100.0 /
        totalTxPackets;
}

double averageDelayAll = 0.0;

if (validFlows > 0)
{
    averageDelayAll =
        totalDelay /
        validFlows;
}

std::cout
    << "\n========================="
    << std::endl;

std::cout
    << "CADRRS SUMMARY"
    << std::endl;

std::cout
    << "========================="
    << std::endl;

std::cout
    << "Total Tx Packets : "
    << totalTxPackets
    << std::endl;

std::cout
    << "Total Rx Packets : "
    << totalRxPackets
    << std::endl;

std::cout
    << "Total Lost Packets : "
    << totalLostPackets
    << std::endl;

std::cout
    << "PDR (%) : "
    << aggregatePdr
    << std::endl;

std::cout
    << "Average Delay (s) : "
    << averageDelayAll
    << std::endl;

std::cout
    << "Throughput (kbps) : "
    << totalThroughput
    << std::endl;

Simulator::Destroy();

return 0;


}

