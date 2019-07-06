#include <redasm/redasm.h>
#include <redasm/plugins/plugin.h>
#include "coff_symboltable.h"

using namespace REDasm;

REDASM_PLUGIN("Dax", "COFF Parser", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({Variant::POINTER, Variant::INTEGER}))
        return false;

    Disassembler* disassembler = r_ctx->disassembler();
    Loader* loader = disassembler->loader();
    u8* data = variant_pointer<u8>(args[0]);
    size_t count = args[1].toU32();

    COFFSymbolTable coff(data, count);

    coff.read([&](const String& name, const COFF_Entry* entry) {
        const Segment* segment = variant_object<Segment>(loader->document()->segments().at(entry->e_scnum - 1));
        loader->document()->lock(segment->address + entry->e_value, name, SymbolType::Function);
    });

    return false;
}
