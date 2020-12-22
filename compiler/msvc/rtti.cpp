#include "rtti.h"
#include <cstring>
#include <string>

#define MSVC_CLASS_PREFIX        ".?AV"
#define MSVC_CLASS_PREFIX_LENGTH 4

#define RTTICOMPLETEOBJECTLOCATOR_NAME "RTTICompleteObjectLocator"
#define RTTITYPEDESCRIPTOR_NAME        "RTTITypeDescriptor"
#define RTTIHIERARCHYDESCRIPTOR_NAME   "RTTIClassHierarchyDescriptor"
#define RTTIBASECLASSDESCRIPTOR_NAME   "RTTIBaseClassDescriptor"
#define DB_RTTICOMPLOBJLOCATOR_Q       (std::string("/msvcrtti/") + RTTICOMPLETEOBJECTLOCATOR_NAME).c_str()
#define DB_RTTITYPEDESCR_Q             (std::string("/msvcrtti/") + RTTITYPEDESCRIPTOR_NAME).c_str()
#define DB_RTTIHIERARCHYDESCR_Q        (std::string("/msvcrtti/") + RTTIHIERARCHYDESCRIPTOR_NAME).c_str()
#define DB_RTTIBASECLASSDESCR_Q        (std::string("/msvcrtti/") + RTTIBASECLASSDESCRIPTOR_NAME).c_str()

MSVCRTTI::MSVCRTTI(RDContext* ctx): m_context(ctx)
{
    m_document = RDContext_GetDocument(m_context);
    m_loader = RDContext_GetLoader(m_context);
    this->registerTypes();
}

void MSVCRTTI::search()
{
    this->findCompleteObjLocators();

    for(const RTTICompleteObjectLocator* col : m_completeobjs)
    {
        auto loc = RD_AddressOf(m_loader, col);
        if(!loc.valid) continue;

        RDDocument_AddTypeName(m_document, loc.address, DB_RTTICOMPLOBJLOCATOR_Q);

        if(RD_IsAddress(m_loader, col->pTypeDescriptor))
            RDDocument_AddTypeName(m_document, col->pTypeDescriptor, DB_RTTITYPEDESCR_Q);

        auto* pchdescr = reinterpret_cast<RTTIClassHierarchyDescriptor*>(RD_AddrPointer(m_loader, col->pClassHierarchyDescriptor));
        if(!pchdescr) continue;

        RDDocument_AddTypeName(m_document, col->pClassHierarchyDescriptor, DB_RTTIHIERARCHYDESCR_Q);
        if(!pchdescr->numBaseClasses) continue;

        for(size_t i = 0; i < pchdescr->numBaseClasses; i++)
        {
            rd_address a = pchdescr->pBaseClassArray + (i * sizeof(u32));
            RDDocument_AddPointer(m_document, a, SymbolType_Data, nullptr);

            u32* p = reinterpret_cast<u32*>(RD_AddrPointer(m_loader, a));
            if(p) RDDocument_AddType(m_document, *p, m_baseclassdescr.get());
        }
    }

    this->parseVTable();
}

std::string MSVCRTTI::objectName(const RTTICompleteObjectLocator* cobjloc) const
{
    auto* ptypedescr = reinterpret_cast<const RTTITypeDescriptor32*>(RD_AddrPointer(m_loader, cobjloc->pTypeDescriptor));
    return RD_Demangle(("?" + std::string(reinterpret_cast<const char*>(&ptypedescr->name)).substr(4) + "6A@Z").c_str());
}

void MSVCRTTI::registerTypes()
{
    RDDatabase* db = RDContext_GetDatabase(m_context);

    rd_ptr<RDType> cobjloc(RDType_CreateStructure(RTTICOMPLETEOBJECTLOCATOR_NAME));
    RDStructure_Append(cobjloc.get(), RDType_CreateInt(4, false), "signature");
    RDStructure_Append(cobjloc.get(), RDType_CreateInt(4, false), "offset");
    RDStructure_Append(cobjloc.get(), RDType_CreateInt(4, false), "cdOffset");
    RDStructure_Append(cobjloc.get(), RDType_CreateInt(4, false), "pTypeDescriptor");
    RDStructure_Append(cobjloc.get(), RDType_CreateInt(4, false), "pClassHierarchyDescriptor");
    RDDatabase_WriteType(db, DB_RTTICOMPLOBJLOCATOR_Q, cobjloc.get());

    rd_ptr<RDType> typedescr(RDType_CreateStructure(RTTITYPEDESCRIPTOR_NAME));
    RDStructure_Append(typedescr.get(), RDType_CreateInt(4, false), "pVFTable");
    RDStructure_Append(typedescr.get(), RDType_CreateInt(4, false), "spare");
    RDStructure_Append(typedescr.get(), RDType_CreateAsciiString(RD_NVAL), "name");
    RDDatabase_WriteType(db, DB_RTTITYPEDESCR_Q, typedescr.get());

    rd_ptr<RDType> hierarchydescr(RDType_CreateStructure(RTTIHIERARCHYDESCRIPTOR_NAME));
    RDStructure_Append(hierarchydescr.get(), RDType_CreateInt(4, false), "signature");
    RDStructure_Append(hierarchydescr.get(), RDType_CreateInt(4, false), "attributes");
    RDStructure_Append(hierarchydescr.get(), RDType_CreateInt(4, false), "numBaseClasses");
    RDStructure_Append(hierarchydescr.get(), RDType_CreateInt(4, false), "pBaseClassArray");
    RDDatabase_WriteType(db, DB_RTTIHIERARCHYDESCR_Q, hierarchydescr.get());

    m_baseclassdescr.reset(RDType_CreateStructure(RTTIBASECLASSDESCRIPTOR_NAME));
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "pTypeDescriptor");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "numContainedBases");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "mdisp");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "pdisp");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "vdisp");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "attributes");
    RDStructure_Append(m_baseclassdescr.get(), RDType_CreateInt(4, false), "pClassDescriptor");
    RDDatabase_WriteType(db, DB_RTTIBASECLASSDESCR_Q, m_baseclassdescr.get());
}

void MSVCRTTI::findCompleteObjLocators()
{
    RDDocument_EachSegment(m_document, [](const RDSegment* s, void* userdata) {
        auto* thethis = reinterpret_cast<MSVCRTTI*>(userdata);
        if(HAS_FLAG(s, SegmentFlags_Bss) || HAS_FLAG(s, SegmentFlags_Code)) return true;
        thethis->findCompleteObjLocators(s);
        return true;
    }, this);

    if(!m_completeobjs.empty()) rd_log("Found " + std::to_string(m_completeobjs.size()) + " RTTI Objects");
}

void MSVCRTTI::parseVTable()
{
    for(const auto* cobjloc : m_completeobjs)
    {
        auto loc = RD_AddressOf(m_loader, cobjloc + 1);
        if(!loc.valid) continue;

        RDBlock b;
        if(!RDDocument_GetBlock(m_document, loc.address, &b) || !IS_TYPE(&b, BlockType_Unexplored)) continue;

        RDBufferView view;
        if(!RDContext_GetBlockView(m_context, &b, &view)) continue;

        // for( ; view.size >= sizeof(u32); RDBufferView_Advance(&view, sizeof(u32)))
        // {
        //     u32* p = reinterpret_cast<u32*>(view.data);
        //     if(!RD_IsAddress(m_loader, *p)) break;

        //     RDDocument_AddPointer(m_document, RD_AddressOf(m_loader, p).address, SymbolType_Data, nullptr);
        //     RDContext_ScheduleFunction(m_context, *p, (this->objectName(cobjloc) + "sub_" + std::to_string(*p)).c_str());
        // }
    }
}

void MSVCRTTI::findCompleteObjLocators(const RDSegment* segment)
{
    rd_status("Searching RTTICompleteObjectLocator @ " + std::string(segment->name));

    RDBufferView view;
    if(!RDContext_GetSegmentView(m_context, segment, &view)) return;

    for( ; view.size >= sizeof(RTTICompleteObjectLocator); RDBufferView_Advance(&view, 1))
    {
        const auto* pobjloc = reinterpret_cast<const RTTICompleteObjectLocator*>(view.data);
        if(!pobjloc->pClassHierarchyDescriptor || !RD_IsAddress(m_loader, pobjloc->pClassHierarchyDescriptor)) continue;
        const auto* ptypedesc = reinterpret_cast<const RTTITypeDescriptor32*>(RD_AddrPointer(m_loader, pobjloc->pTypeDescriptor));
        if(!ptypedesc) continue;

        const char* pname = reinterpret_cast<const char*>(&ptypedesc->name);
        if(!pname || std::strncmp(pname, MSVC_CLASS_PREFIX, MSVC_CLASS_PREFIX_LENGTH)) continue;
        m_completeobjs.push_back(pobjloc);
    }
}
