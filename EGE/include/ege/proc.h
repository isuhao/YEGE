﻿#ifndef Inc_ege_proc_h_
#define Inc_ege_proc_h_

#include "ege/def.h"

namespace ege
{

using CALLBACK_PROC = void();

EGEAPI int
SetCloseHandler(CALLBACK_PROC* func);

}

#endif

