#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/blockchain-miner.h"
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <random>

static double GetWallTime();

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("BlockchainMiner");
    NS_OBJECT_ENSURE_REGISTERED(BlockchainMiner);

    TypeId
    BlockchainMiner::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3:BlockchainMiner")
            .SetParent<Application>()
            .SetGroupName("Application")
            .AddConstructor<BlockchainMiner>()
            .AddAttribute("Local",
                            "The Address on which to Bind the rx socket.",
                            AddressValue(),
                            MakeAddressAccessor(&BlockchainMiner::m_local),
                            MakeAddressChecker())
            .AddAttribute("Protocol", 
                            "The type id of the protocol to use for the rx socket.",
                            TypeIdValue(UdpSocketFactory::GetTypeId()),
                            MakeTypeIdAccessor (&BlockchainMiner::m_tid),
                            MakeTypeIdChecker())
            .AddAttribute("NumberOfMiners",
                            "The number of miners",
                            UintegerValue(16),
                            MakeUintegerAccessor (&BlockchainMiner::m_noMiners),
                            MakeUintegerChecker<uint32_t>())
            .AddAttribute("FixedBlockSize",
                            "The fixed size of the block",
                            UintegerValue(0),
                            MakeUintegerAccessor(&BlockchainMiner::m_fixedBlockSize),
                            MakeUintegerChecker<uint32_t>())
            .AddAttribute("FixedBlockIntervalGeneration",
                            "The fixed time to wait between two consecutive block generations",
                            DoubleValue(0),
                            MakeDoubleAccessor(&BlockchainMiner::m_fixedBlockTimeGeneration),
                            MakeDoubleChecker<double>())
            .AddAttribute("InvTimeoutMinutes",
                            "The timeout of inv messages in minutes",
                            TimeValue(Minutes(2)),
                            MakeTimeAccessor(&BlockchainMiner::m_invTimeoutMinutes),
                            MakeTimeChecker())
            .AddAttribute("HashRate",
                            "The hash rate of the miner",
                            DoubleValue(0.2),
                            MakeDoubleAccessor(&BlockchainMiner::m_hashRate),
                            MakeDoubleChecker<double>())
            .AddAttribute("BlockGenBinSize",
                            "The block generation bin size",
                            DoubleValue(-1),
                            MakeDoubleAccessor(&BlockchainMiner::m_blockGenBinSize),
                            MakeDoubleChecker<double>())
            .AddAttribute("BlockGenParameter",
                            "The block generation distribution parameter",
                            DoubleValue(-1),
                            MakeDoubleAccessor(&BlockchainMiner::m_blockGenParameter),
                            MakeDoubleChecker<double>())
            .AddAttribute("AverageBlockGenIntervalSeconds",
                            "The average block generation interval we ami at (in seconds) ", 
                            DoubleValue(15),
                            MakeDoubleAccessor(&BlockchainMiner::m_averageBlockGenIntervalSeconds),
                            MakeDoubleChecker<double>())
            .AddAttribute("Cryptocurrency",
                            "ETHEREUM, HYPERLEDGER",
                            UintegerValue(0),
                            MakeUintegerAccessor(&BlockchainMiner::m_cryptocurrency),
                            MakeUintegerChecker<uint32_t> ())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&BlockchainMiner::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback")
            ;
            return tid;
    }

    BlockchainMiner::BlockchainMiner() : BlockchainNode(), m_realAverageBlockGenIntervalSeconds(15),
                                        m_timeStart(0), m_timeFinish(0), m_fistToMine(false)
    {
        NS_LOG_FUNCTION(this);
        m_minerAverageBlockGenInterval = 0;
        m_minerGeneratedBlocks = 0;
        m_previousBlockGenerationTime = 0;
        m_meanNumberofTransactions = 0;

        std::random_device rd;
        m_generator.seed(rd());

        if(m_fixedBlockTimeGeneration > 0)
        {
            m_nextBlockTime = m_fixedBlockTimeGeneration;
        }
        else
        {
            m_nextBlockTime = 0;
        }

        if(m_fixedBlockSize > 0)
        {
            m_nextBlockSize = m_fixedBlockSize;
        }
        else
        {
            m_nextBlockSize = 0;
        }

        m_isMiner = true;

    }

    BlockchainMiner::~BlockchainMiner(void)
    {
        NS_LOG_FUNCTION(this);


    }

    void
    BlockchainMiner::StartApplication()
    {
        NS_LOG_FUNCTION(this);
        
        BlockchainNode::StartApplication();
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_noMiners = " << m_noMiners << "");
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_realAverageBlockGenIntervalSeconds = " << m_realAverageBlockGenIntervalSeconds << " s" );
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_averageBlockGenIntervalSeconds = " << m_averageBlockGenIntervalSeconds << " s");
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_fixedBlockTimeGeneration = " << m_fixedBlockTimeGeneration << " s");
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_hashRate = " << m_hashRate);
        NS_LOG_WARN("Miner " << GetNode()->GetId() << " m_cryptocurrency = " << m_cryptocurrency);
        
        if(m_blockGenBinSize < 0 && m_blockGenParameter <0)
        {
            m_blockGenBinSize = 1.0/60/1000;
            m_blockGenParameter = 0.19 * m_blockGenBinSize / 2;
            //std::cout<< "m_blockGenBinSize : "<< m_blockGenBinSize << "\n";
            //std::cout<< "m_blockGenParameter : "<< m_blockGenParameter << "\n";
        }
        else
        {
            m_blockGenParameter *= m_hashRate;
            //std::cout<< "m_blockGenParameter : "<< m_blockGenParameter << "\n";
        }

        if(m_fixedBlockTimeGeneration == 0)
        {
            m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter));
        }

        if(m_fixedBlockSize > 0)
        {
            m_nextBlockSize = m_fixedBlockSize;
        }

        m_nodeStats->hashRate = m_hashRate;
        m_nodeStats->miner = 1;

        ScheduleNextMiningEvent();

    }

    void
    BlockchainMiner::StopApplication()
    {
        NS_LOG_FUNCTION(this);
        
        BlockchainNode::StopApplication();
        Simulator::Cancel(m_nextMiningEvent);

        NS_LOG_WARN("The miner " << GetNode()->GetId() << " with hash rate = " << m_hashRate
                    << " generated " << m_minerGeneratedBlocks << " blocks " << " ( " << 100.0*m_minerGeneratedBlocks/(m_blockchain.GetTotalBlocks()-1)
                    << "%) with average block generation time = " << m_minerAverageBlockGenInterval
                    << "s or " << static_cast<int>(m_minerAverageBlockGenInterval)/60 << "min and "
                    << m_minerAverageBlockGenInterval - static_cast<int>(m_minerAverageBlockGenInterval)/60*60 << "s" 
                    << " and average size " << m_minerAverageBlockSize << " Bytes");

        m_nodeStats->minerGeneratedBlocks = m_minerGeneratedBlocks;
        m_nodeStats->minerAverageBlockGenInterval = m_minerAverageBlockGenInterval;
        m_nodeStats->minerAverageBlockSize = m_minerAverageBlockSize;
        m_nodeStats->meanNumberofTransactions = m_meanNumberofTransactions;

        if(m_fistToMine)
        {
            m_timeFinish = GetWallTime();
            std::cout << "Time/Block = " << (m_timeFinish - m_timeStart)/(m_blockchain.GetTotalBlocks()-1) << "s\n";
        }
    }

    void
    BlockchainMiner::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        BlockchainNode::DoDispose();
    }

    double
    BlockchainMiner::GetFixedBlockTimeGeneration(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_fixedBlockTimeGeneration;
    }

    void
    BlockchainMiner::SetFixedBlockTimeGeneration(double fixedBlockTimeGeneration)
    {
        NS_LOG_FUNCTION(this);
        m_fixedBlockTimeGeneration = fixedBlockTimeGeneration;
    }

    uint32_t
    BlockchainMiner::GetFixedBlockSize(void) const
    {   
        NS_LOG_FUNCTION(this);
        return m_fixedBlockSize;
    }

    void
    BlockchainMiner::SetFixedBlockSize(uint32_t fixedBlockSize)
    {
        NS_LOG_FUNCTION(this);
        m_fixedBlockSize = fixedBlockSize;
    }

    double
    BlockchainMiner::GetBlockGenBinSize(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_blockGenBinSize;
    }

    void
    BlockchainMiner::SetBlockGenBinSize(double blockGenBinSize)
    {
        NS_LOG_FUNCTION(this);
        m_blockGenBinSize = blockGenBinSize;
    }

    double
    BlockchainMiner::GetBlockGenParameter(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_blockGenParameter;
    }

    void
    BlockchainMiner::SetBlockGenParameter(double blockGenParameter)
    {
        NS_LOG_FUNCTION(this);
        m_blockGenParameter = blockGenParameter;
        m_blockGenTimeDistribution.param(std::geometric_distribution<int>::param_type(m_blockGenParameter));

    }

    double
    BlockchainMiner::GetHashRate(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_hashRate;
    }

    void
    BlockchainMiner::SetHashRate(double hashRate)
    {
        NS_LOG_FUNCTION(this);
        m_hashRate = hashRate;
    }

    void
    BlockchainMiner::ScheduleNextMiningEvent(void)
    {
        NS_LOG_FUNCTION(this);
        //std::cout<< "Schedule Mine application\n";

        if(m_fixedBlockTimeGeneration > 0)
        {
            m_nextBlockTime = m_fixedBlockTimeGeneration;
            m_nextMiningEvent = Simulator::Schedule(Seconds(m_fixedBlockTimeGeneration), &BlockchainMiner::MineBlock, this);
            //std::cout<<"m_nextBlockTime(1) : " << m_nextBlockTime <<"\n";
        }
        else
        {
            /*
            m_nextBlockTime = m_blockGenTimeDistribution(m_generator)*m_blockGenBinSize*60
                                *(m_averageBlockGenIntervalSeconds/m_realAverageBlockGenIntervalSeconds)/m_hashRate;
            //std::cout<< "m_blockGenTimeDistribution(m_generator) : "<<m_blockGenTimeDistribution(m_generator)<<"\n";
            //std::cout<<"m_nextBlockTime(2) : " << m_nextBlockTime <<"\n";
            m_nextMiningEvent = Simulator::Schedule(Seconds(m_nextBlockTime), &BlockchainMiner::MineBlock, this);
            */
            m_nextBlockTime = 2;
            m_nextMiningEvent = Simulator::Schedule(Seconds(m_nextBlockTime), &BlockchainMiner::MineBlock, this);
        }
    }

    void
    BlockchainMiner::MineBlock(void)
    {   
        NS_LOG_FUNCTION(this);
        //std::cout<< "Start MineBlock function\n";
        rapidjson::Document inv;
        rapidjson::Document block;

        std::vector<Transaction>::iterator      trans_it;
        int height = m_blockchain.GetCurrentTopBlock()->GetBlockHeight() + 1;
        int minerId = GetNode()->GetId();
        int nonce = 0;
        int parentBlockMinerId = m_blockchain.GetCurrentTopBlock()->GetMinerId();
        double currentTime = Simulator::Now().GetSeconds();
        std::ostringstream stringStream;
        std::string blockHash;

        stringStream << height << "/" << minerId;
        blockHash = stringStream.str();

        inv.SetObject();
        block.SetObject();

        if(height == 1)
        {
            m_fistToMine = true;
            m_timeStart = GetWallTime();
        }

        if(m_fixedBlockSize > 0)
        {
            m_nextBlockSize = m_fixedBlockSize;
            
        }
        else
        {
            std::normal_distribution<double> dist(23.0, 2.0);
            //m_nextBlockSize = dist(m_generator);
            m_nextBlockSize = (int)(dist(m_generator)*1000);
            //std::cout <<(int)(dist(m_generator)*1000) <<"\n";
        }

        if(m_nextBlockSize < m_averageTransacionSize)
        {
            m_nextBlockSize = m_averageTransacionSize + m_headersSizeBytes;
        }

        Block newBlock(height, minerId, nonce, parentBlockMinerId, m_nextBlockSize,
                        currentTime, currentTime, Ipv4Address("127.0.0.1"));
        
        /*
         * Push transactions to new Blocks
         */
        
        for(trans_it = m_notValidatedTransaction.begin(); trans_it < m_notValidatedTransaction.end(); trans_it++)
        {
            trans_it->SetValidation();
            m_totalOrdering++;
            m_meanOrderingTime = (m_meanOrderingTime*static_cast<double>(m_totalOrdering-1) + (Simulator::Now().GetSeconds() - trans_it->GetTransTimeStamp()))/static_cast<double>(m_totalOrdering);

        }
        //std::cout<<m_notValidatedTransaction.size()<<"\n";
        m_meanNumberofTransactions = (m_meanNumberofTransactions*static_cast<double>(m_minerGeneratedBlocks) + m_notValidatedTransaction.size())/static_cast<double>(m_minerGeneratedBlocks+1);
        newBlock.SetTransactions(m_notValidatedTransaction);
        m_notValidatedTransaction.clear();

        //newBlock.PrintAllTransaction();
        
        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        //rapidjson::Value blockInfor(rapidjson::kObjectType);

        value.SetString("block");
        inv.AddMember("type", value, inv.GetAllocator());

        if(m_protocolType == STANDARD_PROTOCOL)
        {
            value = INV;
            inv.AddMember("message", value, inv.GetAllocator());

            value.SetString(blockHash.c_str(), blockHash.size(), inv.GetAllocator());
            array.PushBack(value, inv.GetAllocator());
            inv.AddMember("inv", array, inv.GetAllocator());
        }

        m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime
                                + (currentTime - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());
        m_previousBlockReceiveTime = currentTime;

        m_meanBlockPropagationTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime;

        m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockSize     
                            + (m_nextBlockSize)/static_cast<double>(m_blockchain.GetTotalBlocks());
        
        m_blockchain.AddBlock(newBlock);

        rapidjson::StringBuffer invInfo;
        rapidjson::Writer<rapidjson::StringBuffer> invWriter(invInfo);
        inv.Accept(invWriter);

        rapidjson::StringBuffer blockInfo;
        rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
        block.Accept(blockWriter);

        //std::cout<< "MineBlock function : Add a new block in packet\n";

        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            
            const uint8_t delimiter[] = "#";

            m_peersSockets[*i]->Send(reinterpret_cast<const uint8_t*>(invInfo.GetString()), invInfo.GetSize(), 0);
            m_peersSockets[*i]->Send(delimiter, 1, 0);
            
            m_nodeStats->invSentBytes += m_blockchainMessageHeader + m_countBytes + inv["inv"].Size()*m_inventorySizeBytes;
            //std::cout<< "Node : " << GetNode()->GetId() <<" complete minning and send packet to " << *i << " \n" ;
            NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                        << " s blockchain miner " << GetNode()->GetId()
                        << " sent a packet " << invInfo.GetString()
                        << " to " << *i);
            

        }
        
        m_minerAverageBlockGenInterval = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockGenInterval
                                        + (Simulator::Now().GetSeconds() - m_previousBlockGenerationTime)/(m_minerGeneratedBlocks+1);

        m_minerAverageBlockSize = m_minerGeneratedBlocks/static_cast<double>(m_minerGeneratedBlocks+1)*m_minerAverageBlockSize
                                + static_cast<double>(m_nextBlockSize)/(m_minerGeneratedBlocks+1);
        m_previousBlockGenerationTime = Simulator::Now().GetSeconds();
        m_minerGeneratedBlocks++;

        ScheduleNextMiningEvent();
        //std::cout<< "MineBlock function : finish MinBLock\n";

    }

    void
    BlockchainMiner::ReceivedHigherBlock(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_WARN("Blockchain miner " << GetNode()->GetId() << " added a new block in the blockchain with higher height");
        Simulator::Cancel(m_nextMiningEvent);
        ScheduleNextMiningEvent();
    }

    void
    BlockchainMiner::SendBlock(std::string packetInfo, Ptr<Socket> socket)
    {
        //std::cout<< "Start SendBlock function\n";
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("SendBlock: At time " << Simulator::Now().GetSeconds()
                    << " s blockchain miner " << GetNode()->GetId() << " send "
                    << packetInfo );
        
        rapidjson::Document d;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        d.Parse(packetInfo.c_str());
        d.Accept(writer);

        SendMessage(NO_MESSAGE, BLOCK, d, socket);
        m_nodeStats->blockSentBytes -= m_blockchainMessageHeader + d["blocks"][0]["size"].GetInt();

    }

}


static double GetWallTime()
{
    struct timeval time;
    if(gettimeofday(&time, NULL))
    {
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}


