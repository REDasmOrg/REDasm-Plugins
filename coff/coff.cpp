#include <redasm/redasm.h>
#include <redasm/plugins/plugin.h>
#include "coff_symboltable.h"

using namespace REDasm;

REDASM_PLUGIN("Dax", "COFF Parser", "MIT", 1)

REDASM_EXEC
{
    if(!args.expect({Variant::POINTER, Variant::INTEGER}))
        return false;

    u8* data = variant_pointer<u8>(args[0]);
    size_t count = args[1].toU32();

    COFFSymbolTable coff(data, count);

    coff.read([&](const String& name, const COFF_Entry* entry) {
        const Segment* segment = r_doc->segmentAt(entry->e_scnum - 1);
        r_doc->function(segment->address + entry->e_value, name);
    });

    return false;
}
