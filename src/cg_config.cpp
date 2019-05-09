#include "cg_config.h"

#include <cstdio>
#include <cstdarg>

CG::Exception::Exception(const char *info, ...) {
    va_list ap;
    va_start(ap, info);
    vsprintf(_info, info, ap);
    va_end(ap);
}

CG::Exception::~Exception() throw() {}