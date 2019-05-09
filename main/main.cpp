#include "cg_config.h"
#include "cg_logger.h"


int main(int /*argc*/, char ** /*argv*/) {
    CG::Logger::getInstance().configure("data/logger.cfg", "Logger");

    try {

    } catch (CG::Exception &ex) {
        CG_FATAL(0, "%s\n", ex._info);
    }

    return 0;
}