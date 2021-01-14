#include "symboltable.h"
#include <cstring>
#include <string>

#define COFF_NEXTENTRY(e)   reinterpret_cast<COFF_SymbolTable*>(RD_RelPointer(e, (e->n_numaux + 1) * sizeof(COFF_SymbolTable)))
#define COFF_IS_FUNCTION(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT))

COFF::COFF(RDContext* ctx, COFF_FileHeader* fileheader, rd_address imagebase): m_context(ctx), m_fileheader(fileheader), m_imagebase(imagebase)
{
    m_document = RDContext_GetDocument(ctx);
    m_sections = reinterpret_cast<COFF_SectionHeader*>(RD_RelPointer(fileheader, sizeof(COFF_FileHeader) + fileheader->f_opthdr));
}

bool COFF::parse()
{
    if(!m_sections) return false;

    auto* coffsymbol = reinterpret_cast<COFF_SymbolTable*>(RD_Pointer(m_context, m_fileheader->f_symptr));
    if(!coffsymbol) return false;

    m_stringtable = reinterpret_cast<const char*>(RD_RelPointer(m_fileheader, m_fileheader->f_symptr + m_fileheader->f_nsyms * sizeof(COFF_SymbolTable)));
    if(!m_stringtable) return false;

    for(u32 i = 0; coffsymbol && (i < m_fileheader->f_nsyms); i++, coffsymbol = COFF_NEXTENTRY(coffsymbol))
    {
        if(coffsymbol->n_scnum < -1) continue;

        std::string name;

        if(!coffsymbol->n_zeroes) name = this->nameFromTable(coffsymbol->n_offset);
        else name = this->nameFromEntry(reinterpret_cast<const char*>(&coffsymbol->n_name));
        if(name.empty()) continue;

        auto loc = this->getLocation(coffsymbol);
        if(!loc.valid) continue;

        switch(coffsymbol->n_sclass)
        {
            case C_EXT: this->parseCEXT(loc.address, name, coffsymbol); break;
            case C_STAT: this->parseCSTAT(loc.address, name, coffsymbol); break;
            default: break;
        }
    }

    return true;
}

const char* COFF::nameFromTable(rd_offset offset) const { return reinterpret_cast<const char*>(m_stringtable + offset); }

std::string COFF::nameFromEntry(const char *name) const
{
    size_t len = std::min(std::strlen(name), static_cast<size_t>(E_SYMNMLEN));
    return std::string(name, len);
}

RDLocation COFF::getLocation(const COFF_SymbolTable* symbol) const
{
    if(!symbol->n_scnum || (symbol->n_scnum < -1) || ((symbol->n_scnum - 1) >= m_fileheader->f_nscns)) return { };
    if(symbol->n_scnum == -1) return { {static_cast<rd_address>(symbol->n_value)}, true };
    return { { m_imagebase + m_sections[symbol->n_scnum - 1].s_vaddr + symbol->n_value }, true };
}

void COFF::parseCEXT(rd_address address, const std::string& name, const COFF_SymbolTable* symbol) const
{
    if(!symbol->n_value) return;

    RDSegment segment;
    if(!RDDocument_GetSegmentAddress(m_document, address, &segment)) return;

    if(HAS_FLAG(&segment, SegmentFlags_Code))
        RDDocument_AddFunction(m_document, address, name.c_str());
    else
    {
        size_t w = RDContext_GetAddressWidth(m_context);
        RDDocument_AddData(m_document, address, w, name.c_str());
    }
}

void COFF::parseCSTAT(rd_address address, const std::string& name, const COFF_SymbolTable* symbol) const
{
    if(!symbol->n_value) return;

    size_t w = RDContext_GetAddressWidth(m_context);
    RDDocument_AddData(m_document, address, w, name.c_str());
}
