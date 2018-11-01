#ifndef BLOCKCHAIN_NODE_H
#define BLOCKCHAIN_NODE_H

#include <algorithm>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "blockchain.h"
#include "ns3/boolean.h"
#include "../../rapidjson/document.h"
#include "../../rapidjson/writer.h"
#include "../../rapidjson/stringbuffer.h"

namespace ns3 {

    class Address;
    class Socket;
    class Packet;

    class BlockchainNode : public Application
    {
        public:

            static TypeId GetTypeId(void);
            BlockchainNode(void);

            virtual ~BlockchainNode(void);

            Ptr<Socket> GetListeningSocket (void) const;

            std::vector<Ipv4Address> GetPeerAddress (void) const;

            void SetPeersAddresses (const std::vector<Ipv4Address> &peers);

            void SetPeersDownloadSpeeds (const std::map<Ipv4Address, double> &peerDownloadSpeeds);

            void SetPeersUploadSpeeds (const std::map<Ipv4Address, double> &peerUploadSpeeds);

            void SetNodeInternetSpeeds (const nodeInternetSpeed &internetSpeeds);

            void SetNodeStats (nodeStatistics *nodeStats);

            void SetProtocolType (enum ProtocolType protocolType);


        protected:

            virtual void DoDispose (void);           // inherited from application base class.

            virtual void StartApplication (void);
            virtual void StopApplication (void);

            /*
             * Handle a packet received by the application
             * param socket : the receiving socket
             */
            void HandleRead (Ptr<Socket> socket);

            /*
             * Handle an incoming connection
             * param socket : the incoming connection socket
             * param from : the address the connection is from
             */
            void HandleAccept (Ptr<Socket> socket, const Address& from);

            /*
             * handle an connection close
             * param socket : the connected socket
             */
            void HandlePeerClose (Ptr<Socket> socket);

            /*
             * handle an connection error
             * param socket : the connected socket
             */
            void HandlePeerError (Ptr<Socket> socket);

            /*
             * handle an incoming BLOCK message.
             * param blockinfo : the block message info
             * param from : the address the connection is from
             */
            void ReceivedBlockMessage(std::string &blockInfo, Address &from);

            /*
             * Called when a new block non-orphan block is received
             * param newBlock : the newly received block
             */
            virtual void ReceiveBlock(const Block &newBlock);

            /*
             * Send a BLOCK message as a response to a GET_DATA message
             * param packetInfo : the info of the BLOCK message
             * param from : the address the GET_DATA was received from
             */
            void SendBlock(std::string packetInfo, Address &from);

            /*
             * Called for blocks with higher score
             * param newBlock : the new block with higher score
             */
            virtual void ReceivedHigherBlock(const Block &newBlock);

            /*
             * Validates new Blocks by calculating the necessary time interval
             * param newBlock : the new block
             */
            void ValidateBlock(const Block &newBlock);

            /*
             * Adds the new block in to the blockchain, advertises it to the peers and validates any ophan children
             * param newBlock : the new block
             */
            void AfterBlockValidation(const Block &newBlock);
            
            /*
             * Validates any orphan children of the newly received block
             * param newBlock : the new block
             */
            void ValidateOrphanChildren(const Block &newBlock);

            /*
             * Advertises the newly validated block
             * param newBlock : the new block
             */
            void AdvertiseNewBlock (const Block &newBlock);

            /*
             * Advertises the newly validated block when blockTorrent is used
             * param newBlock : the new block
             */
            //void AdvertiseFullBlock (const Block &newBlock);

            void CreateTransaction();

            void ScheduleNextTransaction();

            /*
             * Send a message to a peer
             * param receivedMessage : the type of the received message
             * param responseMessage : the type of the response message
             * param d : rapidjson document containing the info of the outgoing message
             * param outgoingSocket : the socket of the peer
             */
            void SendMessage(enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Ptr<Socket> outgoingSocket);
            
             /*
             * Send a message to a peer
             * param receivedMessage : the type of the received message
             * param responseMessage : the type of the response message
             * param d : rapidjson document containing the info of the outgoing message
             * param outgoingAddress : the Address of the peer
             */
            void SendMessage(enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Address &outgoingAddress);

             /*
             * Send a message to a peer
             * param receivedMessage : the type of the received message
             * param responseMessage : the type of the response message
             * param packet : string containing the info of the outgoing message
             * param outgoingAddress : the Address of the peer
             */
            void SendMessage(enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Address &outgoingAddress);
            
            /*
             * Called when a timout for a block expires
             * param blockhash : the block hash for which the timeout expired
             */
            void InvTimeoutExpired (std::string blockHash);
            
            /*
             * Checks if a block has been received but not been validated yet (if it is included in m_receivedNotValidated)
             * parm blockHash : the block hash
             * return true : if the block has been received but not validated yet, false : otherwise
             */
            bool ReceivedButNotValidated(std::string blockHash);

            /*
             * Removes a block from m_receivedNotValidated
             * param blockHash : the block hash
             */
            void RemoveReceivedButNotvalidated(std::string blockHash);

            /*
             * Checks if the node has recieved only the headers of a particular block(if it is included in m_onlyHeadersReceived)
             * parm blockHash : the block hash
             * return true : if only the block headers have been received, false : otherwise
             */
            bool OnlyHeadersReceived (std::string blockHash);

            /*
             * Remove the first element from m_sendBlockTimes, when a block is sent
             */
            void RemoveSendTime();

            /*
             * Remove the first element from m_sendCompressedBlockTimes, when a compressd-block is sent
             */
            void RemoveCompressedBlockSendTime();

            /*
             * Removes the first element from m_receivedBlockTimes, when block is received
             */
            void RemoveReceiveTime();

            /*
             * Removes the first element from m_receiveCompressedBlockTime, when a compressed-block is received
             */
            void RemoveCompressedBlockReceiveTime();




            Ptr<Socket>     m_socket;                       //Listening socket
            Address         m_local;                        //Local address to bind to
            TypeId          m_tid;                          //Protocol TypeID
            int             m_numberOfPeers;                //Number of node's peers
            double          m_meanBlockReceiveTime;         //The mean time interval between two consecutive blocks (10~15sec)
            double          m_previousBlockReceiveTime;     //The time that the node received the previous block
            double          m_meanBlockPropagationTime;     //The mean time that the node has to wait in order to receive a newly mined block
            double          m_meanBlockSize;                //The mean Block size
            Blockchain      m_blockchain;                   //The node's blockchain
            Time            m_invTimeoutMinutes;
            bool            m_isMiner;                      //True if the node is a miner
            double          m_downloadSpeed;                // Bytes/s
            double          m_uploadSpeed;                  // Bytes/s
            double          m_averageTransacionSize;        //The average transaction size, Needed for compressed blocks
            int             m_transactionIndexSize;         //The transaction index size in bytes. Needed for compressed blocks
            int             m_transactionId;
            EventId         m_nextTransaction;

            std::vector<Transaction>                        m_transaction;
            std::vector<Transaction>                        m_notValidatedTransaction;            
            std::vector<Ipv4Address>                        m_peersAddresses;                   // The address of peers
            std::map<Ipv4Address, double>                   m_peersDownloadSpeeds;              // The peerDownloadSpeeds of channels
            std::map<Ipv4Address, double>                   m_peersUploadSpeeds;                // The peerUploadSpeeds of channels
            std::map<Ipv4Address, Ptr<Socket>>              m_peersSockets;                     // The sockets of peers
            std::map<std::string, std::vector<Address>>     m_queueInv;
            std::map<std::string, EventId>                  m_invTimeouts;
            std::map<Address, std::string>                  m_bufferedData;                     // map holding the buffered data from previous handleRead events
            std::map<std::string, Block>                    m_receivedNotValidated;             // Vevtor holding the received but not yet validated blocks
            std::map<std::string, Block>                    m_onlyHeadersReceived;              // Vevtor holding the blocks that we know byt not received
            nodeStatistics                                  *m_nodeStats;                       // Struct holding the node stats
            std::vector<double>                             m_sendBlockTimes;                   // contains the times of the next sendBlock events
            std::vector<double>                             m_sendCompressedBlockTimes;         // contains the times of the next sendBlock events
            std::vector<double>                             m_receiveBlockTimes;                // contains the times of the next sendBlock events
            std::vector<double>                             m_receiveCompressedBlockTimes;      // contains the times of the next sendBlock events
            enum ProtocolType                               m_protocolType;                     // protocol type

            const int       m_blockchainPort;           //8080
            const int       m_secondsPerMin;            //60
            const int       m_countBytes;               //The size of count variable in message, 4 Bytes
            const int       m_blockchainMessageHeader;  //The size of the Blockchain Message Header, 90 Bytes
            const int       m_inventorySizeBytes;       //The size of inventories in INV messages,36Bytes
            const int       m_getHeaderSizeBytes;       //The size of the GET_HEADERS message, 72bytes
            const int       m_headersSizeBytes;         //81Bytes
            const int       m_blockHeadersSizeBytes;     //81Bytes

            /*
             * Traced Callback: recevied packets, source address. 
             */

            TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

    };

}

#endif