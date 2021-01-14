#pragma once

#include <rdapi/types.h>
#include "constants.h"

#pragma pack(push, 1)
struct COFF_FileHeader
{
    u16 f_magic;  // Magic number
    u16 f_nscns;  // Number of Sections
    u32 f_timdat; // Time & date stamp
    u32 f_symptr; // File pointer to Symbol Table
    u32 f_nsyms;  // Number of Symbols
    u16 f_opthdr; // sizeof(Optional Header)
    u16 f_flags;  // Flags
};

struct COFF_SectionHeader
{
    char s_name[8]; // Section Name
    u32 s_paddr;    // Physical Address
    u32 s_vaddr;    // Virtual Address
    u32 s_size;     // Section Size in Bytes
    u32 s_scnptr;   // File offset to the Section data
    u32 s_relptr;   // File offset to the Relocation table for this Section
    u32 s_lnnoptr;  // File offset to the Line Number table for this Section
    u16 s_nreloc;   // Number of Relocation table entries
    u16 s_nlnno;    // Number of Line Number table entries
    u32 s_flags;    // Flags for this section
};

struct COFF_SymbolTable
{
    union {
        char n_name[E_SYMNMLEN];

        struct {
            u32 n_zeroes;
            u32 n_offset;
        };
    };

    u32 n_value;    // Value of Symbol
    s16 n_scnum;    // Section Number
    u16 n_type;     // Symbol Type
    char n_sclass;  // Storage Class
    char n_numaux;  // Auxiliary Count
};
#pragma pack(pop)

static_assert(sizeof(COFF_SymbolTable) == 18);
