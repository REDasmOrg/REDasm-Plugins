#include <rdapi/rdapi.h>
#include "msvc/msvccompiler.h"

void rdplugin_init(RDContext*, RDPluginModule* pm)
{
    RD_PLUGIN_ENTRY(RDEntryAnalyzer, msvcanalyzer, "MSVC C++ ABI Analyzer");
    msvcanalyzer.description = "Analyze C++ ABI and RTTI";
    msvcanalyzer.flags = AnalyzerFlags_Experimental /*| AnalyzerFlags_Selected */ | AnalyzerFlags_RunOnce;
    msvcanalyzer.order = 2000;
    msvcanalyzer.isenabled = &MSVCCompiler::isEnabled;
    msvcanalyzer.execute = &MSVCCompiler::execute;

    RDAnalyzer_Register(pm, &msvcanalyzer);
}
