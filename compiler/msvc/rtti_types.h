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

struct RTTIClassHierarchyDescriptor
{
    u32 signature;
    u32 attributes;
    u32 numBaseClasses;
    u32 pBaseClassArray;
};

struct RTTIBaseClassDescriptor
{
    u32 pTypeDescriptor;
    u32 numContainedBases;
    u32 mdisp, pdisp, vdisp;
    u32 attributes;
    u32 pClassDescriptor;
};

template<typename T> struct RTTITypeDescriptorT
{
    T pVFTable, spare;
    char name[1];
};

using RTTITypeDescriptor32 = RTTITypeDescriptorT<u32>;
using RTTITypeDescriptor64 = RTTITypeDescriptorT<u64>;
