#ifndef BLOCKCHAIN_MINER_HELPER_H
#define BLOCKCHAIN_MINER_HELPER_H

#include "ns3/blockchain-node-helper.h"

namespace ns3{

    class BlockchainMinerHelper : public BlockchainNodeHelper
    {
        public:

            BlockchainMinerHelper(std::string protocol, Address address, std::vector<Ipv4Address> peer, int noMiners,
                                std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                nodeInternetSpeed &internetSpeeds, nodeStatistics *stats, double hashRate, double averageBlockGenIntervalSeconds);            

            enum MinerType GetMinerType(void);
            void SetMinerType(enum MinerType m);

        protected:

            virtual Ptr<Application> InstallPriv(Ptr<Node> node);

            void SetFactoryAttributes(void);

            enum MinerType      m_minerType;
            int                 m_noMiners;
            double              m_hashRate;
            double              m_blockGenBinSize;
            double              m_blockGenParameter;
            double              m_averageBlockGenIntervalSeconds;
            enum CommitterType  m_committerType;

        

    };

}

#endif