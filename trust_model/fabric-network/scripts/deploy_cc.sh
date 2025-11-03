#!/bin/bash

CHAINCODE_NAME=mileage
CC_SRC_PATH=chaincode
CC_VERSION=1.0

docker exec peer0.org1.example.com peer lifecycle chaincode package ${CHAINCODE_NAME}.tar.gz \
  --path $CC_SRC_PATH --lang golang --label ${CHAINCODE_NAME}_${CC_VERSION}

docker exec peer0.org1.example.com peer lifecycle chaincode install ${CHAINCODE_NAME}.tar.gz

docker exec peer0.org1.example.com peer lifecycle chaincode approveformyorg \
  -o orderer.example.com:7050 \
  --channelID mychannel \
  --name $CHAINCODE_NAME \
  --version $CC_VERSION \
  --sequence 1

docker exec peer0.org1.example.com peer lifecycle chaincode commit \
  -o orderer.example.com:7050 \
  --channelID mychannel \
  --name $CHAINCODE_NAME \
  --version $CC_VERSION \
  --sequence 1
