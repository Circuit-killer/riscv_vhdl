/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      UART functional model.
 */

#include "api_core.h"
#include "uart.h"

namespace debugger {

static const uint32_t UART_STATUS_TX_FULL     = 0x00000001;
static const uint32_t UART_STATUS_TX_EMPTY    = 0x00000002;
static const uint32_t UART_STATUS_RX_FULL     = 0x00000010;
static const uint32_t UART_STATUS_RX_EMPTY    = 0x00000020;
static const uint32_t UART_STATUS_ERR_PARITY  = 0x00000100;
static const uint32_t UART_STATUS_ERR_STOPBIT = 0x00000200;
static const uint32_t UART_CONTROL_RX_IRQ_ENA = 0x00002000;
static const uint32_t UART_CONTROL_TX_IRQ_ENA = 0x00004000;
static const uint32_t UART_CONTROL_PARITY_ENA = 0x00008000;


UART::UART(const char *name)  : IService(name) {
    registerInterface(static_cast<IMemoryOperation *>(this));
    registerInterface(static_cast<ISerial *>(this));
    registerAttribute("IrqControl", &irqctrl_);

    listeners_.make_list(0);
    RISCV_mutex_init(&mutexListeners_);

    memset(&regs_, 0, sizeof(regs_));
    regs_.status = UART_STATUS_TX_EMPTY | UART_STATUS_RX_EMPTY;

    p_rx_wr_ = rxfifo_;
    p_rx_rd_ = rxfifo_;
    rx_total_ = 0;
}

UART::~UART() {
    RISCV_mutex_destroy(&mutexListeners_);
}

void UART::postinitService() {
    iwire_ = static_cast<IWire *>(
        RISCV_get_service_port_iface(irqctrl_[0u].to_string(),
                                     irqctrl_[1].to_string(),
                                     IFACE_WIRE));
    if (!iwire_) {
        RISCV_error("Can't find IWire interface %s", irqctrl_[0u].to_string());
    }
}

int UART::writeData(const char *buf, int sz) {
    if (sz > (RX_FIFO_SIZE - rx_total_)) {
        sz = (RX_FIFO_SIZE - rx_total_);
    }
    for (int i = 0; i < sz; i++) {
        rx_total_++;
        *p_rx_wr_ = buf[i];
        if ((++p_rx_wr_) >= (rxfifo_ + RX_FIFO_SIZE)) {
            p_rx_wr_ = rxfifo_;
        }
    }

    if (regs_.status & UART_CONTROL_RX_IRQ_ENA) {
        iwire_->raiseLine();
    }
    return sz;
}

void UART::registerRawListener(IFace *listener) {
    AttributeType lstn(listener);
    RISCV_mutex_lock(&mutexListeners_);
    listeners_.add_to_list(&lstn);
    RISCV_mutex_unlock(&mutexListeners_);
}

void UART::unregisterRawListener(IFace *listener) {
    for (unsigned i = 0; i < listeners_.size(); i++) {
        IFace *iface = listeners_[i].to_iface();
        if (iface == listener) {
            RISCV_mutex_lock(&mutexListeners_);
            listeners_.remove_from_list(i);
            RISCV_mutex_unlock(&mutexListeners_);
            break;
        }
    }
}

ETransStatus UART::b_transport(Axi4TransactionType *trans) {
    uint64_t mask = (length_.to_uint64() - 1);
    uint64_t off = ((trans->addr - getBaseAddress()) & mask) / 4;
    char wrdata;
    trans->response = MemResp_Valid;
    if (trans->action == MemAction_Write) {
        for (uint64_t i = 0; i < trans->xsize/4; i++) {
            if ((trans->wstrb & (0xf << 4*i)) == 0) {
                continue;
            }
            switch (off + i) {
            case 0:
                regs_.status = trans->wpayload.b32[i];
                RISCV_info("Set status = %08x", regs_.status);
                break;
            case 1:
                regs_.scaler = trans->wpayload.b32[i];
                RISCV_info("Set scaler = %d", regs_.scaler);
                break;
            case 4:
                wrdata = static_cast<char>(trans->wpayload.b32[i]);
                RISCV_info("Set data = %s", &regs_.data);
                RISCV_mutex_lock(&mutexListeners_);
                for (unsigned n = 0; n < listeners_.size(); n++) {
                    IRawListener *lstn = static_cast<IRawListener *>(
                                        listeners_[n].to_iface());

                    lstn->updateData(&wrdata, 1);
                }
                RISCV_mutex_unlock(&mutexListeners_);
                break;
            default:;
            }
        }
    } else {
        for (uint64_t i = 0; i < trans->xsize/4; i++) {
            switch (off + i) {
            case 0:
                if (0) {
                    regs_.status &= ~UART_STATUS_TX_EMPTY;
                } else {
                    regs_.status |= UART_STATUS_TX_EMPTY;
                }
                if (rx_total_ == 0) {
                    regs_.status |= UART_STATUS_RX_EMPTY;
                } else {
                    regs_.status &= ~UART_STATUS_RX_EMPTY;
                }
                trans->rpayload.b32[i] = regs_.status;
                RISCV_info("Get status = %08x", regs_.status);
                break;
            case 1:
                trans->rpayload.b32[i] = regs_.scaler;
                RISCV_info("Get scaler = %d", regs_.scaler);
                break;
            case 4:
                if (rx_total_ == 0) {
                    trans->rpayload.b32[i] = 0;
                } else {
                    trans->rpayload.b32[i] = *p_rx_rd_;
                    rx_total_--;
                    if ((++p_rx_rd_) >= (rxfifo_ + RX_FIFO_SIZE)) {
                        p_rx_rd_ = rxfifo_;
                    }
                }
                RISCV_debug("Get data = %02x", (trans->rpayload.b32[i] & 0xFF));
                break;
            default:
                trans->rpayload.b32[i] = ~0;
            }
        }
    }
    return TRANS_OK;
}

}  // namespace debugger

