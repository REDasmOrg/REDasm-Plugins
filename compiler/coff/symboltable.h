#pragma once

// http://www.delorie.com/djgpp/doc/coff/symtab.html
// https://www.ti.com/lit/an/spraao8/spraao8.pdf
// http://wiki.osdev.org/COFF

#include <functional>
#include <rdapi/rdapi.h>
#include "types.h"

class COFF
{
    public:
        COFF(RDContext* ctx, COFF_FileHeader* fileheader, rd_address imagebase);
        bool parse();

    private:
        const char* nameFromTable(rd_offset offset) const;
        std::string nameFromEntry(const char* name) const;
        RDLocation getLocation(const COFF_SymbolTable* symbol) const;
        void parseCEXT(rd_address address, const std::string& name, const COFF_SymbolTable* symbol) const;
        void parseCSTAT(rd_address address, const std::string& name, const COFF_SymbolTable* symbol) const;

    private:
        RDContext* m_context;
        RDDocument* m_document;
        COFF_FileHeader* m_fileheader;
        COFF_SectionHeader* m_sections;
        rd_address m_imagebase;
        const char* m_stringtable;
};
