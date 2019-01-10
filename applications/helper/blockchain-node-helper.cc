#include "blockchain-node-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"
#include "../model/blockchain-node.h"

namespace ns3 {

    BlockchainNodeHelper::BlockchainNodeHelper(std::string protocol, Address address, std::vector<Ipv4Address> &peers,
                                                std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                                nodeInternetSpeed &internetSpeeds, nodeStatistics *stats)
    {
        m_factory.SetTypeId("ns3::BlockchainNode");
        
        commonConstructor(protocol, address, peers, peersDownloadSpeeds, peersUploadSpeeds, internetSpeeds, stats);
    }

    BlockchainNodeHelper::BlockchainNodeHelper(void)
    {

    }

    void
    BlockchainNodeHelper::commonConstructor(std::string protocol, Address address, std::vector<Ipv4Address> &peers,
                                            std::map<Ipv4Address, double> &peersDownloadSpeeds, std::map<Ipv4Address, double> &peersUploadSpeeds,
                                            nodeInternetSpeed &internetSpeeds, nodeStatistics *stats)
    {
        m_protocol = protocol;
        m_address = address;
        m_peersAddresses = peers;
        m_peersDownloadSpeeds = peersDownloadSpeeds;
        m_peersUploadSpeeds = peersUploadSpeeds;
        m_internetSpeeds = internetSpeeds;
        m_nodeStats = stats;
        m_protocolType = STANDARD_PROTOCOL;
        m_committerType = COMMITTER;

        m_factory.Set("Protocol", StringValue(m_protocol));
        m_factory.Set("Local", AddressValue(m_address));
        

    }

    void
    BlockchainNodeHelper::SetAttribute(std::string name, const AttributeValue &value)
    {
        m_factory.Set(name, value);
    }

    ApplicationContainer
    BlockchainNodeHelper::Install(Ptr<Node> node)
    {
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    BlockchainNodeHelper::Install(std::string nodeName)
    {
        Ptr<Node> node = Names::Find<Node> (nodeName);
        return ApplicationContainer(InstallPriv(node));
    }

    ApplicationContainer
    BlockchainNodeHelper::Install(NodeContainer c)
    {
        ApplicationContainer apps;
        for(NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
        {
            apps.Add (InstallPriv(*i));
        }
        return apps;
    }

    Ptr<Application>
    BlockchainNodeHelper::InstallPriv(Ptr<Node> node)
    {
        Ptr<BlockchainNode> app = m_factory.Create<BlockchainNode> ();
        app->SetPeersAddresses(m_peersAddresses);
        app->SetPeersDownloadSpeeds(m_peersDownloadSpeeds);
        app->SetPeersUploadSpeeds(m_peersUploadSpeeds);
        app->SetNodeInternetSpeeds(m_internetSpeeds);
        app->SetNodeStats(m_nodeStats);
        app->SetProtocolType(m_protocolType);
        app->SetCommitterType(m_committerType);
        app->SetCreatingTransactionTime(m_creatingTransactionTime);

        node->AddApplication(app);

        return app;
    }

    void
    BlockchainNodeHelper::SetPeersAddresses (std::vector<Ipv4Address> &peersAddresses)
    {
        m_peersAddresses = peersAddresses;
    }

    void
    BlockchainNodeHelper::SetPeersDownloadSpeeds(std::map<Ipv4Address, double> &peersDownloadSpeeds)
    {
        m_peersDownloadSpeeds = peersDownloadSpeeds;
    }

    void
    BlockchainNodeHelper::SetPeersUploadSpeeds(std::map<Ipv4Address, double> &peersUploadSpeeds)
    {
        m_peersUploadSpeeds = peersUploadSpeeds;
    }

    void
    BlockchainNodeHelper::SetNodeInternetSpeeds(nodeInternetSpeed &internetSpeeds)
    {
        m_internetSpeeds = internetSpeeds;
    }

    void
    BlockchainNodeHelper::SetNodeStats(nodeStatistics *nodeStats)
    {
        m_nodeStats = nodeStats;
    }

    void
    BlockchainNodeHelper::SetProtocolType(enum ProtocolType protocolType)
    {
        m_protocolType = protocolType;
    }

    void
    BlockchainNodeHelper::SetCommitterType(enum CommitterType cType)
    {
        m_committerType = cType;
    }

    void
    BlockchainNodeHelper::SetCreatingTransactionTime(int cTime)
    {
        m_creatingTransactionTime = cTime;
    }

}