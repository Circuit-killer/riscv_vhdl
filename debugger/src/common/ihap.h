/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Hap interface declaration.
 */

#ifndef __DEBUGGER_IHAP_H__
#define __DEBUGGER_IHAP_H__

#include <iface.h>
#include <stdarg.h>

namespace debugger {

static const char *const IFACE_HAP = "IHap";

enum EHapType {
    HAP_All,
    HAP_ConfigDone,
    HAP_Halt,
    HAP_BreakSimulation,
    HAP_CpuTurnON,
    HAP_CpuTurnOFF
};

class IHap : public IFace {
 public:
    explicit IHap(EHapType type = HAP_All) : IFace(IFACE_HAP), type_(type) {}

    EHapType getType() { return type_; }

    virtual void hapTriggered(IFace *isrc, EHapType type,
                             const char *descr) = 0;

 protected:
    EHapType type_;
};

}  // namespace debugger

#endif  // __DEBUGGER_IHAP_H__
