#include <redasm/redasm.h>
#include "rtti_msvc.h"

using namespace REDasm;

REDASM_PLUGIN("MSVC RTTI Parser", "Dax", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({ArgumentType::Pointer, ArgumentType::Integer}))
        return false;

    Disassembler* disassembler = args[0].pointer<Disassembler>();
    size_t bits = args[1].integer<size_t>();

    if(bits == 64)
        RTTIMsvc<u64>(disassembler).search();
    else
        RTTIMsvc<u32>(disassembler).search();

    return true;
}
