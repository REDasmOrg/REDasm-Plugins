#include <redasm/redasm.h>
#include "rtti_msvc.h"

using namespace REDasm;

REDASM_PLUGIN("MSVC RTTI Parser", "Dax", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({Variant::INTEGER}))
        return false;

    size_t bits = args[0].toU32();

    if(bits == 64)
        RTTIMsvc<u64>().search();
    else if(bits == 32)
        RTTIMsvc<u32>().search();
    else
    {
        r_ctx->log("Unsupported bits: " + String::number(bits));
        return false;
    }

    return true;
}
