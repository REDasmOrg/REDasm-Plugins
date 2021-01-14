#include <rdapi/rdapi.h>
#include "msvc/msvccompiler.h"
#include "coff/symboltable.h"

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryCommand, parsecoff, "COFF Parser");
    parsecoff.signature = "pu";

    parsecoff.isenabled = [](const RDContext* ctx) {
        return RDContext_MatchLoader(ctx, "pe");
    };

    parsecoff.execute = [](RDContext* ctx, const RDArguments* a) {
        auto* cfh = reinterpret_cast<COFF_FileHeader*>(a->args[0].p_data);
        if(!cfh) return false;

        COFF coff(ctx, cfh, static_cast<rd_address>(a->args[1].u_data));
        return coff.parse();
    };

    RDCommand_Register(pm, &parsecoff);

    RD_PLUGIN_ENTRY(RDEntryAnalyzer, msvcanalyzer, "MSVC C++ ABI Analyzer");
    msvcanalyzer.description = "Analyze C++ ABI and RTTI";
    msvcanalyzer.flags = AnalyzerFlags_Experimental | AnalyzerFlags_Selected; // | AnalyzerFlags_RunOnce;
    msvcanalyzer.order = 2000;
    msvcanalyzer.isenabled = &MSVCCompiler::isEnabled;
    msvcanalyzer.execute = &MSVCCompiler::execute;

    RDAnalyzer_Register(pm, &msvcanalyzer);
}
