/*
 *  Copyright 2018 Sergey Khabarov, sergeykhbr@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <api_core.h>
#include "iotypes.h"

namespace debugger {

IORegType::IORegType(IService *parent, const char *name,
    uint32_t addr, uint32_t len, int priority) {
    if (parent == NULL) {
    } else {
        parent->registerPortInterface(name,
            static_cast<IMemoryOperation *>(this));
        parent->registerPortInterface(name, static_cast<IIOPort *>(this));
        parent->registerPortInterface(name,
            static_cast<IResetListener *>(this));
    }
    parent_ = parent;
    portListeners_.make_list(0);
    regname_.make_string(name);
    baseAddress_.make_uint64(addr);
    length_.make_uint64(len);
    priority_.make_int64(priority);
}

void IORegType::registerPortListener(IFace *listener) {
    AttributeType item;
    item.make_iface(listener);
    portListeners_.add_to_list(&item);
}

void IORegType::unregisterPortListener(IFace *listener) {
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        if (listener == portListeners_[i].to_iface()) {
            portListeners_.remove_from_list(i);
            break;
        }
    }
}

IFace *IORegType::getInterface(const char *name) {
    if (strcmp(name, IFACE_MEMORY_OPERATION) == 0) {
        return static_cast<IMemoryOperation *>(this);
    }
    return parent_->getInterface(name);
}

IOReg8Type::IOReg8Type(IService *parent, const char *name,
                  uint32_t addr, uint32_t len, int priority) :
                  IORegType(parent, name, addr, len, priority) {
    value.byte = 0;
    hard_reset_value_ = 0;
}

void IOReg8Type::reset(bool active) {
    if (!active) {
        return;
    }
    write(hard_reset_value_);
}

ETransStatus IOReg8Type::b_transport(Axi4TransactionType *trans) {
    uint16_t addr = static_cast<uint16_t>(trans->addr);
    if (trans->action == MemAction_Read) {
        trans->rpayload.b8[0] = read();
        RISCV_debug("Read %s [%02x] => %02x",
                    regName(), addr, trans->rpayload.b8[0]);
    } else {
        write(trans->wpayload.b8[0]);
        RISCV_debug("Write %s [%02x] <= %02x",
                    regName(), addr, trans->wpayload.b8[0]);
    }
    return TRANS_OK;
}

uint8_t IOReg8Type::read() {
    IIOPortListener *lstn;
    uint8_t odata = value.byte;
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener *>(portListeners_[i].to_iface());
        lstn->readData(&odata, get_direction());
    }
    return odata;
}

void IOReg8Type::write(uint8_t data) {
    IIOPortListener *lstn;
    value.byte = data;
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener *>(portListeners_[i].to_iface());
        lstn->writeData(data, get_direction());
    }
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener *>(portListeners_[i].to_iface());
        lstn->latch();
    }
}

IOReg32Type::IOReg32Type(IService *parent, const char *name,
    uint32_t addr, uint32_t len, int priority) :
    IORegType(parent, name, addr, len, priority) {
    value.buf32[0] = 0;
    hard_reset_value_ = 0;
}

void IOReg32Type::reset(bool active) {
    if (!active) {
        return;
    }
    write(hard_reset_value_);
}

ETransStatus IOReg32Type::b_transport(Axi4TransactionType *trans) {
    uint16_t addr = static_cast<uint16_t>(trans->addr);
    if (trans->action == MemAction_Read) {
        trans->rpayload.b32[0] = read();
        RISCV_debug("Read %s [%08x] => %08x",
            regName(), addr, trans->rpayload.b32[0]);
    } else {
        write(trans->wpayload.b32[0]);
        RISCV_debug("Write %s [%08x] <= %08x",
            regName(), addr, trans->wpayload.b32[0]);
    }
    return TRANS_OK;
}

uint32_t IOReg32Type::read() {
    IIOPortListener32 *lstn;
    uint32_t odata = value.buf32[0];
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener32 *>(portListeners_[i].to_iface());
        lstn->readData(&odata, get_direction());
    }
    return odata;
}

void IOReg32Type::write(uint32_t data) {
    IIOPortListener32 *lstn;
    value.buf32[0] = data;
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener32 *>(portListeners_[i].to_iface());
        lstn->writeData(data, get_direction());
    }
    for (unsigned i = 0; i < portListeners_.size(); i++) {
        lstn = static_cast<IIOPortListener32 *>(portListeners_[i].to_iface());
        lstn->latch();
    }
}
/** */
IOPinType::IOPinType(IService *parent, const char *name) : parent_(parent) {
    pinName_.make_string(name);
    iwire_ = 0;
    value_ = 0;
    bitIdx_ = 0;
    access_ = 0;

    char tstr[256];
    RISCV_sprintf(tstr, sizeof(tstr), "pin_%s", name);
    parent_->registerAttribute(tstr, &IOPinTypeCfg_);
}

IOPinType::IOPinType(IService *parent, const char *name, AttributeType &pincfg)
    : parent_(parent) {
    pinName_.make_string(name);
    iwire_ = 0;
    value_ = 0;
    bitIdx_ = 0;
    access_ = 0;
    IOPinTypeCfg_ = pincfg;
}

// Memory access use direction xDD register value (IN/OUT)
void IOPinType::readData(uint8_t *val, uint8_t mask) {
    uint8_t v;
    if (iwire_ && (access_ & READ_MASK)) {
        v = iwire_->getLevel();
    } else {
        v = aboutToRead(value_);
    }
    *val &= ~(1 << bitIdx_);
    *val |= (v << bitIdx_);
}

void IOPinType::writeData(uint8_t val, uint8_t mask) {
    prelatch_ = (val >> bitIdx_) & 0x1;
    if (iwire_ && (access_ & WRITE_MASK) && (mask & (1u << bitIdx_))) {
        iwire_->setLevel(prelatch_ == 0 ? false : true);
    } else {
        aboutToWrite(value_, prelatch_);
    }
}

// Direct access to wire doesn't use Direction xDD mask register
uint8_t IOPinType::get_bit() {
    if (iwire_ && (access_ & READ_MASK)) {
        return iwire_->getLevel() ? 1u : 0;
    }
    return value_;
}

void IOPinType::set_bit(uint8_t v) {
    if (iwire_ && (access_ & WRITE_MASK)) {
        iwire_->setLevel(v == 0 ? false : true);
    }
    prelatch_ = v;
    value_ = prelatch_;
}


void IOPinType::latch() {
    value_ = prelatch_;
}

void IOPinType::postinit() {
    connectToBit(IOPinTypeCfg_);
    if (IOPinTypeCfg_.size() >= 3) {
        connectToWire(IOPinTypeCfg_[2]);
    }
}

void IOPinType::connectToBit(const AttributeType &cfg) {
    if (!cfg.is_list() || cfg.size() < 2) {
        RISCV_printf(NULL, LOG_ERROR,
            "Cannot connect IOPinType %s: Wrong format of port attribute",
            pinName_.to_string());
        return;
    }
    IIOPort *iport = 0;
    if (cfg[0u].is_string()) {
        iport = static_cast<IIOPort *>(RISCV_get_service_iface(
                cfg[0u].to_string(), IFACE_IOPORT));
    } else if (cfg[0u].is_list()) {
        const AttributeType &prt = cfg[0u];
        iport = static_cast<IIOPort *>(RISCV_get_service_port_iface(
                prt[0u].to_string(), prt[1].to_string(), IFACE_IOPORT));
    }
    bitIdx_ = cfg[1].to_int();

    if (iport == 0) {
        RISCV_printf(NULL, LOG_ERROR,
            "Cannot connect IOPinType: Can't get port interface %s",
             cfg[0u].to_string());
        return;
    }
    iport->registerPortListener(static_cast<IIOPortListener *>(this));
}

void IOPinType::connectToWire(const AttributeType &cfg) {
    if (!cfg.is_list() || cfg.size() < 2) {
        RISCV_printf(NULL, LOG_ERROR,
            "[%s] Cannot connect IWire input: wrong attribute",
            pinName_.to_string());
        return;
    }
    const char *rw;
    if (cfg.size() == 2) {
        iwire_ = static_cast<IWire *>(RISCV_get_service_iface(
                cfg[0u].to_string(), IFACE_WIRE));
        rw = cfg[1].to_string();
    } else {
        iwire_ = static_cast<IWire *>(RISCV_get_service_port_iface(
                cfg[0u].to_string(), cfg[1].to_string(), IFACE_WIRE));
        rw = cfg[2].to_string();
    }
    if (!iwire_) {
        RISCV_printf(NULL, LOG_ERROR,
            "[%s] Cannot find IWire interface in %s",
            pinName_.to_string(), cfg[0u].to_string());
        return;
    }

    access_ = 0;
    if (strstr(rw, "r")) {
        access_ |= READ_MASK;
        prelatch_ = iwire_->getLevel();
        value_ = prelatch_;
    }
    if (strstr(rw, "w")) {
        access_ |= WRITE_MASK;
    }
}

}  // namespace debugger

