#ifndef BLOCKCHAIN_NODE_HELPER_H
#define BLOCKCHAIN_NODE_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/uinteger.h"
#include "ns3/blockchain.h"

namespace ns3
{

    class BlockchainNodeHelper{
        public:

            BlockchainNodeHelper(std::string protocol,  Address address, std::vector<Ipv4Address> &peers, 
                                std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                nodeInternetSpeed &internetSpeeds, nodeStatistics *stats);
            
            BlockchainNodeHelper(void);

            void commonConstructor(std::string protocol, Address address, std::vector<Ipv4Address> &peers, 
                                std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                nodeInternetSpeed &internetSpeeds, nodeStatistics *stats);
            
            void SetAttribute(std::string name, const AttributeValue &value);

            ApplicationContainer Install (NodeContainer c);
            ApplicationContainer Install (Ptr<Node> node);
            ApplicationContainer Install (std::string nodeName);

            void SetPeersAddresses(std::vector<Ipv4Address> &peersAddress);
            void SetPeersDownloadSpeeds(std::map<Ipv4Address, double> &peersDownloadSpeeds);
            void SetPeersUploadSpeeds(std::map<Ipv4Address, double> &peersUploadSpeeds);
            void SetNodeInternetSpeeds(nodeInternetSpeed &internetSpeeds);
            void SetNodeStats(nodeStatistics *nodeStats);
            void SetProtocolType(enum ProtocolType protocolType);
            void SetCommitterType(enum CommitterType cType);
            void SetCreatingTransactionTime(int cTime);

        protected:

            virtual Ptr<Application> InstallPriv (Ptr<Node> node);

            ObjectFactory                   m_factory;
            std::string                     m_protocol;
            Address                         m_address;
            std::vector<Ipv4Address>        m_peersAddresses;
            std::map<Ipv4Address, double>   m_peersDownloadSpeeds;
            std::map<Ipv4Address, double>   m_peersUploadSpeeds;
            nodeInternetSpeed               m_internetSpeeds;
            nodeStatistics                  *m_nodeStats;
            enum ProtocolType               m_protocolType;
            enum CommitterType              m_committerType;
            int                             m_creatingTransactionTime;          

    };

}

#endif