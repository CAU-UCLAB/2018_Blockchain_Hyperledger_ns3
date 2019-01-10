#include <fstream>
#include <time.h>
#include <sys/time.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/mpi-interface.h"
#define MPI_TEST

#ifdef NS3_MPI
#include <mpi.h>
#endif

using namespace ns3;

double get_wall_time();
int GetNodeIdByIpv4 (Ipv4InterfaceContainer container, Ipv4Address addr);
void PrintStatsForEachNode(nodeStatistics *stats, int totalNodes);
void PrintTotalStats(nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalSeconds);
void PrintBlockchainRegionStats(uint32_t *blockchainNodeRegions, uint32_t totalNodes);

NS_LOG_COMPONENT_DEFINE("Blockchain_test3");

int main(int argc, char *argv[])
{
    #ifdef NS3_MPI
    
    bool nullmsg = false;
    bool testScalability = false;
    long blockSize = -1;
    int invTimeoutMins = -1;
    enum Cryptocurrency cryptocurrency = HYPERLEDGER;
    double tStart = get_wall_time();
    double tStartSimulation;
    double tFinish;
    const int secsPerMin = 60;
    const uint16_t blockchainPort = 8333;
    const double realAverageBlockGenIntervalSeconds = 15;
    int targetNumberOfBlocks = 100;
    double averageBlockGenIntervalSeconds = 15;
    double fixedHashRate = 0.5;
    int start = 0;
  
    int totalNoNodes = 16;
    int minConnectionsPerNode = -1;
    int maxConnectionsPerNode = -1;
    double *minersHash;
    enum BlockchainRegion *minersRegions;
    int noMiners =1;
    int noEndorsers = 6;
    int noClient = 10;
    int creatingTime = 20;

    #ifdef MPI_TEST
    
        double blockchainMinerHash[] = {0.289, 0.196, 0.159, 0.133, 0.066, 0.054, 0.029,
                                        0.016, 0.012, 0.012, 0.012, 0.009, 0.005, 0.005,
                                        0.002, 0.002};

        enum BlockchainRegion blockchainMinersRegions[] = {KOREA, KOREA, KOREA, NORTH_AMERICA,
                                                            KOREA, NORTH_AMERICA, EUROPE, EUROPE,
                                                            NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, EUROPE,
                                                            NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA, NORTH_AMERICA};

    #else

        double blockchainMinerHash[] = {0.5, 0.5};
        enum BlockchainRegion blockchainMinersRegions[] = {KOREA, KOREA};

    #endif

    double averageBlockGenIntervalMinuates = averageBlockGenIntervalSeconds/60; 
    double stop;

    Ipv4InterfaceContainer  ipv4Interfacecontainer;
    std::map<uint32_t, std::vector<Ipv4Address>> nodesConnections;
    std::map<uint32_t, std::map<Ipv4Address, double>> peersDownloadSpeeds;
    std::map<uint32_t, std::map<Ipv4Address, double>> peersUploadSpeeds;
    std::map<uint32_t, nodeInternetSpeed> nodesInternetSpeeds;
    std::vector<uint32_t> miners;
    int nodesInSystemId0 = 0;

    Time::SetResolution(Time::NS);

    CommandLine cmd;
    cmd.AddValue("nullmsg", "Enable the use of null-message synchronization", nullmsg);
    cmd.AddValue("blockSize", "The fixed block size(Bytes)", blockSize);
    cmd.AddValue("noBlocks", "The number of generated blocks", targetNumberOfBlocks);
    cmd.AddValue("nodes", "the total number of nodes in the networks", totalNoNodes);
    cmd.AddValue("miners", "The total number of miners in the network", noMiners);
    cmd.AddValue("minConnections", "The minConnectionsPerNode of the gird", minConnectionsPerNode);
    cmd.AddValue("maxConnections", "The maxConnectionsPerNode of the grid", maxConnectionsPerNode);
    cmd.AddValue("blockIntervalSeconds", "The average block generation interval in seconds", averageBlockGenIntervalSeconds);
    cmd.AddValue("invTimeoutMins", "The inv block timeout(default = 1)", invTimeoutMins);
    cmd.AddValue("test", "Test the scalability of the simulation", testScalability);
    cmd.AddValue("endorsers", "The total number of endorsers in the networks", noEndorsers);
    cmd.AddValue("clients", "The total number of clients in the networks", noClient);
    cmd.AddValue("creatingTime", "The time for generating transaction", creatingTime);

    cmd.Parse(argc, argv);

    /*
    if(noMiners %16 != 0)
    {
        std::cout << "The number of miners must be multiple of 16" << std::endl;
        return 0;
    }
    */

    minersHash = new double[noMiners];
    minersRegions = new enum BlockchainRegion[noMiners];

    minersHash[0] = blockchainMinerHash[0]*16/noMiners;
    minersRegions[0] = blockchainMinersRegions[0];

    /*
    for(int i = 0; i < noMiners/16; i++)
    {
        for(int j = 0; j < 16 ; j++)
        {
            minersHash[i*16+j] = blockchainMinerHash[j]*16/noMiners;
            minersRegions[i*16+j] = blockchainMinersRegions[j];
        }
    }
    */


    averageBlockGenIntervalSeconds = averageBlockGenIntervalMinuates*60;
    stop = targetNumberOfBlocks * averageBlockGenIntervalMinuates;
    nodeStatistics *stats = new nodeStatistics[totalNoNodes];
    averageBlockGenIntervalMinuates = averageBlockGenIntervalSeconds/60;

    


    #ifdef MPI_TEST

        if(nullmsg)
        {
            GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::NullMessagesSimulatorImpl"));
        }
        else
        {
            GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::DistributedSimulatorImpl"));

        }

        MpiInterface::Enable(&argc, &argv);
        uint32_t systemId = MpiInterface::GetSystemId();
        uint32_t systemCount = MpiInterface::GetSize();

    #else
        uint32_t systemId = 0;
        uint32_t systemCount = -1;
    #endif

    BlockchainTopologyHelper blockchainTopologyHelper (systemCount, totalNoNodes, noMiners, minersRegions,
                                                        cryptocurrency, minConnectionsPerNode, maxConnectionsPerNode, 5, systemId);

    
    InternetStackHelper stack;
    blockchainTopologyHelper.InstallStack(stack);

    NS_LOG_INFO("Set Ip address");
    blockchainTopologyHelper.AssignIpv4Addresses(Ipv4AddressHelperCustom("1.0.0.0", "255.255.255.0", false));
    ipv4Interfacecontainer = blockchainTopologyHelper.GetIpv4InterfaceContainer();
    nodesConnections = blockchainTopologyHelper.GetNodesConnectionsIps();
    miners = blockchainTopologyHelper.GetMiners();
    peersUploadSpeeds = blockchainTopologyHelper.GetPeersUploadSpeeds();
    peersDownloadSpeeds = blockchainTopologyHelper.GetPeersDownloadSpeeds();
    nodesInternetSpeeds = blockchainTopologyHelper.GetNodesInternetSpeeds();

    if(systemId == 0)
    {
        PrintBlockchainRegionStats(blockchainTopologyHelper.GetBlockchainNodesRegions(), totalNoNodes);
    }

    NS_LOG_INFO("Create Blockchain miner");
    BlockchainMinerHelper blockchainMinerHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), blockchainPort),
                                                nodesConnections[miners[0]], noMiners, peersDownloadSpeeds[0], peersUploadSpeeds[0],
                                                nodesInternetSpeeds[0], stats, minersHash[0], averageBlockGenIntervalSeconds);

    ApplicationContainer blockchainMiners;
    int count = 0;
    if(testScalability == true)
    {
        blockchainMinerHelper.SetAttribute("FixedBlockIntervalGeneration", DoubleValue(averageBlockGenIntervalSeconds));
    }

    for(auto &miner : miners)
    {
        Ptr<Node> targetNode = blockchainTopologyHelper.GetNode(miner);
        
        if(systemId == targetNode->GetSystemId())
        {
            blockchainMinerHelper.SetAttribute("HashRate", DoubleValue(minersHash[count]));

            if(invTimeoutMins != -1) 
            {
                blockchainMinerHelper.SetAttribute("InvTimeoutMinutes", TimeValue(Minutes(invTimeoutMins)));
            }
            else
            {
                blockchainMinerHelper.SetAttribute("InvTimeoutMinutes", TimeValue(Minutes(2*averageBlockGenIntervalMinuates)));
            }

            if(blockSize != -1)
            {
                blockchainMinerHelper.SetAttribute("FixedBlockSize", UintegerValue(blockSize));
            }

            blockchainMinerHelper.SetPeersAddresses(nodesConnections[miner]);
            blockchainMinerHelper.SetPeersDownloadSpeeds(peersDownloadSpeeds[miner]);
            blockchainMinerHelper.SetPeersUploadSpeeds(peersUploadSpeeds[miner]);
            blockchainMinerHelper.SetNodeInternetSpeeds(nodesInternetSpeeds[miner]);
            blockchainMinerHelper.SetNodeStats(&stats[miner]);

            blockchainMiners.Add(blockchainMinerHelper.Install(targetNode));

            if(systemId == 0)
            {
                nodesInSystemId0++;
            }
        }
        
        count++;
        if(testScalability == true)
        {
            blockchainMinerHelper.SetAttribute("FixedBlockIntervalGeneration", DoubleValue(3*averageBlockGenIntervalSeconds));
        }
    }
    
    blockchainMiners.Start(Seconds(start));
    blockchainMiners.Stop(Minutes(stop));

    NS_LOG_INFO("Create Blockchain node");
    BlockchainNodeHelper blockchainNodeHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), blockchainPort),
                                                nodesConnections[0], peersDownloadSpeeds[0], peersUploadSpeeds[0], nodesInternetSpeeds[0], stats);

    ApplicationContainer blockchainNodes;

    int adoptedEndorser = noEndorsers;
    int adoptedClient = noClient;
    int adoptedCommitter = totalNoNodes - noEndorsers - noClient;
    int *adoptedNodes = new int[totalNoNodes];
    int counter = 0;

    while(totalNoNodes != counter)
    {
        int x = rand()%3;

        if(adoptedEndorser != 0 && x == 0)
        {
            adoptedNodes[counter] = x;
            adoptedEndorser--;
            counter++;
        }
        else if(adoptedClient != 0 && x== 1)
        {
            adoptedNodes[counter] = x;
            adoptedClient--;
            counter++;
        }
        else if(adoptedCommitter !=0 && x==2)
        {
            adoptedNodes[counter] = 2;
            adoptedCommitter--;
            counter++;
        }
    }

    for(int i = 0 ; i < totalNoNodes ; i++)
    {
        std::cout<<adoptedNodes[i]<<" ";
        if(i != 0 && (i+1)%10 ==0)
        {
            std::cout<<"\n";
        }
    }

    int j = 0;

    for(auto &node : nodesConnections)
    {
        Ptr<Node> targetNode = blockchainTopologyHelper.GetNode(node.first);

        if(systemId == targetNode->GetSystemId())
        {
            if(std::find(miners.begin(), miners.end(), node.first) == miners.end())
            {
                if(invTimeoutMins != -1)
                {
                    blockchainNodeHelper.SetAttribute("InvTimeoutMinutes", TimeValue(Minutes(invTimeoutMins)));
                }
                else
                {
                    blockchainNodeHelper.SetAttribute("InvTimeoutMinutes", TimeValue(Minutes(4*averageBlockGenIntervalMinuates)));
                }

                blockchainNodeHelper.SetPeersAddresses(node.second);
                blockchainNodeHelper.SetPeersDownloadSpeeds(peersDownloadSpeeds[node.first]);
                blockchainNodeHelper.SetPeersUploadSpeeds(peersUploadSpeeds[node.first]);
                blockchainNodeHelper.SetNodeInternetSpeeds(nodesInternetSpeeds[node.first]);
                blockchainNodeHelper.SetNodeStats(&stats[node.first]);

                if(adoptedNodes[j] == 0)
                {
                    blockchainNodeHelper.SetCommitterType(ENDORSER);
                }
                else if(adoptedNodes[j] == 1)
                {
                    blockchainNodeHelper.SetCommitterType(CLIENT);
                    blockchainNodeHelper.SetCreatingTransactionTime(creatingTime);
                }
                else if(adoptedNodes[j] == 2)
                {
                    blockchainNodeHelper.SetCommitterType(COMMITTER);
                }
                j++;

                blockchainNodes.Add(blockchainNodeHelper.Install(targetNode));

                if(systemId == 0)
                {
                    nodesInSystemId0++;
                }
            }
        }
    }

    blockchainNodes.Start(Seconds(start));
    blockchainNodes.Stop(Minutes(stop));

    if(systemId == 0)
    {
        std::cout << "The applications have been setup.\n";
    }

    tStartSimulation = get_wall_time();
    if(systemId == 0)
    {
        std::cout << "Setup time = " << tStartSimulation - tStart << " s\n";
    }
    Simulator::Stop(Minutes(stop + 0.1));
    Simulator::Run();
    Simulator::Destroy();

    #ifdef MPI_TEST

        int blocklen[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1};
        MPI_Aint    disp[32];
        MPI_Datatype    dtypes[32] = {MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE,
                                        MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG, MPI_LONG,
                                        MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_LONG, MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE,
                                        MPI_INT, MPI_DOUBLE};
        MPI_Datatype    mpi_nodeStatisticsType;

        disp[0]= offsetof(nodeStatistics, nodeId);
        disp[1]= offsetof(nodeStatistics, meanBlockReceiveTime);
        disp[2]= offsetof(nodeStatistics, meanBlockPropagationTime);
        disp[3]= offsetof(nodeStatistics, meanBlockSize);
        disp[4]= offsetof(nodeStatistics, totalBlocks);
        disp[5]= offsetof(nodeStatistics, miner);
        disp[6]= offsetof(nodeStatistics, minerGeneratedBlocks);
        disp[7]= offsetof(nodeStatistics, minerAverageBlockGenInterval);
        disp[8]= offsetof(nodeStatistics, minerAverageBlockSize);
        disp[9]= offsetof(nodeStatistics, hashRate);
        disp[10]= offsetof(nodeStatistics, invReceivedBytes);
        disp[11]= offsetof(nodeStatistics, invSentBytes);
        disp[12]= offsetof(nodeStatistics, getHeadersReceivedBytes);
        disp[13]= offsetof(nodeStatistics, getHeadersSentBytes);
        disp[14]= offsetof(nodeStatistics, headersReceivedBytes);
        disp[15]= offsetof(nodeStatistics, headersSentBytes);
        disp[16]= offsetof(nodeStatistics, getDataReceivedBytes);
        disp[17]= offsetof(nodeStatistics, getDataSentBytes);
        disp[18]= offsetof(nodeStatistics, blockReceivedBytes);
        disp[19]= offsetof(nodeStatistics, blockSentBytes);
        disp[20]= offsetof(nodeStatistics, longestFork);
        disp[21]= offsetof(nodeStatistics, blocksInForks);
        disp[22]= offsetof(nodeStatistics, connections);
        disp[23]= offsetof(nodeStatistics, minedBlocksInMainChain);
        disp[24]= offsetof(nodeStatistics, blockTimeouts);
        disp[25]= offsetof(nodeStatistics, nodeGeneratedTransaction);
        disp[26]= offsetof(nodeStatistics, meanEndorsementTime);
        disp[27]= offsetof(nodeStatistics, meanOrderingTime);
        disp[28]= offsetof(nodeStatistics, meanValidationTime);
        disp[29]= offsetof(nodeStatistics, meanLatency);
        disp[30]= offsetof(nodeStatistics, nodeType);
        disp[31]= offsetof(nodeStatistics, meanNumberofTransactions);

        MPI_Type_create_struct(32, blocklen, disp, dtypes, &mpi_nodeStatisticsType);
        MPI_Type_commit(&mpi_nodeStatisticsType);

        if(systemId != 0 && systemCount > 1)
        {
            for(int i = 0 ; i < totalNoNodes; i++)
            {
                Ptr<Node> targetNode = blockchainTopologyHelper.GetNode(i);
                if(systemId == targetNode->GetSystemId())
                {
                    MPI_Send(&stats[i], 1, mpi_nodeStatisticsType, 0, 8888, MPI_COMM_WORLD);
                }
            }
        }
        else if(systemId == 0 && systemCount >1)
        {
            int count = nodesInSystemId0;

            while(count < totalNoNodes)
            {
                MPI_Status status;
                nodeStatistics recv;

                MPI_Recv(&recv, 1, mpi_nodeStatisticsType, MPI_ANY_SOURCE, 8888, MPI_COMM_WORLD, &status);

                stats[recv.nodeId].nodeId = recv.nodeId;
                stats[recv.nodeId].meanBlockReceiveTime =recv.meanBlockReceiveTime;
                stats[recv.nodeId].meanBlockPropagationTime = recv.meanBlockPropagationTime;
                stats[recv.nodeId].meanBlockSize =recv.meanBlockSize;
                stats[recv.nodeId].totalBlocks =recv.totalBlocks;
                stats[recv.nodeId].miner = recv.miner;
                stats[recv.nodeId].minerGeneratedBlocks =recv.minerGeneratedBlocks;
                stats[recv.nodeId].minerAverageBlockGenInterval =recv.minerAverageBlockGenInterval;
                stats[recv.nodeId].minerAverageBlockSize =recv.minerAverageBlockSize;
                stats[recv.nodeId].hashRate = recv.hashRate;
                stats[recv.nodeId].invReceivedBytes =recv.invReceivedBytes;
                stats[recv.nodeId].invSentBytes =recv.invSentBytes;
                stats[recv.nodeId].getHeadersReceivedBytes =recv.getHeadersReceivedBytes;
                stats[recv.nodeId].getHeadersSentBytes=recv.getHeadersSentBytes;
                stats[recv.nodeId].headersReceivedBytes=recv.headersReceivedBytes;
                stats[recv.nodeId].headersSentBytes=recv.headersSentBytes;
                stats[recv.nodeId].getDataReceivedBytes=recv.getDataReceivedBytes;
                stats[recv.nodeId].getDataSentBytes =recv.getDataSentBytes;
                stats[recv.nodeId].blockReceivedBytes=recv.blockReceivedBytes;
                stats[recv.nodeId].blockSentBytes=recv.blockSentBytes;
                stats[recv.nodeId].longestFork=recv.longestFork;
                stats[recv.nodeId].blocksInForks=recv.blocksInForks;
                stats[recv.nodeId].connections=recv.connections;
                stats[recv.nodeId].minedBlocksInMainChain=recv.minedBlocksInMainChain;
                stats[recv.nodeId].blockTimeouts =recv.blockTimeouts;
                stats[recv.nodeId].nodeGeneratedTransaction =recv.nodeGeneratedTransaction;
                stats[recv.nodeId].meanEndorsementTime =recv.meanEndorsementTime;
                stats[recv.nodeId].meanOrderingTime =recv.meanOrderingTime;
                stats[recv.nodeId].meanValidationTime =recv.meanValidationTime;
                stats[recv.nodeId].meanLatency =recv.meanLatency;
                stats[recv.nodeId].nodeType =recv.nodeType;
                stats[recv.nodeId].meanNumberofTransactions = recv.meanNumberofTransactions;
                count++;
            }
        }
    #endif

    if(systemId ==0)
    {
        tFinish = get_wall_time();

        PrintTotalStats(stats, totalNoNodes, tStartSimulation, tFinish, averageBlockGenIntervalMinuates);
        std::cout<<"\nThe simulation run for " << tFinish - tStart << "s simulating"
                    << stop << " mins, Performed " << "Setup time = " << tStartSimulation - tStart << "s\n"
                    << "It consisted of " << totalNoNodes << "nodes (" << noMiners << "miners) with minConnectionPerNode= "
                    << minConnectionsPerNode << " and maxConnectionsperNode = " << maxConnectionsPerNode 
                    << " .\n the averageBlockGenIntervalSeconds was " << averageBlockGenIntervalSeconds << "sec.\n";

    }

    #ifdef MPI_TEST
        MpiInterface::Disable();
    #endif

    delete[] stats;
    return 0;

    #else
    NS_FATAL_ERROR("Can't use distributed simulator withour MPI compiled in");
    #endif

}

double get_wall_time()
{
    struct timeval time;
    if(gettimeofday(&time, NULL))
    {
        //handle error
        return 0;
    }
}

int GetNodeByIpv(Ipv4InterfaceContainer container, Ipv4Address addr)
{
    for(auto it = container.Begin(); it != container.End(); it++)
    {
        int32_t interface = it->first->GetInterfaceForAddress(addr);
        if(interface != -1)
        {
            return it->first->GetNetDevice(interface)->GetNode()->GetId();
        }
    }
}

void PrintStatsForEachNode(nodeStatistics *stats, int totalNodes)
{

    int secPerMin = 60;

    for(int it = 0; it < totalNodes; it++)
    {
        std::cout << "\nNode " << stats[it].nodeId << " statistics:\n";
        std::cout << "Connections = " << stats[it].connections << "\n";
        std::cout << "Mean Block Receive Time = " << stats[it].meanBlockReceiveTime << " or " 
                << static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin << "min and " 
                << stats[it].meanBlockReceiveTime - static_cast<int>(stats[it].meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
        std::cout << "Mean Block Propagation Time = " << stats[it].meanBlockPropagationTime << "s\n";
        std::cout << "Mean Block Size = " << stats[it].meanBlockSize << " Bytes\n";
        std::cout << "Total Blocks = " << stats[it].totalBlocks << "\n";
        std::cout << "The size of the longest fork was " << stats[it].longestFork << " blocks\n";
        std::cout << "There were in total " << stats[it].blocksInForks << " blocks in forks\n";
        std::cout << "The total received INV messages were " << stats[it].invReceivedBytes << " Bytes\n";
        std::cout << "The total received GET_HEADERS messages were " << stats[it].getHeadersReceivedBytes << " Bytes\n";
        std::cout << "The total received HEADERS messages were " << stats[it].headersReceivedBytes << " Bytes\n";
        std::cout << "The total received GET_DATA messages were " << stats[it].getDataReceivedBytes << " Bytes\n";
        std::cout << "The total received BLOCK messages were " << stats[it].blockReceivedBytes << " Bytes\n";
        std::cout << "The total sent INV messages were " << stats[it].invSentBytes << " Bytes\n";
        std::cout << "The total sent GET_HEADERS messages were " << stats[it].getHeadersSentBytes << " Bytes\n";
        std::cout << "The total sent HEADERS messages were " << stats[it].headersSentBytes << " Bytes\n";
        std::cout << "The total sent GET_DATA messages were " << stats[it].getDataSentBytes << " Bytes\n";
        std::cout << "The total sent BLOCK messages were " << stats[it].blockSentBytes << " Bytes\n";
       
        if ( stats[it].miner == 1)
        {
            std::cout << "The miner " << stats[it].nodeId << " with hash rate = " << stats[it].hashRate*100 << "% generated " << stats[it].minerGeneratedBlocks 
                    << " blocks "<< "(" << 100. * stats[it].minerGeneratedBlocks / (stats[it].totalBlocks - 1)
                    << "%) with average block generation time = " << stats[it].minerAverageBlockGenInterval
                    << "s or " << static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin << "min and " 
                    << stats[it].minerAverageBlockGenInterval - static_cast<int>(stats[it].minerAverageBlockGenInterval) / secPerMin * secPerMin << "s"
                    << " and average size " << stats[it].minerAverageBlockSize << " Bytes\n";
        }
    }

}

void PrintTotalStats(nodeStatistics *stats, int totalNodes, double start, double finish, double averageBlockGenIntervalMinutes)
{
    const int  secPerMin = 60;
    double     meanBlockReceiveTime = 0;
    double     meanBlockPropagationTime = 0;
    double     meanMinersBlockPropagationTime = 0;
    double     meanBlockSize = 0;
    double     totalBlocks = 0;
    double     invReceivedBytes = 0;
    double     invSentBytes = 0;
    double     getHeadersReceivedBytes = 0;
    double     getHeadersSentBytes = 0;
    double     headersReceivedBytes = 0;
    double     headersSentBytes = 0;
    double     getDataReceivedBytes = 0;
    double     getDataSentBytes = 0;
    double     blockReceivedBytes = 0;
    double     blockSentBytes = 0;
    double     longestFork = 0;
    double     blocksInForks = 0;
    double     averageBandwidthPerNode = 0;
    double     connectionsPerNode = 0;
    double     connectionsPerMiner = 0;
    double     download = 0;
    double     upload = 0;
    double     meanEndorsementTime = 0;
    double     meanValidationTime = 0;
    double     meanOrderingTime = 0;
    double     meanLatency = 0;
    double     meanNumberofTransactions = 0;

    int        nodeType = 0;
    uint32_t   nodes = 0;
    uint32_t   miners = 0;
    std::vector<double>    propagationTimes;
    std::vector<double>    minersPropagationTimes;
    std::vector<double>    downloadBandwidths;
    std::vector<double>    uploadBandwidths;
    std::vector<double>    totalBandwidths;
    std::vector<long>      blockTimeouts;
  
    for (int it = 0; it < totalNodes; it++ )
    {
        meanBlockReceiveTime = meanBlockReceiveTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                            + stats[it].meanBlockReceiveTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
        meanBlockPropagationTime = meanBlockPropagationTime*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                            + stats[it].meanBlockPropagationTime*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
        meanBlockSize = meanBlockSize*totalBlocks/(totalBlocks + stats[it].totalBlocks)
                    + stats[it].meanBlockSize*stats[it].totalBlocks/(totalBlocks + stats[it].totalBlocks);
        totalBlocks += stats[it].totalBlocks;
        invReceivedBytes = invReceivedBytes*it/static_cast<double>(it + 1) + stats[it].invReceivedBytes/static_cast<double>(it + 1);
        invSentBytes = invSentBytes*it/static_cast<double>(it + 1) + stats[it].invSentBytes/static_cast<double>(it + 1);
        getHeadersReceivedBytes = getHeadersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersReceivedBytes/static_cast<double>(it + 1);
        getHeadersSentBytes = getHeadersSentBytes*it/static_cast<double>(it + 1) + stats[it].getHeadersSentBytes/static_cast<double>(it + 1);
        headersReceivedBytes = headersReceivedBytes*it/static_cast<double>(it + 1) + stats[it].headersReceivedBytes/static_cast<double>(it + 1);
        headersSentBytes = headersSentBytes*it/static_cast<double>(it + 1) + stats[it].headersSentBytes/static_cast<double>(it + 1);
        getDataReceivedBytes = getDataReceivedBytes*it/static_cast<double>(it + 1) + stats[it].getDataReceivedBytes/static_cast<double>(it + 1);
        getDataSentBytes = getDataSentBytes*it/static_cast<double>(it + 1) + stats[it].getDataSentBytes/static_cast<double>(it + 1);
        blockReceivedBytes = blockReceivedBytes*it/static_cast<double>(it + 1) + stats[it].blockReceivedBytes/static_cast<double>(it + 1);
        blockSentBytes = blockSentBytes*it/static_cast<double>(it + 1) + stats[it].blockSentBytes/static_cast<double>(it + 1);
        longestFork = longestFork*it/static_cast<double>(it + 1) + stats[it].longestFork/static_cast<double>(it + 1);
        blocksInForks = blocksInForks*it/static_cast<double>(it + 1) + stats[it].blocksInForks/static_cast<double>(it + 1);
        
        propagationTimes.push_back(stats[it].meanBlockPropagationTime);

        download = stats[it].invReceivedBytes + stats[it].getHeadersReceivedBytes + stats[it].headersReceivedBytes
                + stats[it].getDataReceivedBytes + stats[it].blockReceivedBytes;
        upload = stats[it].invSentBytes + stats[it].getHeadersSentBytes + stats[it].headersSentBytes
            + stats[it].getDataSentBytes + stats[it].blockSentBytes;
        download = download / (1000 *(stats[it].totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8;
        upload = upload / (1000 *(stats[it].totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8;
        downloadBandwidths.push_back(download);  
        uploadBandwidths.push_back(upload);     	  
        totalBandwidths.push_back(download + upload); 
        blockTimeouts.push_back(stats[it].blockTimeouts);

        if(stats[it].miner == 0)
        {
            connectionsPerNode = connectionsPerNode*nodes/static_cast<double>(nodes + 1) + stats[it].connections/static_cast<double>(nodes + 1);
            nodes++;
        }
        else
        {
            connectionsPerMiner = connectionsPerMiner*miners/static_cast<double>(miners + 1) + stats[it].connections/static_cast<double>(miners + 1);
            meanMinersBlockPropagationTime = meanMinersBlockPropagationTime*miners/static_cast<double>(miners + 1) + stats[it].meanBlockPropagationTime/static_cast<double>(miners + 1);
            minersPropagationTimes.push_back(stats[it].meanBlockPropagationTime);
            meanNumberofTransactions = (meanNumberofTransactions*static_cast<double>(miners) + stats[it].meanNumberofTransactions)/static_cast<double>(miners + 1);
            miners++;
        }

        if(stats[it].nodeType == 0)
        {
            meanValidationTime = (meanValidationTime*it + stats[it].meanValidationTime)/static_cast<double>(it+1) ;
        }
        else if(stats[it].nodeType == 1)
        {
            meanValidationTime = (meanValidationTime*it + stats[it].meanValidationTime)/static_cast<double>(it+1) ;
            meanEndorsementTime = (meanEndorsementTime*it + stats[it].meanEndorsementTime)/static_cast<double>(it+1);
        }
        else if(stats[it].nodeType ==2)
        {
            meanLatency = (meanLatency*it + stats[it].meanLatency)/static_cast<double>(it+1);
        }
        else
        {
            meanOrderingTime = (meanOrderingTime*it + stats[it].meanOrderingTime)/static_cast<double>(it+1);
        }

    }
  
    averageBandwidthPerNode = invReceivedBytes + invSentBytes + getHeadersReceivedBytes + getHeadersSentBytes + headersReceivedBytes
                            + headersSentBytes + getDataReceivedBytes + getDataSentBytes + blockReceivedBytes + blockSentBytes;

    totalBlocks /= totalNodes;

    //sort(propagationTimes.begin(), propagationTimes.end());
    sort(minersPropagationTimes.begin(), minersPropagationTimes.end());
    sort(blockTimeouts.begin(), blockTimeouts.end());
  
    double median = *(propagationTimes.begin()+propagationTimes.size()/2);
    double p_10 = *(propagationTimes.begin()+int(propagationTimes.size()*.1));
    double p_25 = *(propagationTimes.begin()+int(propagationTimes.size()*.25));
    double p_75 = *(propagationTimes.begin()+int(propagationTimes.size()*.75));
    double p_90 = *(propagationTimes.begin()+int(propagationTimes.size()*.90));
    double minersMedian = *(minersPropagationTimes.begin()+int(minersPropagationTimes.size()/2));
    
    std::cout << "\nTotal Stats:\n";
    std::cout << "Average Connections/node = " << connectionsPerNode << "\n";
    std::cout << "Average Connections/miner = " << connectionsPerMiner << "\n";
    std::cout << "Mean Block Receive Time = " << meanBlockReceiveTime << " or " 
                << static_cast<int>(meanBlockReceiveTime) / secPerMin << "min and " 
                << meanBlockReceiveTime - static_cast<int>(meanBlockReceiveTime) / secPerMin * secPerMin << "s\n";
    std::cout << "Mean Block Propagation Time = " << meanBlockPropagationTime << "s\n";
    //std::cout << "Median Block Propagation Time = " << median << "s\n";
    std::cout << "Miners Mean Block Propagation Time = " << meanMinersBlockPropagationTime << "s\n";
    std::cout << "Miners Median Block Propagation Time = " << minersMedian << "s\n";
    std::cout << "Mean Block Size = " << meanBlockSize << " Bytes\n";
    std::cout << "Total Blocks = " << totalBlocks << "\n";
    std::cout << "The average received INV messages were " << invReceivedBytes << " Bytes (" 
                << 100. * invReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received GET_HEADERS messages were " << getHeadersReceivedBytes << " Bytes (" 
                << 100. * getHeadersReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received HEADERS messages were " << headersReceivedBytes << " Bytes (" 
                << 100. * headersReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received GET_DATA messages were " << getDataReceivedBytes << " Bytes (" 
                << 100. * getDataReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average received BLOCK messages were " << blockReceivedBytes << " Bytes (" 
                << 100. * blockReceivedBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent INV messages were " << invSentBytes << " Bytes (" 
                << 100. * invSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent GET_HEADERS messages were " << getHeadersSentBytes << " Bytes (" 
                << 100. * getHeadersSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent HEADERS messages were " << headersSentBytes << " Bytes (" 
                << 100. * headersSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent GET_DATA messages were " << getDataSentBytes << " Bytes (" 
                << 100. * getDataSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "The average sent BLOCK messages were " << blockSentBytes << " Bytes (" 
                << 100. * blockSentBytes / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to INV messages = " << invReceivedBytes +  invSentBytes << " Bytes(" 
                << 100. * (invReceivedBytes +  invSentBytes) / averageBandwidthPerNode << "%)\n";	
    std::cout << "Total average traffic due to GET_HEADERS messages = " << getHeadersReceivedBytes +  getHeadersSentBytes << " Bytes(" 
                << 100. * (getHeadersReceivedBytes +  getHeadersSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to HEADERS messages = " << headersReceivedBytes +  headersSentBytes << " Bytes(" 
                << 100. * (headersReceivedBytes +  headersSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic due to GET_DATA messages = " << getDataReceivedBytes +  getDataSentBytes << " Bytes(" 
                << 100. * (getDataReceivedBytes +  getDataSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic4 due to BLOCK messages = " << blockReceivedBytes +  blockSentBytes << " Bytes(" 
                << 100. * (blockReceivedBytes +  blockSentBytes) / averageBandwidthPerNode << "%)\n";
    std::cout << "Total average traffic/node = " << averageBandwidthPerNode << " Bytes (" 
                << averageBandwidthPerNode / (1000 *(totalBlocks - 1) * averageBlockGenIntervalMinutes * secPerMin) * 8
                << " Kbps and " << averageBandwidthPerNode / (1000 * (totalBlocks - 1)) << " KB/block)\n";
    std::cout << (finish - start)/ (totalBlocks - 1)<< "s per generated block\n";
    std::cout << "meanEndorsementTime =" << meanEndorsementTime <<"s \n";
    std::cout << "meanOrderingTime =" << meanOrderingTime <<"s \n";
    std::cout << "meanValidationTime =" << meanValidationTime <<"s \n";
    std::cout << "meanLatency =" << meanLatency <<"s \n";
    std::cout << "Ths average transactions in a block =" << meanNumberofTransactions <<"\n";
    
    
    std::cout << "\nBlock Propagation Times = [";
    for(auto it = propagationTimes.begin(); it != propagationTimes.end(); it++)
    {
        if (it == propagationTimes.begin())
        std::cout << *it;
        else
        std::cout << ", " << *it ;
    }
    std::cout << "]\n" ;

    std::cout << "\nTotal Bandwidths = [";
    double average = 0;
    for(auto it = totalBandwidths.begin(); it != totalBandwidths.end(); it++)
    {
        if (it == totalBandwidths.begin())
        std::cout << *it;
        else
        std::cout << ", " << *it ;
        average += *it;
    }
    std::cout << "] average = " << average/totalBandwidths.size() << "\n" ;

}

void PrintBlockchainRegionStats(uint32_t *blockchainNodeRegions, uint32_t totalNodes)
{
    uint32_t regions[7] = {0, 0, 0, 0, 0, 0, 0};

    for(uint32_t i = 0 ; i < totalNodes; i++)
    {
        regions[blockchainNodeRegions[i]]++;
    }
    
    std::cout << "Nodes distribution \n";
    for(uint32_t i = 0; i < 7; i++)
    {
        std::cout << getBlockchainRegion(getBlockchainEnum(i)) << " : " <<regions[i]*100/totalNodes << "%\n";
    }
}

