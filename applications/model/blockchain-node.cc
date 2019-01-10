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
#include "blockchain-node.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE("BlockchainNode");
    NS_OBJECT_ENSURE_REGISTERED(BlockchainNode);

    TypeId
    BlockchainNode::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::BlockchainNode")
        .SetParent<Application>()
        .SetGroupName("Applications")
        .AddConstructor<BlockchainNode>()
        .AddAttribute("Local",
                        "The Address on which to Bind the rx socket." ,
                        AddressValue(),
                        MakeAddressAccessor(&BlockchainNode::m_local),
                        MakeAddressChecker())
        .AddAttribute("Protocol",
                        "The type id of the protocol to use for the rx socket",
                        TypeIdValue (UdpSocketFactory::GetTypeId()),
                        MakeTypeIdAccessor (&BlockchainNode::m_tid),
                        MakeTypeIdChecker())
        .AddAttribute("InvTimeoutMinutes",
                        "The timeout of inv messages in minutes",
                        TimeValue(Minutes(2)),
                        MakeTimeAccessor(&BlockchainNode::m_invTimeoutMinutes),
                        MakeTimeChecker())
        .AddTraceSource("Rx",
                        "A packet has been received",
                        MakeTraceSourceAccessor(&BlockchainNode::m_rxTrace),
                        "ns3::Packet::AddressTracedCallback")
        ;
        return tid;
    }

    BlockchainNode::BlockchainNode (void) : m_isMiner(false), m_averageTransacionSize(522.4), m_transactionIndexSize(2), m_blockchainPort(8333), m_secondsPerMin(60), 
                                            m_countBytes(4), m_blockchainMessageHeader(90), m_inventorySizeBytes(36), m_getHeaderSizeBytes(72),
                                            m_headersSizeBytes(81), m_blockHeadersSizeBytes (81)
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;
        m_meanBlockReceiveTime = 0;
        m_previousBlockReceiveTime = 0;
        m_meanBlockPropagationTime = 0;
        m_meanEndorsementTime = 0;
        m_meanOrderingTime = 0;
        m_meanValidationTime = 0;
        m_meanLatency = 0;
        m_meanBlockSize = 0;
        m_numberOfPeers = m_peersAddresses.size();
        m_transactionId = 1;
        m_numberofEndorsers = 10;
        m_totalEndorsement = 0;
        m_totalOrdering = 0;
        m_totalValidation = 0;
        m_totalCreatedTransaction = 0;


    }

    BlockchainNode::~BlockchainNode(void)
    {
        NS_LOG_FUNCTION(this);
    }

    Ptr<Socket>
    BlockchainNode::GetListeningSocket(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_socket;
    }

    std::vector<Ipv4Address>
    BlockchainNode::GetPeerAddress(void) const
    {
        NS_LOG_FUNCTION(this);
        return m_peersAddresses;
    }

    void
    BlockchainNode::SetPeersAddresses (const std::vector<Ipv4Address> &peers)
    {
        NS_LOG_FUNCTION(this);
        m_peersAddresses = peers;
        m_numberOfPeers = m_peersAddresses.size();
    }

    void
    BlockchainNode::SetPeersDownloadSpeeds(const std::map<Ipv4Address, double> &peersDownloadSpeeds)
    {
        NS_LOG_FUNCTION(this);
        m_peersDownloadSpeeds = peersDownloadSpeeds;
    }

    void
    BlockchainNode::SetPeersUploadSpeeds(const std::map<Ipv4Address, double> &peersUploadSpeeds)
    {
        NS_LOG_FUNCTION(this);
        m_peersUploadSpeeds = peersUploadSpeeds;
    }

    void
    BlockchainNode::SetNodeInternetSpeeds(const nodeInternetSpeed &internetSpeeds)
    {
        NS_LOG_FUNCTION(this);

        m_downloadSpeed = internetSpeeds.downloadSpeed*1000000/8;
        m_uploadSpeed = internetSpeeds.uploadSpeed*1000000/8;
    }

    void
    BlockchainNode::SetNodeStats (nodeStatistics *nodeStats)
    {
        NS_LOG_FUNCTION(this);
        m_nodeStats = nodeStats;
    }

    void
    BlockchainNode::SetProtocolType(enum ProtocolType protocolType)
    {
        NS_LOG_FUNCTION(this);
        m_protocolType = protocolType;
    }

    void
    BlockchainNode::SetCommitterType(enum CommitterType cType)
    {
        NS_LOG_FUNCTION(this);
        m_committerType = cType;
    }

    void
    BlockchainNode::SetCreatingTransactionTime(int cTime)
    {
        NS_LOG_FUNCTION(this);
        m_creatingTransactionTime = cTime;
    }

    void
    BlockchainNode::DoDispose(void)
    {
        NS_LOG_FUNCTION(this);
        m_socket = 0;

        Application::DoDispose();
    }

    void
    BlockchainNode::StartApplication()
    {
        NS_LOG_FUNCTION(this);

        srand(time(NULL) + GetNode()->GetId());
        NS_LOG_INFO("Node" << GetNode()->GetId() << ": download speed = " << m_downloadSpeed << "B/s");
        NS_LOG_INFO("Node" << GetNode()->GetId() << ": upload speed = " << m_uploadSpeed << "B/s");
        NS_LOG_INFO("Node" << GetNode()->GetId() << ": m_numberOfPeers = " << m_numberOfPeers);
        NS_LOG_INFO("Node" << GetNode()->GetId() << ": m_protocolType = " << getProtocolType(m_protocolType));

        NS_LOG_INFO("Node" << GetNode()->GetId() << ": My peers are");

        for(auto it = m_peersAddresses.begin(); it != m_peersAddresses.end(); it++)
        {
            NS_LOG_INFO("\t" << *it);
        }

        if(!m_socket)
        {
            m_socket = Socket::CreateSocket(GetNode(), m_tid);
            m_socket->Bind(m_local);
            m_socket->Listen();
            m_socket->ShutdownSend();


            if(addressUtils::IsMulticast(m_local))
            {
                Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
                if(udpSocket)
                {
                    udpSocket->MulticastJoinGroup(0, m_local);
                }
                else
                {
                    NS_FATAL_ERROR("Error : joining multicast on a non-UDP socket");
                }
            }
        }

        m_socket->SetRecvCallback(MakeCallback(&BlockchainNode::HandleRead, this));
        m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
                                    MakeCallback(&BlockchainNode::HandleAccept, this));
        m_socket->SetCloseCallbacks(MakeCallback(&BlockchainNode::HandlePeerClose, this),
                                    MakeCallback(&BlockchainNode::HandlePeerError, this));
        
        NS_LOG_DEBUG("Node" << GetNode()->GetId() << ":Before creating sockets");
        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            m_peersSockets[*i] = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_peersSockets[*i]->Connect(InetSocketAddress(*i, m_blockchainPort));
        }
        NS_LOG_DEBUG("Node" << GetNode()->GetId()<<": After creating sockets");

        m_nodeStats->nodeId = GetNode()->GetId();
        m_nodeStats->meanBlockReceiveTime = 0;
        m_nodeStats->meanBlockPropagationTime = 0;
        m_nodeStats->meanBlockSize = 0;
        m_nodeStats->totalBlocks = 0;
        m_nodeStats->miner = 0;
        m_nodeStats->minerGeneratedBlocks = 0;
        m_nodeStats->minerAverageBlockGenInterval = 0;
        m_nodeStats->minerAverageBlockSize = 0;
        m_nodeStats->hashRate = 0;
        m_nodeStats->invReceivedBytes = 0;
        m_nodeStats->invSentBytes = 0;
        m_nodeStats->getHeadersReceivedBytes = 0;
        m_nodeStats->getHeadersSentBytes = 0;
        m_nodeStats->headersReceivedBytes = 0;
        m_nodeStats->headersSentBytes = 0;
        m_nodeStats->getDataReceivedBytes = 0;
        m_nodeStats->getDataSentBytes = 0;
        m_nodeStats->blockReceivedBytes = 0;
        m_nodeStats->blockSentBytes = 0;
        m_nodeStats->longestFork = 0;
        m_nodeStats->blocksInForks = 0;
        m_nodeStats->connections = m_peersAddresses.size();
        m_nodeStats->blockTimeouts = 0;
        m_nodeStats->nodeGeneratedTransaction = 0;
        m_nodeStats->meanEndorsementTime = 0;
        m_nodeStats->meanOrderingTime = 0;
        m_nodeStats->meanValidationTime = 0;
        m_nodeStats->meanLatency = 0;

        if(m_committerType == COMMITTER)
        {
            m_nodeStats->nodeType = 0;
        }
        else if(m_committerType == ENDORSER)
        {
            m_nodeStats->nodeType = 1;
        }
        else if(m_committerType == CLIENT)
        {
            m_nodeStats->nodeType = 2;
            CreateTransaction();
        }
        else
        {
            m_nodeStats->nodeType = 3;
        }
        
        //ScheduleNextTransaction();
    }

    void
    BlockchainNode::StopApplication()
    {
        NS_LOG_FUNCTION(this);

        for(std::vector<Ipv4Address>::iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            m_peersSockets[*i]->Close();
        }

        if(m_socket)
        {
            m_socket->Close();
            m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        }

        Simulator::Cancel(m_nextTransaction);

        NS_LOG_WARN("\n\nBLOCKCHAIN NODE " <<GetNode()->GetId() << ":");
        //NS_LOG_WARN("Current Top Block is \n"<<*(m_blockchain.GetCurrentTopBlock()));
        //NS_LOG_WARN("Current Blockchain is \n" << m_blockchain);

        NS_LOG_WARN("Mean Block Receive Time = " << m_meanBlockReceiveTime << " or "
                    << static_cast<int>(m_meanBlockReceiveTime)/m_secondsPerMin << "min and "
                    << m_meanBlockReceiveTime - static_cast<int>(m_meanBlockReceiveTime)/m_secondsPerMin * m_secondsPerMin << "s");
        NS_LOG_WARN("Mean Block Propagation Time = " << m_meanBlockPropagationTime << "s");
        NS_LOG_WARN("Mean Block Size = " << m_meanBlockSize << "Bytes");
        NS_LOG_WARN("Total Block = " << m_blockchain.GetTotalBlocks());
        NS_LOG_WARN("Received But Not Validataed size : " << m_receivedNotValidated.size());
        NS_LOG_WARN("m_sendBlockTime size = " <<m_receiveBlockTimes.size());

        m_nodeStats->meanBlockReceiveTime = m_meanBlockReceiveTime;
        m_nodeStats->meanBlockPropagationTime = m_meanBlockPropagationTime;
        m_nodeStats->meanBlockSize = m_meanBlockSize;
        m_nodeStats->totalBlocks = m_blockchain.GetTotalBlocks();
        m_nodeStats->meanEndorsementTime = m_meanEndorsementTime;
        m_nodeStats->meanOrderingTime = m_meanOrderingTime;
        m_nodeStats->meanValidationTime = m_meanValidationTime;
        m_nodeStats->meanLatency = m_meanLatency;
        
    }

    void
    BlockchainNode::HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_INFO(this << socket);
        Ptr<Packet> packet;
        Address from;
        //std::cout<<"Node : "<< GetNode()->GetId()<< " Receive packet\n";

        //double newBlockReceiveTime = Simulator::Now().GetSeconds();

        while((packet = socket->RecvFrom(from)))
        {
            if(packet->GetSize() == 0)
            {
                break;
            }

            if(InetSocketAddress::IsMatchingType(from))
            {
                /*
                 * We may receive more than one packets simultaneously on the socket,
                 * so we have to parse each one of them.
                 */
                std::string delimiter = "#";
                std::string parsedPacket;
                size_t pos = 0;
                char *packetInfo = new char[packet->GetSize() + 1];
                std::ostringstream totalStream;
                
                packet->CopyData(reinterpret_cast<uint8_t *>(packetInfo), packet->GetSize());
                packetInfo[packet->GetSize()] = '\0';

                totalStream << m_bufferedData[from] << packetInfo;
                std::string totalReceivedData(totalStream.str());
                NS_LOG_INFO("Node " << GetNode()->GetId() << "Total Received Data : " << totalReceivedData);

                while((pos = totalReceivedData.find(delimiter)) != std::string::npos)
                {
                    parsedPacket = totalReceivedData.substr(0,pos);
                    NS_LOG_INFO("Node " << GetNode()->GetId() << " Parsed Packet: " << parsedPacket);
                    
                    rapidjson::Document d;
                    d.Parse(parsedPacket.c_str());

                    if(!d.IsObject())
                    {
                        NS_LOG_WARN("The parsed packet is corrupted");
                        totalReceivedData.erase(0, pos + delimiter.length());
                        continue;
                    }

                    rapidjson::StringBuffer buffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                    d.Accept(writer);

                    NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                                << "s Blockchain node " << GetNode()->GetId() << " received"
                                << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                << " port " << InetSocketAddress::ConvertFrom(from).GetPort()
                                << " with info = " << buffer.GetString());

                    switch(d["message"].GetInt())
                    {
                        case INV:
                        {
                            NS_LOG_INFO("INV");

                            if(m_committerType != CLIENT)
                            {
                                unsigned int j;
                                std::vector<std::string>            requestBlocks;
                                std::vector<std::string>::iterator  block_it;

                                m_nodeStats->invReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes; 
                                

                                for(j = 0; j < d["inv"].Size() ; j++)
                                {
                                    std::string invDelimiter = "/";
                                    std::string parsedInv = d["inv"][j].GetString();
                                    size_t invPos = parsedInv.find(invDelimiter);
                                    EventId timeout;

                                    int height = atoi(parsedInv.substr(0, invPos).c_str());
                                    int minerId = atoi(parsedInv.substr(invPos+1, parsedInv.size()).c_str());

                                    if(m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId) || ReceivedButNotValidated(parsedInv))
                                    {
                                        /*std::cout<<"INV : Blockchain node " << GetNode()->GetId()
                                                    << " has already received the block with height = "
                                                    << height << " and minerId = " << minerId << "\n";*/
                                        NS_LOG_INFO("INV : Blockchain node " << GetNode()->GetId()
                                                    << " has already received the block with height = "
                                                    << height << " and minerId = " << minerId);
                                    }
                                    else
                                    {
                                        /*std::cout<<"INV : Blockchain node " << GetNode()->GetId()
                                                    << " does not have the block with height = "
                                                    << height << " and minerId = " << minerId << "\n";*/
                                        NS_LOG_INFO("INV : Blockchain node " << GetNode()->GetId()
                                                    << " does not have the block with height = "
                                                    << height << " and minerId = " << minerId);

                                        /*
                                        * check if we have already requested the block
                                        */
                                        if(m_invTimeouts.find(parsedInv) == m_invTimeouts.end())
                                        {
                                            /*std::cout<<"INV: Blockchain node " << GetNode()->GetId()
                                                        << " has not requested the block yet" << "\n";*/
                                            NS_LOG_INFO("INV: Blockchain node " << GetNode()->GetId()
                                                        << " has not requested the block yet");
                                            requestBlocks.push_back(parsedInv);
                                            timeout = Simulator::Schedule(m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, parsedInv);
                                            m_invTimeouts[parsedInv] = timeout;
                                        }
                                        else
                                        {
                                            NS_LOG_INFO("INV : Blockchain node " << GetNode()->GetId()
                                                        << " has already requested the block");
                                        }

                                        m_queueInv[parsedInv].push_back(from);
                                    }
                                }

                                
                                if(!requestBlocks.empty())
                                {
                                    rapidjson::Value value;
                                    rapidjson::Value array(rapidjson::kArrayType);
                                    d.RemoveMember("inv");

                                    for(block_it = requestBlocks.begin(); block_it < requestBlocks.end(); block_it++)
                                    {
                                        value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
                                        array.PushBack(value, d.GetAllocator());
                                    }

                                    d.AddMember("blocks", array, d.GetAllocator());

                                    SendMessage(INV, GET_HEADERS, d, from );
                                    SendMessage(INV, GET_DATA, d, from );
                                }
                            }
                            
                            break;
                        }
                        case REQUEST_TRANS:
                        {
                            NS_LOG_INFO("REQUEST_TRANS");
                            //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() << " received request_transaction\n";
                            
                            if(m_committerType != CLIENT)
                            {
                                unsigned int j;
                                std::vector<Transaction>            requestTransactions;
                                std::vector<Transaction>::iterator  trans_it;

                                m_nodeStats->getDataReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["transactions"].Size()*m_inventorySizeBytes;

                                for(j = 0; j < d["transactions"].Size(); j++)
                                {
                                    int nodeId = d["transactions"][j]["nodeId"].GetInt();
                                    int transId = d["transactions"][j]["transId"].GetInt();
                                    double timestamp = d["transactions"][j]["timestamp"].GetDouble();
                                    bool transValidation = d["transactions"][j]["validation"].GetBool();
                                    int transExecution = d["transactions"][j]["execution"].GetInt();
                                
                                    if(HasTransaction(nodeId, transId))
                                    {
                                        NS_LOG_INFO("REQUEST_TRANS: Blockchain node " << GetNode()->GetId()
                                                    << " has the transaction nodeID: " << nodeId
                                                    << " and transId = " << transId);
                                        //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() << " alread received request transaction\n";
                                    }
                                    else
                                    {
                                        Transaction newTrans(nodeId, transId, timestamp);
                                        m_transaction.push_back(newTrans);
                                        //m_notValidatedTransaction.push_back(newTrans);

                                        if(m_committerType == ENDORSER)
                                        {
                                            newTrans.SetExecution(GetNode()->GetId());
                                            m_totalEndorsement++;
                                            m_meanEndorsementTime = (m_meanEndorsementTime*static_cast<double>(m_totalEndorsement-1) + (Simulator::Now().GetSeconds() - timestamp))/static_cast<double>(m_totalEndorsement);
                                            ExecuteTransaction(newTrans, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                            //std::cout<<"Type: ENDOESER " <<" Node Id: "<< GetNode()->GetId() << " excute transaction\n";
                                        }
                                        else
                                        {
                                            AdvertiseNewTransaction(newTrans, REQUEST_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                            //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() <<" forwarding request transaction\n";
                                        }
                                    }
                                    
                                }
                            }
                            

                            break;
                        }
                        case REPLY_TRANS:
                        {
                            NS_LOG_INFO("REPLY_TRANS");

                            //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() << " received reply_transaction\n";

                            unsigned int j;
                            std::vector<Transaction>::iterator  trans_it;

                            m_nodeStats->getDataReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["transactions"].Size()*m_inventorySizeBytes;

                            for(j = 0; j < d["transactions"].Size(); j++)
                            {
                                int nodeId = d["transactions"][j]["nodeId"].GetInt();
                                int transId = d["transactions"][j]["transId"].GetInt();
                                double timestamp = d["transactions"][j]["timestamp"].GetDouble();
                                bool transValidation = d["transactions"][j]["validation"].GetBool();
                                int transExecution = d["transactions"][j]["execution"].GetInt();

                            
                                if(HasReplyTransaction(nodeId, transId, transExecution))
                                {
                                    NS_LOG_INFO("REPLY_TRANS: Blockchain node " << GetNode()->GetId()
                                                << " has the reply_transaction nodeID: " << nodeId
                                                << " and transId = " << transId);
                                }
                                else if(!HasReplyTransaction(nodeId, transId, transExecution) && GetNode()->GetId() != nodeId)
                                {
                                    //if node is Committer...

                                    Transaction newTrans(nodeId, transId, timestamp);
                                    newTrans.SetExecution(transExecution);

                                    if(HasTransaction(nodeId, transId))
                                    {
                                        std::vector<Transaction>::iterator it_tran;

                                        for(it_tran = m_transaction.begin(); it_tran < m_transaction.end(); it_tran++)
                                        {
                                            if(it_tran->GetTransNodeId() == nodeId && it_tran->GetTransId()==transId)
                                            {
                                                it_tran->SetExecution(transExecution);
                                                break;
                                            }
                                        }
                                        
                                    }
                                    else
                                    {
                                        m_transaction.push_back(newTrans);
                                    }
                                    
                                    m_replyTransaction.push_back(newTrans);
                                    AdvertiseNewTransaction(newTrans, REPLY_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());

                                }
                                else if(!HasReplyTransaction(nodeId, transId, transExecution) && GetNode()->GetId() == nodeId)
                                {
                                    
                                    //if node is Client...

                                    std::vector<Transaction>::iterator it_tran;

                                    for(it_tran = m_transaction.begin(); it_tran < m_transaction.end(); it_tran++)
                                    {
                                        if(it_tran->GetTransNodeId() == nodeId && it_tran->GetTransId()==transId)
                                        {
                                            it_tran->SetExecution(transExecution);
                                            break;
                                        }
                                    }
                                    
                                    Transaction newTrans(nodeId, transId, timestamp);
                                    newTrans.SetExecution(transExecution);

                                    for(it_tran = m_waitingEndorsers.begin(); it_tran < m_waitingEndorsers.end() ; it_tran++)
                                    {
                                        if(it_tran->GetTransNodeId() == nodeId && it_tran->GetTransId() == transId && it_tran->GetExecution() == transExecution )
                                        {
                                            NS_LOG_INFO("REPLY_TRANS: Blockchain node " << GetNode()->GetId()
                                                        << " already received it to endorsers");
                                            break;
                                        }
                                    }

                                    if(it_tran == m_waitingEndorsers.end())
                                    {
                                        m_waitingEndorsers.push_back(newTrans);
                                    }


                                    if(m_waitingEndorsers.size() == m_numberofEndorsers)
                                    {
                                        AdvertiseNewTransaction(newTrans, MSG_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                        m_waitingEndorsers.clear();
                                    }
                                }
                                else
                                {
                                    //if node is committer which didn't receive oiginal transaction
                                    
                                    Transaction newTrans(nodeId, transId, timestamp);
                                    newTrans.SetExecution(transExecution);
                                    m_transaction.push_back(newTrans);
                                    //m_notValidatedTransaction.push_back(newTrans);
                                    m_replyTransaction.push_back(newTrans);
                                    AdvertiseNewTransaction(newTrans, REPLY_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                }
                                
                            }
                            break;
                        }
                        case MSG_TRANS:
                        {
                            NS_LOG_INFO("MSG_TRANS");

                            //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() << " received MSG_transaction\n";

                            if(m_committerType != CLIENT)
                            {
                                unsigned int j;
                                std::vector<Transaction>::iterator  trans_it;

                                m_nodeStats->getDataReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["transactions"].Size()*m_inventorySizeBytes;

                                for(j = 0; j < d["transactions"].Size(); j++)
                                {
                                    int nodeId = d["transactions"][j]["nodeId"].GetInt();
                                    int transId = d["transactions"][j]["transId"].GetInt();
                                    double timestamp = d["transactions"][j]["timestamp"].GetDouble();
                                    bool transValidation = d["transactions"][j]["validation"].GetBool();
                                    int transExecution = d["transactions"][j]["execution"].GetInt();

                                    if(HasMessageTransaction(nodeId, transId))
                                    {
                                        NS_LOG_INFO("MSG_TRANS: Blockchain node " << GetNode()->GetId()
                                                    << " has transaction which is already executed and not validated nodeID: " << nodeId
                                                    << " and transId = " << transId);
                                    }
                                    else
                                    {
                                        
                                        Transaction newTrans(nodeId, transId, timestamp);
                                        newTrans.SetExecution(transExecution);
                                        m_msgTransaction.push_back(newTrans);
                                        
                                        if(m_isMiner != true)
                                        {
                                            AdvertiseNewTransaction(newTrans, MSG_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                        }
                                        else
                                        {
                                            m_notValidatedTransaction.push_back(newTrans);
                                        }
                                    }


                                }
                            }
                            
                            break;
                        }
                        case RESULT_TRANS:
                        {
                            NS_LOG_INFO("RESULT_TRANS");
                            //std::cout<<"Type: " << m_protocolType <<" Node Id: "<< GetNode()->GetId() << " received result_transaction\n";
                            unsigned int j;
                            std::vector<Transaction>::iterator  trans_it;

                            m_nodeStats->getDataReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["transactions"].Size()*m_inventorySizeBytes;

                            for(j = 0; j < d["transactions"].Size(); j++)
                            {
                                int nodeId = d["transactions"][j]["nodeId"].GetInt();
                                int transId = d["transactions"][j]["transId"].GetInt();
                                double timestamp = d["transactions"][j]["timestamp"].GetDouble();
                                bool transValidation = d["transactions"][j]["validation"].GetBool();
                                int transExecution = d["transactions"][j]["execution"].GetInt();

                                if(HasResultTransaction(nodeId, transId))
                                {
                                    NS_LOG_INFO("RESULT_TRANS: Blockchain node " << GetNode()->GetId()
                                                << " has result_transaction nodeId: " << nodeId
                                                << " and transId = " << transId);
                                }
                                else
                                {

                                    Transaction newTrans(nodeId, transId, timestamp);
                                    m_resultTransaction.push_back(newTrans);

                                    if(GetNode()->GetId() != nodeId)
                                    {
                                        AdvertiseNewTransaction(newTrans, RESULT_TRANS, InetSocketAddress::ConvertFrom(from).GetIpv4());
                                    }
                                    else
                                    {
                                        m_totalCreatedTransaction++;
                                        m_meanLatency = (m_meanLatency*static_cast<double>(m_totalCreatedTransaction-1) + (Simulator::Now().GetSeconds() - timestamp))/static_cast<double>(m_totalCreatedTransaction);
                                        //Measure received time
                                        //std::cout<<"latency : "<<Simulator::Now().GetSeconds() - timestamp <<" , CLIENT node "<<GetNode()->GetId()<< " confirmed that transactions had succeeded\n";
                                    }
                                }
                            }
                            break;
                        }
                        case GET_HEADERS:
                        {
                            
                            NS_LOG_INFO("GET_HEADERS");

                            if(m_committerType != CLIENT)
                            {
                                unsigned int j;
                                std::vector<Block>              requestHeaders;
                                std::vector<Block>::iterator    block_it;

                                m_nodeStats->getHeadersReceivedBytes += m_blockchainMessageHeader + m_getHeaderSizeBytes;

                                for(j =0 ; j < d["blocks"].Size(); j++)
                                {
                                    std::string invDelimiter = "/";
                                    std::string blockHash = d["blocks"][j].GetString();
                                    size_t      invPos = blockHash.find(invDelimiter);

                                    int height = atoi(blockHash.substr(0, invPos).c_str());
                                    int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());

                                    if(m_blockchain.HasBlock(height, minerId) || m_blockchain.IsOrphan(height, minerId))
                                    {
                                        /*std::cout<<"GET_HEADERS: Blockchain node " << GetNode()->GetId()
                                                    << " has the block with height = " << height
                                                    << " and minerId = " << minerId << "\n";*/
                                        
                                        NS_LOG_INFO("GET_HEADERS: Blockchain node " << GetNode()->GetId()
                                                    << " has the block with height = " << height
                                                    << " and minerId = " << minerId);
                                        Block newBlock(m_blockchain.ReturnBlock(height, minerId));
                                        requestHeaders.push_back(newBlock);

                                    } 
                                    else if (ReceivedButNotValidated(blockHash))
                                    {
                                        /*std::cout<<"GET_HEADERS: Blockchain node " << GetNode()->GetId()
                                                    << " has received but not yet validated the block with height = "
                                                    << height << " and minerId = " << minerId << "\n";*/
                                        
                                        NS_LOG_INFO("GET_HEADERS: Blockchain node " << GetNode()->GetId()
                                                    << " has received but not yet validated the block with height = "
                                                    << height << " and minerId = " << minerId);
                                        requestHeaders.push_back(m_receivedNotValidated[blockHash]);
                                    }
                                    else
                                    {
                                        NS_LOG_INFO("GET_HEADERS: Blockchain node " << GetNode()->GetId()
                                                    << " does not have the full block with height = "
                                                    << height << " and minerId = " << minerId);
                                    }
                                }

                                if(!requestHeaders.empty())
                                {
                                    rapidjson::Value value;
                                    rapidjson::Value array(rapidjson::kArrayType);

                                    d.RemoveMember("blocks");

                                    for(block_it = requestHeaders.begin() ; block_it < requestHeaders.end(); block_it++)
                                    {
                                        rapidjson::Value blockInfo(rapidjson::kObjectType);
                                        //NS_LOG_INFO("In requestHeaders " << *block_it);

                                        value = block_it->GetBlockHeight();
                                        blockInfo.AddMember("height", value, d.GetAllocator());
                                        
                                        value = block_it->GetMinerId();
                                        blockInfo.AddMember("minerId", value, d.GetAllocator());

                                        value = block_it->GetNonce();
                                        blockInfo.AddMember("nonce", value, d.GetAllocator());

                                        value = block_it->GetParentBlockMinerId();
                                        blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator());

                                        value = block_it->GetBlockSizeBytes();
                                        blockInfo.AddMember("size", value, d.GetAllocator());

                                        value = block_it->GetTimeStamp();
                                        blockInfo.AddMember("timeStamp", value, d.GetAllocator());

                                        value = block_it->GetTimeReceived();
                                        blockInfo.AddMember("timeReceived", value, d.GetAllocator());

                                        array.PushBack(blockInfo, d.GetAllocator());
                                    
                                    }

                                    d.AddMember("blocks", array, d.GetAllocator());

                                    SendMessage(GET_HEADERS, HEADERS, d, from);

                                }
                            }

                            break;
                        }
                        case HEADERS:
                        {
                            
                            NS_LOG_INFO("HEADERS");

                            if(m_committerType != CLIENT)
                            {
                                std::vector<std::string>        requestHeaders;
                                std::vector<std::string>        requestBlocks;
                                std::vector<std::string>::iterator  block_it;
                                unsigned int j;

                                m_nodeStats->headersReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;

                                for(j = 0; j <d["blocks"].Size(); j++)
                                {
                                    int parentHeight = d["blocks"][j]["height"].GetInt() - 1;
                                    int parentMinerId = d["blocks"][j]["parentBlockMinerId"].GetInt();
                                    int height = d["blocks"][j]["height"].GetInt();
                                    int minerId = d["blocks"][j]["minerId"].GetInt();

                                    EventId         timeout;
                                    std::stringstream   stringStream;
                                    std::string         blockHash;
                                    std::string         parentBlockHash;

                                    stringStream << height << "/" << minerId;
                                    blockHash = stringStream.str();
                                    Block newBlockHeaders(d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["nonce"].GetInt()
                                                        , d["blocks"][j]["parentBlockMinerId"].GetInt(), d["blocks"][j]["size"].GetInt()
                                                        , d["blocks"][j]["timeStamp"].GetDouble(), Simulator::Now().GetSeconds(), InetSocketAddress::ConvertFrom(from).GetIpv4());

                                    m_onlyHeadersReceived[blockHash] = Block(d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["nonce"].GetInt()
                                                                            , d["blocks"][j]["parentBlockMinerId"].GetInt(), d["blocks"][j]["size"].GetInt()
                                                                            , d["blocks"][j]["timeStamp"].GetDouble(), Simulator::Now().GetSeconds(), InetSocketAddress::ConvertFrom(from).GetIpv4());

                                    stringStream.clear();
                                    stringStream.str("");

                                    stringStream << parentHeight << "/" << parentMinerId;
                                    parentBlockHash = stringStream.str();

                                    if (!m_blockchain.HasBlock(parentHeight, parentMinerId) && !m_blockchain.IsOrphan(parentHeight, parentMinerId) && !ReceivedButNotValidated(parentBlockHash))
                                    {
                                        /*std::cout<<"The Block with height  = " << d["blocks"][j]["height"].GetInt()
                                                    << " and minerID = " << d["blocks"][j]["minerId"].GetInt()
                                                    << " is an orphan\n" << "\n";*/
                                        
                                        NS_LOG_INFO("The Block with height  = " << d["blocks"][j]["height"].GetInt()
                                                    << " and minerID = " << d["blocks"][j]["minerId"].GetInt()
                                                    << " is an orphan\n");
                                        
                                        if(m_invTimeouts.find(parentBlockHash) == m_invTimeouts.end())
                                        {
                                            NS_LOG_INFO("HEADERS : Blockchain node " << GetNode()->GetId()
                                                        << " has not requested its parent block yet");
                                            if(!OnlyHeadersReceived(parentBlockHash))
                                            {
                                                requestHeaders.push_back(parentBlockHash.c_str());
                                            }
                                            timeout = Simulator::Schedule(m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, parentBlockHash);
                                            m_invTimeouts[parentBlockHash] = timeout;

                                        }
                                        else
                                        {
                                            NS_LOG_INFO("HEADERS: Blockchain node " << GetNode()->GetId()
                                                        << "has already requested the block");
                                        }

                                        m_queueInv[parentBlockHash].push_back(from);

                                    }
                                    else
                                    {
                                        /*std::cout<<"The Block with height = " << d["blocks"][j]["height"].GetInt()
                                                    << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
                                                    << " is NOT an orphan\n";*/
                                        
                                        NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
                                                    << " and minerId = " << d["blocks"][j]["minerId"].GetInt()
                                                    << " is NOT an orphan\n");
                                    }
                                }

                                if(!requestHeaders.empty())
                                {
                                    rapidjson::Value        value;
                                    rapidjson::Value        array(rapidjson::kArrayType);
                                    Time                    timeout;

                                    d.RemoveMember("blocks");

                                    for(block_it = requestHeaders.begin(); block_it < requestHeaders.end(); block_it++)
                                    {
                                        value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
                                        array.PushBack(value, d.GetAllocator());
                                    }

                                    d.AddMember("blocks", array, d.GetAllocator());

                                    SendMessage(HEADERS, GET_HEADERS, d, from);
                                    SendMessage(HEADERS, GET_DATA, d, from);
                                }

                                if(!requestBlocks.empty())
                                {
                                    rapidjson::Value        value;
                                    rapidjson::Value        array(rapidjson::kArrayType);
                                    Time                    timeout;

                                    d.RemoveMember("blocks");

                                    for(block_it = requestBlocks.begin(); block_it < requestBlocks.end(); block_it++)
                                    {
                                        value.SetString(block_it->c_str(), block_it->size(), d.GetAllocator());
                                        array.PushBack(value, d.GetAllocator());
                                    }

                                    d.AddMember("blocks", array, d.GetAllocator());

                                    SendMessage(HEADERS, GET_DATA, d, from);
                                }
                            }

                            break;
                        }
                        case GET_DATA:
                        {
                            NS_LOG_INFO("GET_DATA");

                            if(m_committerType != CLIENT)
                            {
                                unsigned int j;
                                int totalBlockMessageSize = 0;
                                std::vector<Block>                      requestBlocks;
                                std::vector<Block>::iterator            block_it;
                                std::vector<Transaction>::iterator      trans_it;

                                m_nodeStats->getDataReceivedBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size()*m_inventorySizeBytes;

                                for(j=0; j < d["blocks"].Size(); j++)
                                {
                                    std::string     invDelimiter = "/";
                                    std::string     parsedInv = d["blocks"][j].GetString();
                                    size_t          invPos = parsedInv.find(invDelimiter);

                                    int height = atoi(parsedInv.substr(0, invPos).c_str());
                                    int minerId = atoi(parsedInv.substr(invPos+1, parsedInv.size()).c_str());

                                    if(m_blockchain.HasBlock(height, minerId))
                                    {
                                        NS_LOG_INFO("GET_DATA : Blockchain node " << GetNode()->GetId()
                                                    << " has the block with height = " << height
                                                    << " and minerId = " << minerId);
                                        Block newBlock(m_blockchain.ReturnBlock(height, minerId));
                                        requestBlocks.push_back(newBlock);
                                    }
                                    else
                                    {
                                        NS_LOG_INFO("GET_DATA : Blockchain node " << GetNode()->GetId()
                                                    << " does not have the block with height = " << height
                                                    << " and minerId = " << minerId);
                                    }

                                }

                                if(!requestBlocks.empty())
                                {
                                    rapidjson::Value value;
                                    rapidjson::Value array(rapidjson::kArrayType);
                                    rapidjson::Value tranArray(rapidjson::kArrayType);
                                    std::vector<Transaction> requestTransactions;

                                    d.RemoveMember("blocks");

                                    for(block_it = requestBlocks.begin() ; block_it < requestBlocks.end(); block_it++)
                                    {
                                        //block_it->PrintAllTransaction();
                                        requestTransactions = block_it->GetTransactions();
                                        rapidjson::Value blockInfo(rapidjson::kObjectType);
                                        
                                        value = block_it->GetBlockHeight();
                                        blockInfo.AddMember("height", value, d.GetAllocator());
                                        
                                        value = block_it->GetMinerId();
                                        blockInfo.AddMember("minerId", value, d.GetAllocator());

                                        value = block_it->GetNonce();
                                        blockInfo.AddMember("nonce", value, d.GetAllocator());

                                        value = block_it->GetParentBlockMinerId();
                                        blockInfo.AddMember("parentBlockMinerId", value, d.GetAllocator());

                                        value = block_it->GetBlockSizeBytes();
                                        blockInfo.AddMember("size", value, d.GetAllocator());

                                        value = block_it->GetTimeStamp();
                                        blockInfo.AddMember("timeStamp", value, d.GetAllocator());

                                        value = block_it->GetTimeReceived();
                                        blockInfo.AddMember("timeReceived", value, d.GetAllocator());

                                        for(trans_it = requestTransactions.begin(); trans_it < requestTransactions.end(); trans_it++)
                                        {
                                            //std::cout<<"node " << GetNode()->GetId()<<" add transaction\n";
                                            rapidjson::Value transInfo(rapidjson::kObjectType);
                                            
                                            value = trans_it->GetTransNodeId();
                                            transInfo.AddMember("nodeId", value, d.GetAllocator());

                                            value = trans_it->GetTransId();
                                            transInfo.AddMember("transId", value, d.GetAllocator());

                                            value = trans_it->GetTransTimeStamp();
                                            transInfo.AddMember("timestamp", value, d.GetAllocator());

                                            tranArray.PushBack(transInfo, d.GetAllocator());

                                        }
                                        blockInfo.AddMember("transactions", tranArray, d.GetAllocator());

                                        array.PushBack(blockInfo, d.GetAllocator());
                                    }

                                    d.AddMember("blocks", array, d.GetAllocator());

                                    double sendTime = totalBlockMessageSize/m_uploadSpeed;
                                    double eventTime;

                                    if(m_sendBlockTimes.size() == 0 || Simulator::Now().GetSeconds() > m_sendBlockTimes.back())
                                    {
                                        eventTime = 0;
                                    }
                                    else
                                    {
                                        eventTime = m_sendBlockTimes.back() - Simulator::Now().GetSeconds();
                                    }

                                    m_sendBlockTimes.push_back(Simulator::Now().GetSeconds()+eventTime + sendTime);

                                    NS_LOG_INFO("Node " << GetNode()->GetId() << " will start sending the block to "
                                                << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                                << " at " << Simulator::Now().GetSeconds() + eventTime << "\n");
                                    
                                    rapidjson::StringBuffer packetInfo;
                                    rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
                                    d.Accept(writer);
                                    std::string packet = packetInfo.GetString();
                                    NS_LOG_INFO("DEBUG: " << packetInfo.GetString());

                                    Simulator::Schedule (Seconds(eventTime), &BlockchainNode::SendBlock, this, packet, from);
                                    Simulator::Schedule (Seconds(eventTime + sendTime), &BlockchainNode::RemoveSendTime, this);
                                }
                            }

                            break;
                        }            
                        case BLOCK:
                        {
                            NS_LOG_INFO("BLOCK");

                            if(m_committerType != CLIENT)
                            {
                                int blockMessageSize = 0;
                                double receiveTime;
                                double eventTime = 0;
                                double minSpeed = std::min(m_downloadSpeed, m_peersUploadSpeeds[InetSocketAddress::ConvertFrom(from).GetIpv4()]*1000000/8);

                                blockMessageSize += m_blockchainMessageHeader;
                                

                                for(unsigned int j = 0; j < d["blocks"].Size(); j++)
                                {
                                    blockMessageSize += d["blocks"][j]["size"].GetInt();
                                }

                        
                                m_nodeStats->blockReceivedBytes += blockMessageSize;

                                rapidjson::StringBuffer blockInfo;
                                rapidjson::Writer<rapidjson::StringBuffer> blockWriter(blockInfo);
                                d.Accept(blockWriter);
                                /*
                                if(GetNode()->GetId() == 10)
                                {
                                    std::cout<<"BLOCK: At time " << Simulator::Now().GetSeconds()
                                            << " Node " << GetNode()->GetId()
                                            << " received a block message " << blockInfo.GetString() << "\n";
                                }
                                */
                                
                                NS_LOG_INFO("BLOCK: At time " << Simulator::Now().GetSeconds()
                                            << " Node " << GetNode()->GetId()
                                            << " received a block message " << blockInfo.GetString());
                                NS_LOG_INFO(m_downloadSpeed << " " 
                                            << m_peersUploadSpeeds[InetSocketAddress::ConvertFrom(from). GetIpv4()] * 1000000/8 << " " << minSpeed);

                                std::string help = blockInfo.GetString();

                                if(m_receiveBlockTimes.size() == 0 || Simulator::Now().GetSeconds() > m_receiveBlockTimes.back())
                                {
                                    receiveTime = blockMessageSize / m_downloadSpeed;
                                    eventTime = blockMessageSize / minSpeed;
                                }
                                else
                                {
                                    receiveTime = blockMessageSize / m_downloadSpeed + m_receiveBlockTimes.back() - Simulator::Now().GetSeconds();
                                    eventTime = blockMessageSize / minSpeed + m_receiveBlockTimes.back() - Simulator::Now().GetSeconds();
                                }

                                m_receiveBlockTimes.push_back(Simulator::Now().GetSeconds()+receiveTime);

                                Simulator::Schedule(Seconds(eventTime), &BlockchainNode::ReceivedBlockMessage, this, help, from);
                                Simulator::Schedule(Seconds(receiveTime), &BlockchainNode::RemoveReceiveTime, this);
                                NS_LOG_INFO("BLOCK: Node " << GetNode()->GetId() << " will receive the full block message at "
                                            << Simulator::Now().GetSeconds() + eventTime);
                            }
                            

                            break;
                        }
                        default:
                        {
                            NS_LOG_INFO("Default");
                            break;
                        }
                        
                    }

                    totalReceivedData.erase(0, pos + delimiter.length());

                }

                m_bufferedData[from] = totalReceivedData;
                delete[] packetInfo;

            }
            else if(InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("At time " << Simulator::Now().GetSeconds()
                            << " s blockchain node " << GetNode()->GetId() << " received"
                            << packet->GetSize() << " bytes from"
                            << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                            << " port" << Inet6SocketAddress::ConvertFrom(from).GetPort());
            }
            m_rxTrace(packet, from);
        }
        
    }

    void
    BlockchainNode::HandleAccept(Ptr<Socket> socket, const Address& from)
    {
        NS_LOG_FUNCTION(this);
        socket->SetRecvCallback (MakeCallback(&BlockchainNode::HandleRead, this));
    }

    void
    BlockchainNode::HandlePeerClose(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this);
    }

    void
    BlockchainNode::HandlePeerError(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this);
    }

    void
    BlockchainNode::ReceivedBlockMessage(std::string &blockInfo, Address &from)
    {
        NS_LOG_FUNCTION(this);

        rapidjson::Document d;
        d.Parse(blockInfo.c_str());

        NS_LOG_INFO("ReceivedBlockMessage : At time : " << Simulator::Now().GetSeconds()
                    << " Node " << GetNode()->GetId() << " received a block message " << blockInfo);
        for(unsigned int  j = 0 ; j < d["blocks"].Size(); j++)
        {
            int parentHeight = d["blocks"][j]["height"].GetInt() - 1;
            int parentMinerId = d["blocks"][j]["parentBlockMinerId"].GetInt();
            int height = d["blocks"][j]["height"].GetInt();
            int minerId = d["blocks"][j]["minerId"].GetInt();

            EventId             timeout;
            std::ostringstream  stringStream;
            std::string         blockHash;
            std::string         parentBlockHash;

            stringStream << height << "/" << minerId;
            blockHash = stringStream.str();

            if(m_onlyHeadersReceived.find(blockHash) != m_onlyHeadersReceived.end())
            {
                m_onlyHeadersReceived.erase(blockHash);
            }

            stringStream.clear();
            stringStream.str("");

            stringStream << parentHeight << "/" << parentMinerId;
            parentBlockHash = stringStream.str();

            if(!m_blockchain.HasBlock(parentHeight, parentMinerId) && !m_blockchain.IsOrphan(parentHeight, parentMinerId)
                && !ReceivedButNotValidated(parentBlockHash) && !OnlyHeadersReceived(parentBlockHash))
            {
                NS_LOG_INFO("The Block with height = " << d["blocks"][j]["height"].GetInt()
                            << " and minerID = " << d["blocks"][j]["minerId"].GetInt()
                            << " is an orphan, so it will be discarded\n");
                m_queueInv.erase(blockHash);
                Simulator::Cancel(m_invTimeouts[blockHash]);
                m_invTimeouts.erase(blockHash);
            }
            else
            {
                std::vector<Transaction> newTransactions;
                Block newBlock(d["blocks"][j]["height"].GetInt(), d["blocks"][j]["minerId"].GetInt(), d["blocks"][j]["nonce"].GetInt()
                                , d["blocks"][j]["parentBlockMinerId"].GetInt(), d["blocks"][j]["size"].GetInt()
                                , d["blocks"][j]["timeStamp"].GetDouble(), Simulator::Now().GetSeconds(), InetSocketAddress::ConvertFrom(from).GetIpv4());
                
                for(unsigned int i = 0 ; i < d["blocks"][j]["transactions"].Size(); i++)
                {
                    int transNodeId = d["blocks"][j]["transactions"][i]["nodeId"].GetInt();
                    int transId = d["blocks"][j]["transactions"][i]["transId"].GetInt();
                    double timeStamp = d["blocks"][j]["transactions"][i]["timestamp"].GetDouble();
                    Transaction newTrans(transNodeId, transId, timeStamp);
                    newTransactions.push_back(newTrans);
                    //std::cout<<"Node " << GetNode()->GetId() << " confirmed transaction nodeid: " << transNodeId << " transId: " <<  transId << "\n";
                }
                newBlock.SetTransactions(newTransactions);
                ReceiveBlock(newBlock);
            }
        }
    }

    void
    BlockchainNode::ReceiveBlock(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("ReceiveBlock: At time " << Simulator::Now().GetSeconds()
                    << "s blockchain node " << GetNode()->GetId() << " received");
        std::ostringstream  stringStream;
        std::string         blockHash = stringStream.str();

        stringStream << newBlock.GetBlockHeight() << "/" << newBlock.GetMinerId();
        blockHash = stringStream.str();

        if(m_blockchain.HasBlock(newBlock) || m_blockchain.IsOrphan(newBlock) || ReceivedButNotValidated(blockHash))
        {
            NS_LOG_INFO("ReceiveBlock: Blockchain node " << GetNode()->GetId()
                        << " has already added this block in the m_blockchain");

            if (m_invTimeouts.find(blockHash) != m_invTimeouts.end())
            {
                m_queueInv.erase(blockHash);
                Simulator::Cancel(m_invTimeouts[blockHash]);
                m_invTimeouts.erase(blockHash);
            }
        }
        else
        {
            NS_LOG_INFO("ReceiveBlock: Blockchain node " << GetNode()->GetId()
                        << " has not added this block in the m_blockchain");
            
            m_receivedNotValidated[blockHash] = newBlock;

            if (m_invTimeouts.find(blockHash) != m_invTimeouts.end())
            {
                m_queueInv.erase(blockHash);
                Simulator::Cancel(m_invTimeouts[blockHash]);
                m_invTimeouts.erase(blockHash);
            }

            ValidateBlock(newBlock);
        }

    }

    void
    BlockchainNode::SendBlock(std::string packetInfo, Address &from)
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_INFO("SendBlock: At time "<<  Simulator::Now().GetSeconds()
                    << "s blockchain node " << GetNode()->GetId() << " sent "
                    << packetInfo << " to " << InetSocketAddress::ConvertFrom(from).GetIpv4());

        SendMessage(GET_DATA, BLOCK,packetInfo, from);
    }

    void
    BlockchainNode::ReceivedHigherBlock(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("ReceivedHigherBlock: blockchain node : " <<GetNode()->GetId() 
                    << " added a new block in the m_blockchain with higher height");
    }

    void
    BlockchainNode::ValidateBlock(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);

        const Block *parent = m_blockchain.GetParent(newBlock);

        if(parent == nullptr)
        {
            NS_LOG_INFO("ValidateBlock : Block is an orphan");
            m_blockchain.AddOrphan(newBlock);
            
        }
        else
        {
            const int averageBlockSizeBytes = 238263;   // we should modify it
            const double averageValidationTimeSeconds = 0.174;
            double validationTime = averageValidationTimeSeconds * newBlock.GetBlockSizeBytes() / averageBlockSizeBytes;
            ValidateTransaction(newBlock);

            //std::cout<<"validationTime : " << validationTime << "\n";
            Simulator::Schedule (Seconds(validationTime), &BlockchainNode::AfterBlockValidation, this, newBlock);
            NS_LOG_INFO("ValidateBlock : the block will be validated in " << validationTime << "s");
        }

    }

    void
    BlockchainNode::ValidateTransaction(const Block &newBlock)
    {
        std::vector<Transaction>                requestTransactions;
        std::vector<Transaction>::iterator      trans_it;
        std::vector<Transaction>::iterator      notValTrans_it;
        requestTransactions = newBlock.GetTransactions();

        for(trans_it = requestTransactions.begin(); trans_it < requestTransactions.end(); trans_it++)
        {
            /*
            std::cout<<"Node "<<GetNode()->GetId() << " is validating transaction nodeId : " 
                    << trans_it->GetTransNodeId() << " transId: " << trans_it->GetTransId() << "\n";
            */
            for(notValTrans_it = m_transaction.begin(); notValTrans_it < m_transaction.end(); notValTrans_it++)
            {
                if(*notValTrans_it == *trans_it)
                {
                    if(notValTrans_it->IsValidated() != true)
                    {
                        notValTrans_it->SetValidation();
                        m_totalValidation++;
                        m_meanValidationTime = (m_meanValidationTime*static_cast<double>(m_totalValidation-1) + (Simulator::Now().GetSeconds() - notValTrans_it->GetTransTimeStamp()))/static_cast<double>(m_totalValidation);
                        //Send to Client node
                        NotifyTransaction(*notValTrans_it);
                    }
                    break;
                }
            }

            if(notValTrans_it == m_transaction.end())
            {
                trans_it->SetValidation();
                m_transaction.push_back(*trans_it);

                //m_totalValidation++;
                //m_meanValidationTime = (m_meanValidationTime*static_cast<double>(m_totalValidation-1) + (Simulator::Now().GetSeconds() - notValTrans_it->GetTransTimeStamp()))/static_cast<double>(m_totalValidation);
            }

            
        }
    }

    void
    BlockchainNode::AfterBlockValidation(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);

        int height = newBlock.GetBlockHeight();
        int minerId = newBlock.GetMinerId();
        std::ostringstream  stringStream;
        std::string         blockHash = stringStream.str();

        stringStream << height << "/" << minerId;
        blockHash = stringStream.str();

        RemoveReceivedButNotvalidated(blockHash);

        NS_LOG_INFO("AfterBlockValidation : at time " << Simulator::Now().GetSeconds()
                    << "s blockchain node " << GetNode()->GetId()
                    << " validated block ");
        if(newBlock.GetBlockHeight() > m_blockchain.GetBlockchainHeight())
        {
            ReceivedHigherBlock(newBlock);
        }

        if(m_blockchain.IsOrphan(newBlock))
        {
            NS_LOG_INFO("AfterBlockValidation: Block was orphan");
             m_blockchain.RemoveOrphan(newBlock);
        }

        m_meanBlockReceiveTime = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockReceiveTime
                                + (newBlock.GetTimeReceived() - m_previousBlockReceiveTime)/(m_blockchain.GetTotalBlocks());
        //std::cout << "m_meanBlockReceiveTime : "<<m_meanBlockReceiveTime <<"\n";
        m_previousBlockReceiveTime = newBlock.GetTimeReceived();
        
        m_meanBlockPropagationTime =  (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks())*m_meanBlockPropagationTime
                                + (newBlock.GetTimeReceived() -newBlock.GetTimeStamp())/(m_blockchain.GetTotalBlocks());
        //std::cout << " m_meanBlockPropagationTime : "<< m_meanBlockPropagationTime <<"\n";
        m_meanBlockSize = (m_blockchain.GetTotalBlocks() - 1)/static_cast<double>(m_blockchain.GetTotalBlocks()) * m_meanBlockSize
                            + (newBlock.GetBlockSizeBytes())/static_cast<double>(m_blockchain.GetTotalBlocks());
        
        m_blockchain.AddBlock(newBlock);
        AdvertiseNewBlock(newBlock);
        ValidateOrphanChildren(newBlock);
        

    }

    void
    BlockchainNode::ValidateOrphanChildren(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);

        std::vector<const Block *> children = m_blockchain.GetOrpharnChildrenPointer(newBlock);

        if(children.size() == 0)
        {
            NS_LOG_INFO("ValidateOrphanChildren : Block has no orpharn children\n");
        }
        else
        {
            NS_LOG_INFO("ValidateOrphanChildren : Block has orpharn children:");
            std::vector<const Block *>::iterator block_it;
            for(block_it = children.begin(); block_it < children.end(); block_it++)
            {
                ValidateBlock(**block_it);
            }
        }
    }

    void
    BlockchainNode::AdvertiseNewBlock(const Block &newBlock)
    {
        NS_LOG_FUNCTION(this);
        
        rapidjson::Document d;
        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        std::ostringstream stringStream;
        std::string blockHash = stringStream.str();
        d.SetObject();

        value.SetString("blocks");
        d.AddMember("type", value, d.GetAllocator());

        if(m_protocolType == STANDARD_PROTOCOL)
        {
            value = INV;
            d.AddMember("message", value, d.GetAllocator());

            stringStream << newBlock.GetBlockHeight() << "/" << newBlock.GetMinerId();
            blockHash = stringStream.str();
            value.SetString(blockHash.c_str(), blockHash.size(), d.GetAllocator());
            array.PushBack(value, d.GetAllocator());
            d.AddMember("inv", array, d.GetAllocator());
        }

        rapidjson::StringBuffer packetInfo;
        rapidjson::Writer<rapidjson::StringBuffer> writer(packetInfo);
        d.Accept(writer);

        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin() ; i != m_peersAddresses.end(); ++i)
        {
            if(*i != newBlock.GetReceivedFromIpv4())
            {
                const uint8_t delimiter[] = "#";
                //std::cout<<"node : " <<GetNode()->GetId()<< " Advertise new block\n";
                m_peersSockets[*i]->Send(reinterpret_cast<const uint8_t*>(packetInfo.GetString()), packetInfo.GetSize(), 0);
                m_peersSockets[*i]->Send(delimiter, 1, 0);

                if(m_protocolType == STANDARD_PROTOCOL)
                {
                    m_nodeStats->invSentBytes += m_blockchainMessageHeader + m_countBytes + d["inv"].Size()*m_inventorySizeBytes;
                }
                NS_LOG_INFO("AdvertiseNewBlock: At time " << Simulator::Now().GetSeconds()
                            << "s blockchain node " << GetNode()->GetId() << " advertised a new block to " << *i);

            }
        }
        /*
        if( GetNode()->GetId() == 5){
            std::cout<< "AdvertiseNeBlock: At time " << Simulator::Now().GetSeconds()
                            << "s blockchain node " << GetNode()->GetId() << " advertised a new block(height) :" 
                            << newBlock.GetBlockHeight() << " minerId : " << newBlock.GetMinerId() <<"\n";
        }
        */
        
    }

    void
    BlockchainNode::AdvertiseNewTransaction(const Transaction &newTrans, enum Messages megType, Ipv4Address receivedFromIpv4)
    {
        NS_LOG_FUNCTION(this);

        rapidjson::Document transD;

        int nodeId = newTrans.GetTransNodeId();
        int transId = newTrans.GetTransId();
        double tranTimestamp = newTrans.GetTransTimeStamp();

        transD.SetObject();

        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        rapidjson::Value transInfo(rapidjson::kObjectType);

        value.SetString("transaction");
        transD.AddMember("type", value, transD.GetAllocator());

        value = megType;
        transD.AddMember("message", value, transD.GetAllocator());

        value = newTrans.GetTransNodeId();
        transInfo.AddMember("nodeId", value, transD.GetAllocator());

        value = newTrans.GetTransId();
        transInfo.AddMember("transId", value, transD.GetAllocator());

        value = newTrans.GetTransTimeStamp();
        transInfo.AddMember("timestamp", value, transD.GetAllocator());

        value = newTrans.IsValidated();
        transInfo.AddMember("validation", value, transD.GetAllocator());

        value = newTrans.GetExecution();
        transInfo.AddMember("execution", value, transD.GetAllocator());

        array.PushBack(transInfo, transD.GetAllocator());
        transD.AddMember("transactions", array, transD.GetAllocator());

        rapidjson::StringBuffer transactionInfo;
        rapidjson::Writer<rapidjson::StringBuffer> tranWriter(transactionInfo);
        transD.Accept(tranWriter);

        //std::cout<<"Type : " << m_committerType << " nodeId : " << GetNode()->GetId() <<" broadcaste " << megType <<"\n";


        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            if(*i != receivedFromIpv4)
            {
                const uint8_t delimiter[] = "#";

                m_peersSockets[*i]->Send(reinterpret_cast<const uint8_t*>(transactionInfo.GetString()), transactionInfo.GetSize(), 0);
                m_peersSockets[*i]->Send(delimiter, 1, 0);
            }
        
        }

    }

    bool
    BlockchainNode::HasTransaction(int nodeId, int transId)
    {
        for(auto const &transaction: m_transaction)
        {
            if(transaction.GetTransNodeId() == nodeId && transaction.GetTransId() == transId)
            {
                return true;
            }
        }
        return false;
    }

    bool
    BlockchainNode::HasReplyTransaction(int nodeId, int transId, int transExecution)
    {
        for(auto const &transaction: m_replyTransaction)
        {
            if(transaction.GetTransNodeId() == nodeId && transaction.GetTransId() == transId && transaction.GetExecution() == transExecution)
            {
                return true;
            }
        }
        return false;
    }

    bool
    BlockchainNode::HasMessageTransaction(int nodeId, int transId)
    {
        for(auto const &transaction: m_msgTransaction)
        {
            if(transaction.GetTransNodeId() == nodeId && transaction.GetTransId() == transId)
            {
                return true;
            }
        }
        return false;
    }

    bool
    BlockchainNode::HasResultTransaction(int nodeId, int transId)
    {
        for(auto const &transaction: m_resultTransaction)
        {
            if(transaction.GetTransNodeId() == nodeId && transaction.GetTransId() == transId)
            {
                return true;
            }
        }
        return false;
    }

    bool
    BlockchainNode::HasTransactionAndValidated(int nodeId, int transId)
    {
        for(auto const &transaction: m_transaction)
        {
            if(transaction.GetTransNodeId() == nodeId && transaction.GetTransId() == transId && transaction.IsValidated() == true)
            {
                return true;
            }
        }
        return false;
    }

    void
    BlockchainNode::CreateTransaction()
    {
        NS_LOG_FUNCTION(this);

        rapidjson::Document transD;

        int nodeId = GetNode()->GetId();
        int transId = m_transactionId;
        double tranTimestamp = Simulator::Now().GetSeconds();
        transD.SetObject();

        Transaction newTrans(nodeId, transId, tranTimestamp);

        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        rapidjson::Value transInfo(rapidjson::kObjectType);

        value.SetString("transaction");
        transD.AddMember("type", value, transD.GetAllocator());

        value = REQUEST_TRANS;
        transD.AddMember("message", value, transD.GetAllocator());

        value = newTrans.GetTransNodeId();
        transInfo.AddMember("nodeId", value, transD.GetAllocator());

        value = newTrans.GetTransId();
        transInfo.AddMember("transId", value, transD.GetAllocator());

        value = newTrans.GetTransTimeStamp();
        transInfo.AddMember("timestamp", value, transD.GetAllocator());

        value = newTrans.IsValidated();
        transInfo.AddMember("validation", value, transD.GetAllocator());

        value = newTrans.GetExecution();
        transInfo.AddMember("execution", value, transD.GetAllocator());

        array.PushBack(transInfo, transD.GetAllocator());
        transD.AddMember("transactions", array, transD.GetAllocator());

        m_transaction.push_back(newTrans);
        //m_notValidatedTransaction.push_back(newTrans);

        rapidjson::StringBuffer transactionInfo;
        rapidjson::Writer<rapidjson::StringBuffer> tranWriter(transactionInfo);
        transD.Accept(tranWriter);

        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            const uint8_t delimiter[] = "#";

            m_peersSockets[*i]->Send(reinterpret_cast<const uint8_t*>(transactionInfo.GetString()), transactionInfo.GetSize(), 0);
            m_peersSockets[*i]->Send(delimiter, 1, 0);
        
        }
        //std::cout<< "time : "<<Simulator::Now().GetSeconds() << ", Node type: "<< m_committerType <<" - NodeId: " <<GetNode()->GetId() << " created and sent transaction\n";
        m_transactionId++;

        ScheduleNextTransaction();

    }

    void
    BlockchainNode::ScheduleNextTransaction()
    {
        NS_LOG_FUNCTION(this);
        double tTime = rand()%m_creatingTransactionTime+1;
        //std::cout<<"crateing time = " << tTime << "\n";
        m_nextTransaction = Simulator::Schedule(Seconds(tTime), &BlockchainNode::CreateTransaction, this);

    }

    void
    BlockchainNode::ExecuteTransaction(const Transaction &newTrans, Ipv4Address receivedFromIpv4)
    {
        NS_LOG_FUNCTION(this);

        rapidjson::Document transD;

        int nodeId = newTrans.GetTransNodeId();
        int transId = newTrans.GetTransId();
        double tranTimestamp = newTrans.GetTransTimeStamp();

        //newTrans.SetExecution();
        transD.SetObject();

        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        rapidjson::Value transInfo(rapidjson::kObjectType);

        value.SetString("transaction");
        transD.AddMember("type", value, transD.GetAllocator());

        value = REPLY_TRANS;
        transD.AddMember("message", value, transD.GetAllocator());

        value = newTrans.GetTransNodeId();
        transInfo.AddMember("nodeId", value, transD.GetAllocator());

        value = newTrans.GetTransId();
        transInfo.AddMember("transId", value, transD.GetAllocator());

        value = newTrans.GetTransTimeStamp();
        transInfo.AddMember("timestamp", value, transD.GetAllocator());

        value = newTrans.IsValidated();
        transInfo.AddMember("validation", value, transD.GetAllocator());
        
        value = newTrans.GetExecution();
        transInfo.AddMember("execution", value, transD.GetAllocator());

        array.PushBack(transInfo, transD.GetAllocator());
        transD.AddMember("transactions", array, transD.GetAllocator());

        rapidjson::StringBuffer transactionInfo;
        rapidjson::Writer<rapidjson::StringBuffer> tranWriter(transactionInfo);
        transD.Accept(tranWriter);

 
        const uint8_t delimiter[] = "#";

        m_peersSockets[receivedFromIpv4]->Send(reinterpret_cast<const uint8_t*>(transactionInfo.GetString()), transactionInfo.GetSize(), 0);
        m_peersSockets[receivedFromIpv4]->Send(delimiter, 1, 0);
    

    }

    void
    BlockchainNode::NotifyTransaction(const Transaction &newTrans)
    {
        NS_LOG_FUNCTION(this);

        rapidjson::Document transD;

        int nodeId = newTrans.GetTransNodeId();
        int transId = newTrans.GetTransId();
        double tranTimestamp = newTrans.GetTransTimeStamp();

        transD.SetObject();

        rapidjson::Value value;
        rapidjson::Value array(rapidjson::kArrayType);
        rapidjson::Value transInfo(rapidjson::kObjectType);

        value.SetString("transaction");
        transD.AddMember("type", value, transD.GetAllocator());

        value = RESULT_TRANS;
        transD.AddMember("message", value, transD.GetAllocator());

        value = newTrans.GetTransNodeId();
        transInfo.AddMember("nodeId", value, transD.GetAllocator());

        value = newTrans.GetTransId();
        transInfo.AddMember("transId", value, transD.GetAllocator());

        value = newTrans.GetTransTimeStamp();
        transInfo.AddMember("timestamp", value, transD.GetAllocator());

        value = newTrans.IsValidated();
        transInfo.AddMember("validation", value, transD.GetAllocator());
        
        value = newTrans.GetExecution();
        transInfo.AddMember("execution", value, transD.GetAllocator());

        array.PushBack(transInfo, transD.GetAllocator());
        transD.AddMember("transactions", array, transD.GetAllocator());

        rapidjson::StringBuffer transactionInfo;
        rapidjson::Writer<rapidjson::StringBuffer> tranWriter(transactionInfo);
        transD.Accept(tranWriter);

        for(std::vector<Ipv4Address>::const_iterator i = m_peersAddresses.begin(); i != m_peersAddresses.end(); ++i)
        {
            
            const uint8_t delimiter[] = "#";

            m_peersSockets[*i]->Send(reinterpret_cast<const uint8_t*>(transactionInfo.GetString()), transactionInfo.GetSize(), 0);
            m_peersSockets[*i]->Send(delimiter, 1, 0);
        
        }

    }



    void
    BlockchainNode::SendMessage(enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Ptr<Socket> outgoingSocket)
    {
        NS_LOG_FUNCTION(this);

        const uint8_t delimiter[] = "#";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        d["message"].SetInt(responseMessage);
        d.Accept(writer);
        NS_LOG_INFO("Node " << GetNode()->GetId() << " got a "
                    << getMessageName(receivedMessage) << " message "
                    << " and sent a " << getMessageName(responseMessage)
                    << " message: " << buffer.GetString() );
        
        outgoingSocket->Send(reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
        outgoingSocket->Send(delimiter, 1, 0);

        switch(d["message"].GetInt())
        {
            case INV:
            {
                m_nodeStats->invSentBytes += m_blockchainMessageHeader + m_countBytes + d["inv"].Size() * m_inventorySizeBytes;
                break;
            }
            case REQUEST_TRANS:
            {
                break;
            }
            case GET_HEADERS:
            {
                m_nodeStats->getHeadersSentBytes += m_blockchainMessageHeader + m_getHeaderSizeBytes;
                break;
            }
            case HEADERS:
            {
                m_nodeStats->headersSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
                break;
            }
            case GET_DATA:
            {
                m_nodeStats->getDataSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size() * m_inventorySizeBytes;
                break;
            }
            case BLOCK:
            {
                for(uint16_t k = 0; k < d["blocks"].Size(); k++)
                {
                    m_nodeStats->blockSentBytes +=  d["blocks"][k]["size"].GetInt();
                }
                m_nodeStats->blockSentBytes += m_blockchainMessageHeader;
                break;
            }
        }

    }

    void
    BlockchainNode::SendMessage(enum Messages receivedMessage, enum Messages responseMessage, rapidjson::Document &d, Address &outgoingAddress)
    {
        NS_LOG_FUNCTION(this);
        
        const uint8_t delimiter[] = "#";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        d["message"].SetInt(responseMessage);
        d.Accept(writer);
        NS_LOG_INFO("Node " << GetNode()->GetId() << " got a "
                    << getMessageName(receivedMessage) << " message "
                    << " and sent a " << getMessageName(responseMessage)
                    << " message: " << buffer.GetString() );
        
        Ipv4Address outgoingIpv4Address = InetSocketAddress::ConvertFrom(outgoingAddress).GetIpv4();
        std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv4Address);

        if(it == m_peersSockets.end())
        {
            m_peersSockets[outgoingIpv4Address] = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_peersSockets[outgoingIpv4Address]->Connect(InetSocketAddress(outgoingIpv4Address, m_blockchainPort));
        }

        m_peersSockets[outgoingIpv4Address]->Send(reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
        m_peersSockets[outgoingIpv4Address]->Send(delimiter, 1, 0);
        
        switch(d["message"].GetInt())
        {
            case INV:
            {
                m_nodeStats->invSentBytes += m_blockchainMessageHeader + m_countBytes + d["inv"].Size() * m_inventorySizeBytes;
                break;
            }
            case REQUEST_TRANS:
            {
                break;
            }
            case GET_HEADERS:
            {
                m_nodeStats->getHeadersSentBytes += m_blockchainMessageHeader + m_getHeaderSizeBytes;
                break;
            }
            case HEADERS:
            {
                m_nodeStats->headersSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
                break;
            }
            case GET_DATA:
            {
                m_nodeStats->getDataSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size() * m_inventorySizeBytes;
                break;
            }
            case BLOCK:
            {
                //std::cout<<"start add block size"<<"\n";
                for(int k = 0; k < d["blocks"].Size(); k++)
                {
                    m_nodeStats->blockSentBytes +=  d["blocks"][k]["size"].GetInt();
                }
                m_nodeStats->blockSentBytes += m_blockchainMessageHeader;
                //std::cout<<"finish add block size"<<"\n";
                break;
            }
        }
    }

    void
    BlockchainNode::SendMessage(enum Messages receivedMessage, enum Messages responseMessage, std::string packet, Address &outgoingAddress)
    {
        NS_LOG_FUNCTION(this);
        
        const uint8_t delimiter[] = "#";
        rapidjson::Document d;

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        d.Parse(packet.c_str());
        d["message"].SetInt(responseMessage);
        d.Accept(writer);
        NS_LOG_INFO("Node " << GetNode()->GetId() << " got a "
                    << getMessageName(receivedMessage) << " message "
                    << " and sent a " << getMessageName(responseMessage)
                    << " message: " << buffer.GetString() );
        
        Ipv4Address outgoingIpv4Address = InetSocketAddress::ConvertFrom(outgoingAddress).GetIpv4();
        std::map<Ipv4Address, Ptr<Socket>>::iterator it = m_peersSockets.find(outgoingIpv4Address);

        if(it == m_peersSockets.end())
        {
            m_peersSockets[outgoingIpv4Address] = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_peersSockets[outgoingIpv4Address]->Connect(InetSocketAddress(outgoingIpv4Address, m_blockchainPort));
        }

        m_peersSockets[outgoingIpv4Address]->Send(reinterpret_cast<const uint8_t*>(buffer.GetString()), buffer.GetSize(), 0);
        m_peersSockets[outgoingIpv4Address]->Send(delimiter, 1, 0);
        
        switch(d["message"].GetInt())
        {
            case INV:
            {
                m_nodeStats->invSentBytes += m_blockchainMessageHeader + m_countBytes + d["inv"].Size() * m_inventorySizeBytes;
                break;
            }
            case REQUEST_TRANS:
            {
                break;
            }
            case GET_HEADERS:
            {
                m_nodeStats->getHeadersSentBytes += m_blockchainMessageHeader + m_getHeaderSizeBytes;
                break;
            }
            case HEADERS:
            {
                m_nodeStats->headersSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size()*m_headersSizeBytes;
                break;
            }
            case GET_DATA:
            {
                m_nodeStats->getDataSentBytes += m_blockchainMessageHeader + m_countBytes + d["blocks"].Size() * m_inventorySizeBytes;
                break;
            }
            case BLOCK:
            {
                for(uint16_t k = 0; k < d["blocks"].Size(); k++)
                {
                    m_nodeStats->blockSentBytes +=  d["blocks"][k]["size"].GetInt();
                }
                m_nodeStats->blockSentBytes += m_blockchainMessageHeader;
                break;
            }
        }

    }

    void
    BlockchainNode::InvTimeoutExpired(std::string blockHash)
    {
        NS_LOG_FUNCTION(this);
        
        std::string     invDelimiter = "/";
        size_t          invPos = blockHash.find(invDelimiter);

        int height = atoi(blockHash.substr(0, invPos).c_str());
        int minerId = atoi(blockHash.substr(invPos+1, blockHash.size()).c_str());

        NS_LOG_INFO("Node " << GetNode()->GetId() << " : At time " << Simulator::Now().GetSeconds()
                    << " the timeour for block " << blockHash << " expired");

        /*
        std::cout<<"Node " << GetNode()->GetId() << " : At time " << Simulator::Now().GetSeconds()
                    << " the timeour for block " << blockHash << " expired\n"; */           

        m_nodeStats->blockTimeouts++;

        m_queueInv[blockHash].erase(m_queueInv[blockHash].begin());
        m_invTimeouts.erase(blockHash);

        if(!m_queueInv[blockHash].empty() && !m_blockchain.HasBlock(height, minerId)
            && !m_blockchain.IsOrphan(height, minerId) && !ReceivedButNotValidated(blockHash))
        {
            rapidjson::Document     d;
            EventId                 timeout;
            rapidjson::Value        value(INV);
            rapidjson::Value        array(rapidjson::kArrayType);

            d.SetObject();
        
            d.AddMember("message", value, d.GetAllocator());
            value.SetString("block");
            d.AddMember("type", value, d.GetAllocator());

            value.SetString(blockHash.c_str(), blockHash.size(), d.GetAllocator());
            array.PushBack(value, d.GetAllocator());
            d.AddMember("blocks", array, d.GetAllocator());

            int index = rand()%m_queueInv[blockHash].size();
            Address temp = m_queueInv[blockHash][0];
            m_queueInv[blockHash][0] = m_queueInv[blockHash][index];
            m_queueInv[blockHash][index] = temp;

            
            SendMessage(INV, GET_HEADERS, d, *(m_queueInv[blockHash].begin()));
            
            SendMessage(INV, GET_DATA, d, *(m_queueInv[blockHash].begin()));

            timeout = Simulator::Schedule(m_invTimeoutMinutes, &BlockchainNode::InvTimeoutExpired, this, blockHash);
            m_invTimeouts[blockHash] = timeout;
        }
        else
        {
            m_queueInv.erase(blockHash);
        }

    }

    bool
    BlockchainNode::ReceivedButNotValidated(std::string blockHash)
    {
        NS_LOG_FUNCTION(this);

        if(m_receivedNotValidated.find(blockHash) != m_receivedNotValidated.end())
            return true;
        else
            return false;
    }

    void
    BlockchainNode::RemoveReceivedButNotvalidated(std::string blockHash)
    {
        NS_LOG_FUNCTION(this);

        if(m_receivedNotValidated.find(blockHash) != m_receivedNotValidated.end())
        {
            m_receivedNotValidated.erase(blockHash);
        }
        else
        {
            NS_LOG_WARN("Block was not found in m_receivedNotValidated");
        }
    }

    bool
    BlockchainNode::OnlyHeadersReceived (std::string blockHash)
    {
        NS_LOG_FUNCTION(this);
        
        if(m_onlyHeadersReceived.find(blockHash) != m_onlyHeadersReceived.end())
            return true;
        else
            return false;
    }

    void
    BlockchainNode::RemoveSendTime()
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_INFO("RemoveSendTime : At time " << Simulator::Now().GetSeconds()
                    << " " << m_sendBlockTimes.front() << " was removed");

        m_sendBlockTimes.erase(m_sendBlockTimes.begin()); 
    }

    void
    BlockchainNode::RemoveCompressedBlockSendTime()
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("RemoveCompressedBlockSendTime: At time " << Simulator::Now().GetSeconds()
                    << " " << m_sendCompressedBlockTimes.front() << " was removed");

        m_sendCompressedBlockTimes.erase(m_sendCompressedBlockTimes.begin());

    }

    void
    BlockchainNode::RemoveReceiveTime()
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_INFO("RemoveReceiveTime: At time " << Simulator::Now().GetSeconds()
                    << " " << m_receiveBlockTimes.front() << " was removed");
        m_receiveBlockTimes.erase(m_receiveBlockTimes.begin());
    }

    void 
    BlockchainNode::RemoveCompressedBlockReceiveTime()
    {
        NS_LOG_FUNCTION(this);

        NS_LOG_INFO("RemoveCompressedBlockReceiveTime: At time " << Simulator::Now().GetSeconds()
                    << " " << m_receiveCompressedBlockTimes.front() << " was removed");
        m_receiveCompressedBlockTimes.erase(m_receiveCompressedBlockTimes.begin());
    }


}