#include <redasm/redasm.h>
#include <redasm/plugins/plugin.h>
#include "coff_symboltable.h"

using namespace REDasm;

REDASM_PLUGIN("Dax", "COFF Parser", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({ArgumentType::Pointer, ArgumentType::Integer}))
        return false;

    Disassembler* disassembler = r_ctx->disassembler();
    Loader* loader = disassembler->loader();
    u8* data = args[0].pointer<u8>();
    size_t count = args[1].integer<size_t>();

    COFFSymbolTable coff(data, count);

    coff.read([&](const std::string& name, const COFF_Entry* entry) {
        const Segment& segment = loader->document()->segments()[entry->e_scnum - 1];
        loader->document()->lock(segment.address + entry->e_value, name, SymbolType::Function);
    });

    return false;
}
