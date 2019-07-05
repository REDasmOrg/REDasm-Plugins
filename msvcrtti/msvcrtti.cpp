#include <redasm/redasm.h>
#include "rtti_msvc.h"

using namespace REDasm;

REDASM_PLUGIN("MSVC RTTI Parser", "Dax", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({ArgumentList::INTEGER}))
        return false;

    size_t bits = args[0].toU32();

    if(bits == 64)
        RTTIMsvc<u64>(r_ctx->disassembler()).search();
    else
        RTTIMsvc<u32>(r_ctx->disassembler()).search();

    return true;
}
