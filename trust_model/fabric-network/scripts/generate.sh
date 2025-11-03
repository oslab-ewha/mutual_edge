#!/bin/bash

cryptogen generate --config=./config/crypto-config.yaml
configtxgen -profile OneOrgGenesis -outputBlock ./channel-artifacts/genesis.block -channelID system-channel
configtxgen -profile OneOrgChannel -outputCreateChannelTx ./channel-artifacts/channel.tx -channelID mychannel
