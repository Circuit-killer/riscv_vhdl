/**
 * @file
 * @copyright  Copyright 2017 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Ethernet MAC device functional model.
 */

#ifndef __COMMON_DEBUG_GRETH_H__
#define __COMMON_DEBUG_GRETH_H__

#include "iclass.h"
#include "iservice.h"
#include "coreservices/ithread.h"
#include "coreservices/iclock.h"
#include "coreservices/imemop.h"
#include "coreservices/ilink.h"
#include "coreservices/irawlistener.h"

namespace debugger {

struct greth_map {
    uint32_t rsrv1;        /// 
    uint64_t rsrv[2];
};

struct EdclControlRequestType {
    // 32 bits fields:
    uint32_t unused : 7;
    uint32_t len    : 10;
    uint32_t write  : 1;    // read = 0; write = 1
    uint32_t seqidx : 14;   // sequence id
    //uint32 data; // 0 to 242 words
};


struct EdclControlResponseType {
    // 32 bits fields:
    uint32_t unused : 7;
    uint32_t len    : 10;
    uint32_t nak    : 1;    // ACK = 0; NAK = 1
    uint32_t seqidx : 14;   // sequence id
    //uint32 data; // 0 to 242 words
};

#pragma pack(1)
struct UdpEdclCommonType {
    uint16_t offset;
    union ControlType {
        uint32_t word;
        EdclControlRequestType request;
        EdclControlResponseType response;
    } control;
    uint32_t address;
    //uint32 data; // 0 to 242 words
};
#pragma pack()

class Greth : public IService, 
              public IThread,
              public IMemoryOperation,
              public IAxi4NbResponse {
 public:
    Greth(const char *name);
    virtual ~Greth();

    /** IService interface */
    virtual void postinitService();

    /** IMemoryOperation */
    virtual ETransStatus b_transport(Axi4TransactionType *trans);
    
    /** IAxi4NbResponse */
    virtual void nb_response(Axi4TransactionType *trans);

 protected:
    /** IThread interface */
    virtual void busyLoop();

 private:
    void write32(uint8_t *buf, uint32_t v);
    uint32_t read32(uint8_t *buf);
    void sendNAK(UdpEdclCommonType *req);

 private:
    AttributeType ip_;
    AttributeType mac_;
    AttributeType bus_;
    AttributeType transport_;

    IMemoryOperation *ibus_;
    IClock *iclk0_;
    ILink *itransport_;

    uint8_t rxbuf_[1<<12];
    uint8_t txbuf_[1<<12];
    uint32_t seq_cnt_ : 14;

    Axi4TransactionType trans_;
    event_def event_tap_;

    greth_map regs_;
};

DECLARE_CLASS(Greth)

}  // namespace debugger

#endif  // __COMMON_DEBUG_GRETH_H__
