#ifndef BLOCKCHAIN_MINER_H
#define BLOCKCHAIN_MINER_H

#include "blockchain-node.h"
#include <random>
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"


namespace ns3{

    class Address;
    class Socket;
    class Packet;

    class BlockchainMiner : public BlockchainNode
    {

        public:

            static TypeId GetTypeId(void);
            BlockchainMiner();

            virtual ~BlockchainMiner(void);

            double GetFixedBlockTimeGeneration(void) const;

            void SetFixedBlockTimeGeneration(double fixedBlockTimeGeneration);

            uint32_t GetFixedBlockSize(void) const;

            void SetFixedBlockSize(uint32_t fixedBlockSize);

            double GetBlockGenBinSize(void) const;

            void SetBlockGenBinSize(double m_blockGenBinSize);

            double GetBlockGenParameter(void) const;

            void SetBlockGenParameter(double blockGenParameter);

            double GetHashRate(void) const;

            void SetHashRate(double blockGenParameter);


        protected:

            virtual void StartApplication(void);
            virtual void StopApplication(void);

            virtual void DoDispose(void);

            void ScheduleNextMiningEvent (void);

            virtual void MineBlock(void);

            virtual void ReceivedHigherBlock(const Block &newBlock);

            void SendBlock(std::string packetInfo, Ptr<Socket> socket);

            int                         m_noMiners;
            uint32_t                    m_fixedBlockSize;
            double                      m_fixedBlockTimeGeneration;
            EventId                     m_nextMiningEvent;
            std::default_random_engine  m_generator;

            double                      m_blockGenBinSize;
            double                      m_blockGenParameter;
            double                      m_nextBlockTime;
            double                      m_previousBlockGenerationTime;
            double                      m_minerAverageBlockGenInterval;
            int                         m_minerGeneratedBlocks;
            double                      m_hashRate;
            double                      m_meanNumberofTransactions;

            std::geometric_distribution<int>    m_blockGenTimeDistribution;

            int                         m_nextBlockSize;
            int                         m_maxBlockSize;
            double                      m_minerAverageBlockSize;
            //std::piecewise_constant_distribution<double> m_blockSizeDistribution;
            //std::normal_distribution<double>    m_blockSizeDistribution;
            
            const double                m_realAverageBlockGenIntervalSeconds;   // in seconds, 15 sec(Ethereum)
            double                      m_averageBlockGenIntervalSeconds;


            enum Cryptocurrency  m_cryptocurrency;

            double  m_timeStart;
            double  m_timeFinish;
            bool    m_fistToMine;

    };

}

#endif