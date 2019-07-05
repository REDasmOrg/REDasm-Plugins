#pragma once

// http://wiki.osdev.org/COFF#Symbol_Table

#include <functional>
#include <redasm/redasm.h>
#include "coff_types.h"

using namespace REDasm;

typedef std::function<void(const String&, const COFF_Entry*)> SymbolCallback;

class COFFSymbolTable
{
    public:
        COFFSymbolTable(const u8* symdata, size_t count);
        void read(const SymbolCallback &symbolcb);
        const COFF_Entry* at(size_t index) const;

    private:
        String nameFromTable(offset_t offset) const;
        String nameFromEntry(const char* name) const;

    private:
        size_t m_count;
        const u8* m_symdata;
        const char* m_stringtable;
};

const COFF_Entry* getSymbolAt(const u8* symdata, u64 count, u32 idx);
