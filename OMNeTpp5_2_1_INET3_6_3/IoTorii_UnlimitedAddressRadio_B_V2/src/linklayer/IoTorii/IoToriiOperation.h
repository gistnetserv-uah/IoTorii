/*
 * Copyright (C) 2018 Elisa Rojas(1), Hedayat Hosseini(2);
 *                    (1) GIST, University of Alcala, Spain.
 *                    (2) CEIT, Amirkabir University of Technology (Tehran Polytechnic), Iran.
 *                    OMNeT++ 5.2.1 & INET 3.6.3
*/

//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef IOTORII_SRC_LINKLAYER_GA3SWITCH_IOTORIIOPERATION_H
#define IOTORII_SRC_LINKLAYER_GA3SWITCH_IOTORIIOPERATION_H

#include "src/linklayer/IoTorii/IHLMACAddressTable.h"
#include "inet/common/INETDefs.h"

#include "inet/linklayer/base/MACProtocolBase.h"
#include "inet/common/lifecycle/ILifecycle.h"
//#include "src/linklayer/simpleidealmac/MACFrameIoTorii_m.h"
#include "inet/linklayer/base/MACFrameBase_m.h"
#include "src/linklayer/IoTorii/SetHLMCFrame_m.h"



namespace iotorii {
using namespace inet;

class StatisticCollector;

class IoToriiOperation : public cSimpleModule, public ILifecycle
{
public:
  /** @brief Gate ids */
  //@{
  int upperLayerInGateId;
  int upperLayerOutGateId;
  int lowerLayerInGateId;
  int lowerLayerOutGateId;
  //@}

  protected:
    StatisticCollector *statisticCollector;
    /** @brief Length of the header*/
    int headerLength;
    int headerLengthPANID;

    int broadcastType;

    //cModule *host;
    MACAddress myMACAddress;

    //jitters
    simtime_t maxJitter;
    cPar *jitterPar = nullptr;


    // Switch parameters
    bool isCoreSwitch;
    int corePrefix;            // main core prefix, set by ned parameter
    simtime_t coreStartTime;
    simtime_t coreInterval;

    //Hello parameters
    simtime_t helloStartTime;
    simtime_t helloInterval; //"Hello" interval time, every helloInterval seconds a node broadcasts Hello messages
    cMessage *HelloTimer;
    /** HeT(Hello Table) **/
    std::vector<MACAddress> neighborList;
    unsigned int maxNeighbors; //maximum number of neighbors. changing this value needs to change HLMACAddress and eGA3Frame structure.
    int maxHLMACs; //maximum number of HLMAC table size.  -1 means "unlimited" size

    IHLMACAddressTable *hlmacTable;

    cMessage *startCoreEvent;

    // Parameters for statistics collection

    long hlmacLenIsLow;
    long hlmacWidthIsLow; //if numNeighbors > maxNeighbors, hlmacWidthIsLow++.
    //long hlmacAffectedByWidthIsLow;
    long numHelloRcvd;                 //Number of Hello messages received
    long numHelloSent;
    int numAllowedNeighbors;  //saves neighbors as long as width of HLMAC address (i.e. 3 for 3-bit width)
    long numNeighbors; //number of neighbors discovered by Hello message
    long numHLMACRcvd;
    long numHLMACAssigned;
    long numHLMACLoopable;  //or num discarded HLMAC
    long numHLMACSent;
    long numDiscardedNoHLMAC; //if node has not HLMAC address, by dropping each packet, this variable is incremented.

    long numReceivedLowerPacket;
    long numReceivedUpperPacket;
    long numDiscardedFrames;
    long numRoutedUnicastFrames;
    long numRoutedBroadcastFrames;
    long numDiscardedUnicastFrames;
    long numDiscardedBroadcastFrames;

    bool isOperational;    // for lifecycle

  public:

    IoToriiOperation();

    ~IoToriiOperation();

    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;

  protected:

    virtual void initialize(int stage) override;

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    /**
     * Calls handleIncomingFrame() for frames arrived from outside,
     * and processFrame() for self messages.
     */
    virtual void handleMessage(cMessage *msg) override;

    /**
     * Writes statistics.
     */
    virtual void finish() override;

    // for lifecycle:

  protected:

    virtual void handleSelfMessage(cMessage *message);

    virtual void handleUpperPacket(cPacket *msg);

    virtual void handleLowerPacket(cPacket *msg);

    //Method to create and send hello frames to other switches (currently only at the beginning).
    //Sends broadcast hello messages, src is MAC address and dst is broadcast MAC address
    virtual void sendAndScheduleHello();

    //functionality of sink node
    virtual void startCore(int core);

    //Sends broadcast SetHLMAC message to neighbors
    virtual void sendToNeighbors(HLMACAddress prefix);

    HLMACAddress extractMyAddress(cPacket *frame);

    //functionality of each node when it receives a SetHLMAC message
    virtual void receiveSetHLMACMessage(HLMACAddress hlmac, simtime_t arrivalTime);

    //check whether received HLMAC address creates a loop or not.
    virtual bool hasLoop(HLMACAddress hlmac);

    //saves received HLMAC address in the HLMAC table
    virtual bool saveHLMAC(HLMACAddress hlmac);

    virtual cObject *setUpControlInfo(cMessage *const pMsg, const MACAddress& pSrcAddr);

    virtual void sendUp(cMessage *message);

    virtual void sendDown(cMessage *message, double delay = 0);

    virtual bool isUpperMessage(cMessage *message);

    virtual bool isLowerMessage(cMessage *message);

    virtual void start();

    virtual void stop();
  public:
    virtual void getMessageStatistics(long &numHelloSent, long &numHLMACSent);

    virtual void getTableStatistics(long &numAllowedNeighbors, long &numNeighbors);

};

} // namespace iotorii

#endif // ifndef IOTORII_SRC_LINKLAYER_GA3SWITCH_IOTORIIOPERATION_H

