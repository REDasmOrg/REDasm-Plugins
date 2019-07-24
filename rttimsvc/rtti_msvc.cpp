#include "rtti_msvc.h"
#include <redasm/disassembler/disassembler.h>
#include <redasm/support/symbolize.h>
#include <redasm/support/utils.h>

#if _MSC_VER
    #pragma warning(disable: 4146)
#endif // _MSC_VER

#define RTTI_MSVC_CLASS_DESCRIPTOR_PREFIX ".?AV"
#define RTTI_MSVC_FIXUP (sizeof(T) * 2)
#define RTTI_MSVC_TYPE_DESCRIPTOR(typedescriptorname) Loader::relpointer<RTTITypeDescriptor>(typedescriptorname, -RTTI_MSVC_FIXUP)

template<typename T> void RTTIMsvc<T>::search()
{
    this->searchDataSegments();
    this->searchTypeDescriptors();
    this->searchCompleteObjects();
    this->searchVTables();

    auto lock = x_lock_safe_ptr(r_doc);

    for(auto& rttivtableitem : m_rttivtables)
    {
        const RTTICompleteObjectLocator* rttiobject = rttivtableitem.first;
        auto it = m_rttiobjects.find(rttiobject);

        if(it == m_rttiobjects.end())
            continue;

        String objectname = this->objectName(rttiobject);
        String vtablename = this->vtableName(rttiobject);
        const T* pobjectdata = rttivtableitem.second;
        address_location address = r_ldr->addressof(pobjectdata), rttiobjectaddress = r_ldr->addressof(rttiobject);

        if(!address.valid || !rttiobjectaddress.valid)
            continue;

        r_ctx->status("Reading " + objectname + "'s VTable");

        lock->type(address, vtablename);
        lock->lock(address, objectname + "::ptr_rtti_object", SymbolType::Data | SymbolType::Pointer);

        REDasm::symbolize<RTTICompleteObjectLocator>(r_disasm, rttiobjectaddress, objectname + "::rtti_complete_object_locator");
        REDasm::symbolize<RTTIClassHierarchyDescriptor>(r_disasm, rttiAddress(rttiobject->pClassHierarchyDescriptor), objectname + "::rtti_class_hierarchy");
        r_disasm->pushReference(rttiobjectaddress, address);
        pobjectdata++; // Skip RTTICompleteObjectLocator

        const Segment* segment = lock->segment(static_cast<T>(*pobjectdata));

        for(T i = 0; segment && segment->is(SegmentType::Code); i++) // Walk vtable
        {
            address = r_ldr->addressof(pobjectdata);
            r_disasm->disassemble(*pobjectdata);

            lock->lock(address, objectname + "::vftable_" + String::number(i), SymbolType::Data | SymbolType::Pointer);
            lock->function(*pobjectdata, objectname + "::sub_" + String::hex(*pobjectdata));

            r_disasm->pushReference(*pobjectdata, address);

            pobjectdata++;
            segment = lock->segment(*pobjectdata);
        }

        this->readHierarchy(lock, rttiobject);
    }

    if(m_rttiobjects.size())
        r_ctx->log("Found " + String::number(m_rttiobjects.size()) + " RTTI objects");
    else
        r_ctx->log("No RTTI Objects found");
}

template<typename T> u32 RTTIMsvc<T>::rttiSignature() const
{
    if(REDasm::bits_count<T>::value == 64)
        return RTTISignatureType::x64;

    return RTTISignatureType::x86;
}

template<typename T> address_t RTTIMsvc<T>::rttiAddress(address_t address) const
{
    if(REDasm::bits_count<T>::value == 64)
        return r_ldr->absaddress(address);

    return address;
}

template<typename T> String RTTIMsvc<T>::objectName(const RTTIMsvc::RTTITypeDescriptor *rttitype)
{
    String rttitypename = rttitype->name;
    return Demangler::demangled("?" + rttitypename.substring(4) + "6A@Z");
}

template<typename T> String RTTIMsvc<T>::objectName(const RTTICompleteObjectLocator *rttiobject) const
{
    const RTTITypeDescriptor* rttitype = r_ldr->addrpointer<RTTITypeDescriptor>(this->rttiAddress(rttiobject->pTypeDescriptor));
    return objectName(rttitype);
}

template<typename T> String RTTIMsvc<T>::vtableName(const RTTICompleteObjectLocator *rttiobject) const
{
    const RTTITypeDescriptor* rttitype = r_ldr->addrpointer<RTTITypeDescriptor>(this->rttiAddress(rttiobject->pTypeDescriptor));
    String rttitypename = rttitype->name;
    return Demangler::demangled("??_7" + rttitypename.substring(4) + "6B@Z");
}

template<typename T> void RTTIMsvc<T>::readHierarchy(document_x_lock& lock, const RTTICompleteObjectLocator* rttiobject) const
{
    String objectname = this->objectName(rttiobject);
    RTTIClassHierarchyDescriptor* pclasshierarchy = r_ldr->addrpointer<RTTIClassHierarchyDescriptor>(this->rttiAddress(rttiobject->pClassHierarchyDescriptor));
    u32* pbcdescriptor = r_ldr->addrpointer<u32>(this->rttiAddress(pclasshierarchy->pBaseClassArray));

    for(u64 i = 0; i < pclasshierarchy->numBaseClasses; i++, pbcdescriptor++) // Walk class hierarchy
    {
        address_t bcaddress = r_ldr->addressof(pbcdescriptor);
        RTTIBaseClassDescriptor* pbaseclass = r_ldr->addrpointer<RTTIBaseClassDescriptor>(this->rttiAddress(*pbcdescriptor));

        lock->pointer(this->rttiAddress(pclasshierarchy->pBaseClassArray), SymbolType::Data);
        REDasm::symbolize<RTTIBaseClassDescriptor>(r_disasm, r_ldr->addressof(pbaseclass), objectname + "::rtti_base_class");

        RTTITypeDescriptor* rttitype = r_ldr->addrpointer<RTTITypeDescriptor>(this->rttiAddress(pbaseclass->pTypeDescriptor));
        lock->lock(bcaddress, objectname + "::ptr_base_" + objectName(rttitype) + "_" + String::hex(bcaddress), SymbolType::Data | SymbolType::Pointer);
    }
}

template<typename T> void RTTIMsvc<T>::searchDataSegments()
{
    for(size_t i = 0; i < r_doc->segments().size(); i++)
    {
        const Segment* segment = variant_object<Segment>(r_doc->segments()[i]);

        if(segment->empty() || segment->is(SegmentType::Bss) || segment->is(SegmentType::Code) || !segment->name.contains("data"))
            continue;

        r_ctx->status("Checking segment '" + segment->name + "'");
        m_segments.push_front(segment);
    }
}

template<typename T> void RTTIMsvc<T>::searchTypeDescriptors()
{
    for(const Segment* segment : m_segments)
    {
        BufferView view = r_ldr->viewSegment(segment);

        if(view.eob())
            continue;

        auto res = view.find<char>(RTTI_MSVC_CLASS_DESCRIPTOR_PREFIX);

        while(res.isValid())
        {
            const RTTITypeDescriptor* rttitype = RTTI_MSVC_TYPE_DESCRIPTOR(res.result());
            address_t rttiaddress = r_ldr->addressof(rttitype);
            r_ctx->statusAddress("Searching RTTITypeDescriptors in " + segment->name.quoted(), rttiaddress);

            if(r_doc->segment(rttitype->pVFTable))
            {
                REDasm::symbolize<RTTITypeDescriptor>(r_disasm, rttiaddress, objectName(rttitype) + "::rtti_type_descriptor");
                m_rttitypes.emplace(segment->address + res.position() - RTTI_MSVC_FIXUP, rttitype);
            }

            res = res.next();
        }
    }
}

template<typename T> void RTTIMsvc<T>::searchCompleteObjects()
{
    RTTICompleteObjectLocatorSearch searchobj = { this->rttiSignature(), 0, 0, 0 };

    for(const auto& item : m_rttitypes)
    {
        if(REDasm::bits_count<T>::value == 64)
            searchobj.pTypeDescriptor = static_cast<u32>(r_ldr->reladdress(item.first));
        else
            searchobj.pTypeDescriptor = static_cast<u32>(item.first);

        for(const Segment* segment : m_segments)
        {
            BufferView view = r_ldr->viewSegment(segment);
            auto res = view.find<RTTICompleteObjectLocatorSearch>(&searchobj);

            if(!res.isValid())
                continue;

            r_ctx->statusProgress("Searching RTTICompleteObjectLocators in " + segment->name.quoted(), r_ldr->address(res.position()));
            m_rttiobjects.emplace(reinterpret_cast<const RTTICompleteObjectLocator*>(res.result()), segment->address + res.position());
            break;
        }
    }
}

template<typename T> void RTTIMsvc<T>::searchVTables()
{
    for(const auto& item : m_rttiobjects)
    {
        const RTTICompleteObjectLocator* rttiobject = item.first;
        const char* s = this->objectName(rttiobject).c_str();
        r_ctx->status("Searching VTables for " + this->objectName(rttiobject).quoted());

        for(const Segment* segment : m_segments)
        {
            BufferView view = r_ldr->viewSegment(segment);
            SearchResult res = view.find(reinterpret_cast<const T*>(&item.second));
            bool found = false;

            while(res.isValid())
            {
                const RTTICompleteObjectLocator* foundrttiobject = r_ldr->addrpointer<RTTICompleteObjectLocator>(*reinterpret_cast<const T*>(res.result()));

                if(rttiobject != foundrttiobject)
                {
                    res = res.next();
                    continue;
                }

                found = true;
                break;
            }

            if(!found)
                continue;

            m_rttivtables.emplace(item.first, r_ldr->pointer<T>(segment->offset + res.position()));
            break;
        }
    }
}

template class RTTIMsvc<u32>;
template class RTTIMsvc<u64>;
