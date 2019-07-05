#pragma once

#include <redasm/redasm.h>
#include <unordered_map>
#include <forward_list>
#include "rtti_msvc_types.h"

using namespace REDasm;

template<typename T> class RTTIMsvc
{
    private:
        typedef RTTITypeDescriptorT<T> RTTITypeDescriptor;

    private:
        struct RTTICompleteObjectLocatorSearch { u32 signature, offset, cdOffset, pTypeDescriptor; };
        typedef std::forward_list<const Segment*> DataSegmentList;
        typedef std::unordered_map<address_t, const RTTITypeDescriptor*> RTTITypeDescriptorMap;
        typedef std::unordered_map<const RTTICompleteObjectLocator*, address_t> RTTICompleteObjectMap;
        typedef std::unordered_map<const RTTICompleteObjectLocator*, const T*> RTTIVTableMap;

    public:
        RTTIMsvc(Disassembler *disassembler);
        void search();

    private:
        u32 rttiSignature() const;
        address_t rttiAddress(address_t address) const;
        String objectName(const RTTICompleteObjectLocator* rttiobject) const;
        String vtableName(const RTTICompleteObjectLocator* rttiobject) const;
        void readHierarchy(document_x_lock& lock, const RTTICompleteObjectLocator* rttiobject) const;
        void searchDataSegments();
        void searchTypeDescriptors();
        void searchCompleteObjects();
        void searchVTables();

    private:
        static String objectName(const RTTITypeDescriptor* rttitype);

    private:
        Disassembler* m_disassembler;
        ListingDocument& m_document;
        const Loader* m_loader;

    private:
        RTTIVTableMap m_rttivtables;
        RTTICompleteObjectMap m_rttiobjects;
        RTTITypeDescriptorMap m_rttitypes;
        DataSegmentList m_segments;
};

#include "rtti_msvc_impl.h"
