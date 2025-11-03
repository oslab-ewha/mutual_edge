package main

import (
    "encoding/json"
    "fmt"

    "github.com/hyperledger/fabric-contract-api-go/contractapi"
)

type MileageContract struct {
    contractapi.Contract
}

type Mileage struct {
    Balance int `json:"balance"`
}

func (c *MileageContract) InitLedger(ctx contractapi.TransactionContextInterface) error {
    initial := Mileage{Balance: 0}
    b, _ := json.Marshal(initial)
    return ctx.GetStub().PutState("A", b)
}

func (c *MileageContract) Consume(ctx contractapi.TransactionContextInterface, enterprise string, amount int) error {
    b, _ := ctx.GetStub().GetState(enterprise)
    var m Mileage
    json.Unmarshal(b, &m)

    m.Balance -= amount
    nb, _ := json.Marshal(m)
    return ctx.GetStub().PutState(enterprise, nb)
}

func (c *MileageContract) Earn(ctx contractapi.TransactionContextInterface, enterprise string, amount int) error {
    b, _ := ctx.GetStub().GetState(enterprise)
    var m Mileage
    json.Unmarshal(b, &m)

    m.Balance += amount
    nb, _ := json.Marshal(m)
    return ctx.GetStub().PutState(enterprise, nb)
}

func (c *MileageContract) Query(ctx contractapi.TransactionContextInterface, enterprise string) (*Mileage, error) {
    b, _ := ctx.GetStub().GetState(enterprise)
    var m Mileage
    json.Unmarshal(b, &m)
    return &m, nil
}

func main() {
    cc, _ := contractapi.NewChaincode(new(MileageContract))
    if err := cc.Start(); err != nil {
        fmt.Printf("Error: %s", err)
    }
}
