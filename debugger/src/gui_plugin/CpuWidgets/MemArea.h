/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Memory Editor area.
 */

#pragma once

#include "api_core.h"   // MUST BE BEFORE QtWidgets.h or any other Qt header.
#include "attribute.h"
#include "igui.h"
#include "coreservices/isocinfo.h"

#include <QtWidgets/QWidget>
#include <QtWidgets/QPlainTextEdit>

namespace debugger {

class MemArea : public QPlainTextEdit,
                public IGuiCmdHandler {
    Q_OBJECT
public:
    MemArea(IGui *gui, QWidget *parent, uint64_t addr, uint64_t sz);
    virtual ~MemArea();

    /** IGuiCmdHandler */
    virtual void handleResponse(const char *cmd);

signals:
    void signalUpdateData();

public slots:
    void slotAddressChanged(AttributeType *cmd);
    void slotUpdateByTimer();
    void slotUpdateData();

private:
    void to_string(uint64_t addr, unsigned bytes, AttributeType *out);

private:
    AttributeType cmdRead_;
    AttributeType respRead_;
    QString name_;
    IGui *igui_;

    AttributeType data_;
    AttributeType tmpBuf_;
    AttributeType dataText_;

    uint64_t reqAddr_;
    unsigned reqBytes_;
    uint64_t reqAddrZ_;
    unsigned reqBytesZ_;
    bool waitingResponse_;
};

}  // namespace debugger
