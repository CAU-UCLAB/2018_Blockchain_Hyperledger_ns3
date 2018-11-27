#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <map>
#include <algorithm>
#include "ns3/address.h"

namespace ns3 {

    enum Messages
    {
        INV,            //0
        REQUEST_TRANS,  //1
        GET_HEADERS,    //2
        HEADERS,        //3
        GET_DATA,       //4
        BLOCK,          //5    
        NO_MESSAGE,     //6
        REPLY_TRANS,    //7
        MSG_TRANS,      //8
        RESULT_TRANS,   //9
    };

    enum MinerType
    {
        NORMAL_MINER,
        HYPERLEDGER_MINER,
    };

    enum CommitterType
    {
        COMMITTER,  //0
        ENDORSER,   //1
        CLIENT,     //2
        ORDER,      //3
    };
    
    enum ProtocolType
    {
        STANDARD_PROTOCOL,      //default
        SENDHEADERS
    };

    enum Cryptocurrency
    {
        ETHEREUM,
        HYPERLEDGER
    };

    enum BlockchainRegion
    {
        NORTH_AMERICA,
        EUROPE,
        SOUTH_AMERICA,
        KOREA,
        JAPAN,
        AUSTRALIA,
        OTHER
    };

    typedef struct{

        int     nodeId;                         // blockchain node ID
        double  meanBlockReceiveTime;        // average of Block receive time
        double  meanBlockPropagationTime;    // average of Block propagation time
        double  meanBlockSize;               // average of Block Size
        int     totalBlocks;
        int     miner;                          //0-> node, 1->miner
        int     minerGeneratedBlocks;
        double  minerAverageBlockGenInterval;
        double  minerAverageBlockSize;
        double  hashRate;
        long    invReceivedBytes;
        long    invSentBytes;
        long    getHeadersReceivedBytes;
        long    getHeadersSentBytes;
        long    headersReceivedBytes;
        long    headersSentBytes;
        long    getDataReceivedBytes;
        long    getDataSentBytes;
        long    blockReceivedBytes;
        long    blockSentBytes;
        int     longestFork;
        int     blocksInForks;
        int     connections;
        int     minedBlocksInMainChain;
        long    blockTimeouts;
        int     nodeGeneratedTransaction;
        double  meanEndorsementTime;
        double  meanOrderingTime;
        double  meanValidationTime;
        double  meanLatency;
        int     nodeType;
        double  meanNumberofTransactions;
      
    
    } nodeStatistics;

    typedef struct{
        double downloadSpeed;
        double uploadSpeed;
    } nodeInternetSpeed;

    const char* getMessageName(enum Messages m);
    const char* getMinerType(enum MinerType m);
    const char* getCommitterType(enum CommitterType m);
    const char* getProtocolType(enum ProtocolType m);
    const char* getCryptocurrency(enum Cryptocurrency m);
    const char* getBlockchainRegion(enum BlockchainRegion m);
    enum BlockchainRegion getBlockchainEnum(uint32_t n);

    class Transaction
    {
        public:

            Transaction(int nodeId, int transId, double timeStamp);
            Transaction();
            virtual ~Transaction(void);

            int GetTransNodeId(void) const;
            void SetTransNodeId(int nodeId);

            int GetTransId(void) const;
            void SetTransId(int transId);

            int GetTransSizeByte(void) const;
            void SetTransSizeByte(int transSizeByte);

            double GetTransTimeStamp(void) const;
            void SetTransTimeStamp(double timeStamp);

            bool IsValidated(void) const;
            void SetValidation();

            int GetExecution(void) const;
            void SetExecution(int endoerserId);

            Transaction& operator = (const Transaction &tranSource);     //Assignment Constructor

            friend bool operator == (const Transaction &tran1, const Transaction &tran2);
            
        
        protected:
            int m_nodeId;
            int m_transId;
            int m_transSizeByte;
            double m_timeStamp;
            bool m_validatation; 
            int m_execution;

    };

    class Block
    {
        
        public:
            Block(int blockHeight, int minerId, int nonce, int parentBlockMinerId, int blockSizeBytes,
                double timeStamp, double timeReceived, Ipv4Address receivedFromIpv4);
            Block();
            Block(const Block &blockSource);
            virtual ~Block(void);

            int GetBlockHeight(void) const;
            void SetBlockHeight(int blockHeight);

            int GetNonce(void) const;
            void SetNonce(int nonce);

            int GetMinerId(void) const;
            void SetMinerId(int minerId);

            int GetParentBlockMinerId(void) const;
            void SetParentBlockMinerId(int parentBlockMinerId);

            int GetBlockSizeBytes(void) const;
            void SetBlockSizeBytes(int blockSizeBytes);

            double GetTimeStamp(void) const;
            void SetTimeStamp(double timeStamp);

            double GetTimeReceived(void) const;
            
            Ipv4Address GetReceivedFromIpv4(void) const;
            void SetReceivedFromIpv4(Ipv4Address receivedFromIpv4);

            std::vector<Transaction> GetTransactions(void) const;
            void SetTransactions(const std::vector<Transaction> &transactions);
            /*
            * Checks if the block provided as the argument is the parent of this block object
            */
            bool IsParent (const Block &block) const;

           /*
           * Checks if the block provided as the argument is the child of this block object
           */
            bool IsChild (const Block &block) const;

            int GetTotalTransaction(void) const;

            Transaction ReturnTransaction(int nodeId, int transId);

            bool HasTransaction(Transaction &newTran) const;

            bool HasTransaction(int nodeId, int tranId) const;
            
            void AddTransaction(const Transaction& newTrans);

            void PrintAllTransaction(void);
            
            Block& operator = (const Block &blockSource);     //Assignment Constructor

            friend bool operator == (const Block &block1, const Block &block2);
            //friend std::ostream& operator<<(std::ostream &out, const Block &block);

        protected:
            int         m_blockHeight;                  //the height of the block
            int         m_minerId;                      //the ID of the miner which mined this block
            int         m_nonce;                        //the nonce of the block
            int         m_parentBlockMinerId;           //the ID of the miner which mined the parent of this block
            int         m_blockSizeBytes;               //the size of the block in bytes
            int         m_totalTransactions;
            double      m_timeStamp;                 //the time stamp that the block was created
            double      m_timeReceived;              //the time that the block was received from the node
            Ipv4Address m_receivedFromIpv4;       //the ipv4 of the node which sent the block to the receiving node
            std::vector<Transaction> m_transactions;
    };

    class Blockchain : public Block
    {
        public:
            Blockchain(void);
            virtual ~Blockchain(void);

            int GetTotalBlocks(void) const;

            int GetNoOrphans(void) const;

            int GetBlockchainHeight(void) const;

            /*
             * Check if the block has been included in the blockchain.
             */
            bool HasBlock(const Block &newBlock) const;
            bool HasBlock(int height, int minerId) const;
            /*
             * Retun to block with the specified height and minerID
             * Should be called after HasBlock() to make sure that the block exists.
             */
            Block ReturnBlock(int height, int minerId);
            
            bool IsOrphan(const Block &newBlock) const;
            bool IsOrphan(int height, int minerId) const;
            
            /*
             * Gets a pointer to the block.
             */
            const Block* GetBlockPointer(const Block &newBlock) const;
            
            const std::vector<const Block *> GetChildrenPointers(const Block &block);

            const std::vector<const Block *> GetOrpharnChildrenPointer(const Block &block);

            const Block* GetParent(const Block &block);

            const Block* GetCurrentTopBlock(void) const;

            void AddBlock(const Block& newBlock);

            void AddOrphan(const Block& newBlock);

            void RemoveOrphan (const Block& newBlock);

            //void PrintOrphans(void);

            //void GetBlocksInForks(void);

            //int GetLongestForkSize(void);

            //friend std:: ostream& operator << (std:ostream &out, Blockchain &blockchain);

        protected:
        
            int                             m_totalBlocks;
            std::vector<std::vector<Block>> m_blocks;
            std::vector<Block>              m_orphans;                 
    };

}

#endif