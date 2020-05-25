#include <rdapi/rdapi.h>
#include "coff_symboltable.h"

static bool execute(const RDCommandPlugin*, const RDArguments* a)
{
    if(a->args[0].type != ArgumentType_Pointer) return false;
    if(a->args[1].type != ArgumentType_Pointer) return false;
    if(a->args[2].type != ArgumentType_UInt) return false;

    RDDocument* doc = reinterpret_cast<RDDocument*>(a->args[0].p_data);
    const u8* data = reinterpret_cast<const u8*>(a->args[1].p_data);
    size_t count = a->args[2].u_data;

    COFFSymbolTable coff(data, count);

    coff.read([&](const char* name, const COFF_Entry* entry) {
        RDSegment segment;
        if(!RDDocument_GetSegmentAt(doc, entry->e_scnum - 1, &segment)) return;
        RDDocument_AddFunction(doc, segment.address + entry->e_value, name);
    });

    return false;
}

void redasm_entry()
{
    RD_PLUGIN_CREATE(RDCommandPlugin, coff, "COFF");
    coff.execute = &execute;

    RDCommand_Register(&coff);
}
