#include "ns3/blockchain-miner-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"
#include "ns3/blockchain-miner.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

    BlockchainMinerHelper::BlockchainMinerHelper(std::string protocol, Address address, std::vector<Ipv4Address> peers, int noMiners,
                                                std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                                nodeInternetSpeed &internetSpeeds, nodeStatistics *stats, double hashRate, double averageBlockGenIntervalSeconds)
                                                :BlockchainNodeHelper(), m_minerType(NORMAL_MINER), m_blockGenBinSize(-1), m_blockGenParameter(-1)
    {
        m_factory.SetTypeId("ns3:BlockchainMiner");
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);

        m_noMiners = noMiners;
        m_hashRate = hashRate;
        m_averageBlockGenIntervalSeconds = averageBlockGenIntervalSeconds;
        m_committerType = ORDER;

        m_factory.Set("NumberOfMiners", UintegerValue(m_noMiners));
        m_factory.Set("HashRate" , DoubleValue(m_hashRate));
        m_factory.Set("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));
    }

    Ptr<Application>
    BlockchainMinerHelper::InstallPriv(Ptr<Node> node)
    {
        switch(m_minerType)
        {
            case NORMAL_MINER:
            {
                Ptr<BlockchainMiner> app = m_factory.Create<BlockchainMiner>();
                app->SetPeersAddresses(m_peersAddresses);
                app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
                app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
                app->SetNodeInternetSpeeds(m_internetSpeeds);
                app->SetNodeStats(m_nodeStats);
                app->SetProtocolType(m_protocolType);
                app->SetCommitterType(m_committerType);
                
                node->AddApplication(app);
                
                return app;
                //break;
            }
            case HYPERLEDGER_MINER:
            {
                Ptr<BlockchainMiner> app = m_factory.Create<BlockchainMiner>();
                app->SetPeersAddresses(m_peersAddresses);
                app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
                app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
                app->SetNodeInternetSpeeds(m_internetSpeeds);
                app->SetNodeStats(m_nodeStats);
                app->SetProtocolType(m_protocolType);
                app->SetCommitterType(m_committerType);
                
                node->AddApplication(app);
                
                return app;
            }
        }

        return 0;
    }

    enum MinerType
    BlockchainMinerHelper::GetMinerType(void)
    {
        return m_minerType;
    }

    void
    BlockchainMinerHelper::SetMinerType(enum MinerType m)
    {
        m_minerType = m;

        switch(m)
        {
            case NORMAL_MINER:
            {
                m_factory.SetTypeId("ns3::BlockchainMiner");
                SetFactoryAttributes();
                break;
            }
            case HYPERLEDGER_MINER:
            {
                m_factory.SetTypeId("ns3::BlockchainMiner");
                SetFactoryAttributes();
            }
        }
    }

    void
    BlockchainMinerHelper::SetFactoryAttributes(void)
    {
        m_factory.Set("Protocol", StringValue(m_protocol));
        m_factory.Set("Local", AddressValue(m_address));
        m_factory.Set("NumberOfMiners", UintegerValue(m_noMiners));
        m_factory.Set("HashRate", DoubleValue(m_hashRate));
        m_factory.Set("AverageBlockGenIntervalSeconds", DoubleValue(m_averageBlockGenIntervalSeconds));

        if(m_blockGenBinSize > 0 && m_blockGenParameter)
        {
            m_factory.Set("BlockGenBinSize", DoubleValue(m_blockGenBinSize));
            m_factory.Set("BlockGenParameter", DoubleValue(m_blockGenParameter));
        }
    }

}