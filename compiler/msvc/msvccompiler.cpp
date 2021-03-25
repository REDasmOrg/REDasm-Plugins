#include "msvccompiler.h"

std::unique_ptr<MSVCRTTI> MSVCCompiler::m_rtti;

bool MSVCCompiler::isEnabled(const RDContext* ctx)
{
    return RDContext_MatchAssembler(ctx, "x86*") &&
           RDContext_GetABI(ctx) == CompilerABI_MSVC;
}

void MSVCCompiler::execute(RDContext* ctx)
{
    if(!m_rtti) m_rtti = std::make_unique<MSVCRTTI>(ctx);
    MSVCCompiler::checkInitTerm(ctx);
    m_rtti->search();
}

std::optional<rd_address> MSVCCompiler::extractInitTermArg(RDContext* ctx, rd_address address)
{
    rd_ptr<RDILExpression> e(RDILExpression_Create(ctx, address));
    if(!e || (RDILExpression_Type(e.get()) != RDIL_Push)) return std::nullopt;

    auto* exprval = RDILExpression_Extract(e.get(), "u:cnst");
    if(!exprval) return std::nullopt;

    RDILValue val;
    if(!RDILExpression_GetValue(exprval, &val)) return std::nullopt;
    return val.address;
}

void MSVCCompiler::parseInitTerm(RDContext* ctx, rd_address address)
{
    RDDocument* doc = RDContext_GetDocument(ctx);
    auto* net = RDContext_GetNet(ctx);

    const RDReference* refs = nullptr;
    size_t c = RDNet_GetReferences(net, address, &refs);

    size_t addresswidth = RDContext_GetAddressWidth(ctx);

    for(size_t i = 0; i < c; i++)
    {
        auto* n = RDNet_FindNode(net, refs[i].address);
        if(!n) continue;

        rd_ptr<RDILExpression> e(RDILExpression_Create(ctx, refs[i].address));
        if(!e || (RDILExpression_Type(e.get()) != RDIL_Call)) continue;

        n = RDNet_GetPrevNode(net, n);
        if(!n) continue;
        auto start = MSVCCompiler::extractInitTermArg(ctx, RDNetNode_GetAddress(n));

        n = RDNet_GetPrevNode(net, n);
        if(!n) continue;
        auto end = MSVCCompiler::extractInitTermArg(ctx, RDNetNode_GetAddress(n));

        if(!start || !end) continue;

        for(rd_address a = *start; a < *end; a += addresswidth)
        {
            auto loc = RDContext_Dereference(ctx, a);
            if(!loc.valid) continue;

            if(loc.address)
            {
                RDDocument_CreateFunction(doc, loc.address, nullptr);
                RDDocument_SetPointer(doc, a, nullptr);
            }
            else
                RDDocument_SetData(doc, a, addresswidth, nullptr);
        }
    }
}

void MSVCCompiler::checkInitTerm(RDContext* ctx)
{
    static const std::vector<std::string> FUNCTIONS = {
        "thunk_*_initterm",
        "thunk_*_initterm_e"
    };

    RDDocument* doc = RDContext_GetDocument(ctx);

    for(const std::string& q : FUNCTIONS)
    {
        rd_address address;
        if(!RDDocument_FindLabel(doc, q.c_str(), &address)) continue;

        if(RDDocument_GetFlags(doc, address) & AddressFlags_Function)
            MSVCCompiler::parseInitTerm(ctx, address);
    }
}
