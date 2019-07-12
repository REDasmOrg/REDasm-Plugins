#pragma once

#include <redasm/types/base.h>
#include <redasm/libs/visit_struct/visit_struct.hpp>

enum RTTISignatureType: u32 { x86 = 0, x64 = 1 };
struct RTTIPMD { u32 mdisp, pdisp, vdisp; };

struct RTTIBaseClassDescriptor { u32 pTypeDescriptor, numContainedBases; RTTIPMD pmd; u32 attributes; };
struct RTTIClassHierarchyDescriptor { u32 signature, attributes, numBaseClasses, pBaseClassArray; };

template<typename T> struct RTTITypeDescriptorT
{
    T pVFTable, spare;
    char name[1];
};

struct RTTICompleteObjectLocator
{
    u32 signature;
    u32 offset, cdOffset;
    u32 pTypeDescriptor;           // x86 -> VA, x64 -> RVA
    u32 pClassHierarchyDescriptor;
    //u32 pSelf;                   // x64 only
};

VISITABLE_STRUCT(RTTIPMD, mdisp, pdisp, vdisp);
VISITABLE_STRUCT(RTTIBaseClassDescriptor, pTypeDescriptor, numContainedBases, pmd, attributes);
VISITABLE_STRUCT(RTTIClassHierarchyDescriptor, signature, attributes, numBaseClasses, pBaseClassArray);

VISITABLE_STRUCT(RTTITypeDescriptorT<u32>, pVFTable, spare, name);
VISITABLE_STRUCT(RTTITypeDescriptorT<u64>, pVFTable, spare, name);

VISITABLE_STRUCT(RTTICompleteObjectLocator, signature, offset, cdOffset, pTypeDescriptor, pClassHierarchyDescriptor);
