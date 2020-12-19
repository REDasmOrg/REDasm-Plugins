#pragma once

#include <rdapi/rdapi.h>
#include <optional>

class MSVCCompiler
{
    public:
        MSVCCompiler() = delete;
        static bool isEnabled(const RDContext* ctx);
        static void execute(RDContext* ctx);

    private:
        static std::optional<rd_address> extractInitTermArg(RDContext* ctx, rd_address address);
        static void parseInitTerm(RDContext* ctx, rd_address address);
        static void checkInitTerm(RDContext* ctx);
};

