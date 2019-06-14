#include <redasm/redasm.h>
#include <redasm/plugins/plugin.h>
#include "coff_symboltable.h"

using namespace REDasm;

REDASM_PLUGIN("Dax", "COFF Parser", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({ArgumentType::Pointer, ArgumentType::Pointer, ArgumentType::Integer}))
        return false;

    Loader* loader = args[0].pointer<Loader>();
    u8* data = args[1].pointer<u8>();
    size_t count = args[2].integer<size_t>();

    COFFSymbolTable coff(data, count);

    coff.read([&](const std::string& name, const COFF_Entry* entry) {
        const Segment& segment = loader->document()->segments()[entry->e_scnum - 1];
        loader->document()->lock(segment.address + entry->e_value, name, SymbolType::Function);
    });

    return false;
}
