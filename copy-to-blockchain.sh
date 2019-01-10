# Configure the following values
NS3_FOLDER=~/workspace/ns-allinone-3.25/ns-3.25
PROJECT_FOLDER=~/workspace/2018_Blockchain_Hyperledger_ns3

# Do not change
cp $NS3_FOLDER/src/applications/model/blockchain.h $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/model/blockchain.cc $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/model/blockchain-miner.h $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/model/blockchain-miner.cc $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/model/blockchain-node.h $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/model/blockchain-node.cc $PROJECT_FOLDER/applications/model/
cp $NS3_FOLDER/src/applications/helper/blockchain-miner-helper.h $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/applications/helper/blockchain-miner-helper.cc $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/applications/helper/blockchain-node-helper.h $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/applications/helper/blockchain-node-helper.cc $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/applications/helper/blockchain-topology-helper.h $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/applications/helper/blockchain-topology-helper.cc $PROJECT_FOLDER/applications/helper/
cp $NS3_FOLDER/src/internet/helper/ipv4-address-helper-custom.h $PROJECT_FOLDER/internet/helper/
cp $NS3_FOLDER/src/internet/helper/ipv4-address-helper-custom.cc $PROJECT_FOLDER/internet/helper/
cp $NS3_FOLDER/scratch/blockchain_test3.cc $PROJECT_FOLDER/scratch

