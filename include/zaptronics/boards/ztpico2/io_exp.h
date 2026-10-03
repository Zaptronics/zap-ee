#pragma once
#include "zaptronics/boards/ztpico2/board.h"
#include "zaptronics/boards/ztpico2/pins.h"
/* Compatibility accessor: valid after board initialization begins. */
#define ioexp (ztpico2_active()->expander)
