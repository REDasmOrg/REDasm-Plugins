#pragma once

#include <rdapi/rdapi.h>

struct RTTICompleteObjectLocator
{
    u32 signature;
    u32 offset, cdOffset;
    u32 pTypeDescriptor;           // x86 -> VA, x64 -> RVA
    u32 pClassHierarchyDescriptor;
    //u32 pSelf;                   // x64 only
};

template<typename T> struct RTTITypeDescriptorT
{
    T pVFTable, spare;
    char name[1];
};

using RTTITypeDescriptor32 = RTTITypeDescriptorT<u32>;
using RTTITypeDescriptor64 = RTTITypeDescriptorT<u64>;
