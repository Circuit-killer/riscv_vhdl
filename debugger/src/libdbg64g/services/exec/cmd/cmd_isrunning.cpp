/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Check target status: run or not.
 */

#include "cmd_isrunning.h"

namespace debugger {

CmdIsRunning::CmdIsRunning(ITap *tap, ISocInfo *info) 
    : ICommand ("isrunning", tap, info) {

    briefDescr_.make_string("Check target's status");
    detailedDescr_.make_string(
        "Description:\n"
        "    Check target's status as a boolean value.\n"
        "Example:\n"
        "    isrunning\n");
}

bool CmdIsRunning::isValid(AttributeType *args) {
    if ((*args)[0u].is_equal(cmdName_.to_string()) && args->size() == 1) {
        return CMD_VALID;
    }
    return CMD_INVALID;
}

void CmdIsRunning::exec(AttributeType *args, AttributeType *res) {
    res->make_boolean(false);
    if (!isValid(args)) {
        generateError(res, "Wrong argument list");
        return;
    }

    Reg64Type t1;
    GenericCpuControlType ctrl;
    DsuMapType *pdsu = info_->getpDsu();
    uint64_t addr = reinterpret_cast<uint64_t>(&pdsu->udbg.v.control);
    tap_->read(addr, 8, t1.buf);
    ctrl.val = t1.val;
    if (ctrl.bits.halt) {
        res->make_boolean(false);
    } else {
        res->make_boolean(true);
    }
}

}  // namespace debugger
