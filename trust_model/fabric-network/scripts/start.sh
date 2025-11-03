#!/bin/bash
docker-compose up -d
sleep 3

# Create channel
docker exec cli configtxgen -profile OneOrgChannel -outputCreateChannelTx /etc/hyperledger/fabric/channel.tx -channelID mychannel

# Join channel
docker exec peer0.org1.example.com peer channel create \
  -o orderer.example.com:7050 \
  -c mychannel \
  -f /etc/hyperledger/fabric/channel.tx \
  --outputBlock /etc/hyperledger/fabric/mychannel.block

docker exec peer0.org1.example.com peer channel join \
  -b /etc/hyperledger/fabric/mychannel.block
