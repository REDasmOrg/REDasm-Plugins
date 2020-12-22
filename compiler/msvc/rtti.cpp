#include "rtti.h"
#include <cstring>
#include <string>

#define MSVC_CLASS_PREFIX        ".?AV"
#define MSVC_CLASS_PREFIX_LENGTH 4

#define RTTICOMPLETEOBJECTLOCATOR_NAME "RTTICompleteObjectLocator"
#define RTTITYPEDESCRIPTOR_NAME        "RTTITypeDescriptor"
#define DB_RTTICOMPLOBJLOCATOR_Q       (std::string("/msvcrtti/") + RTTICOMPLETEOBJECTLOCATOR_NAME).c_str()
#define DB_RTTITYPEDESCR_Q             (std::string("/msvcrtti/") + RTTITYPEDESCRIPTOR_NAME).c_str()

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
    }
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
}

void MSVCRTTI::findCompleteObjLocators()
{
    RDDocument_EachSegment(m_document, [](const RDSegment* s, void* userdata) {
        auto* thethis = reinterpret_cast<MSVCRTTI*>(userdata);
        if(HAS_FLAG(s, SegmentFlags_Code)) thethis->findCompleteObjLocators(s);
        return true;
    }, this);
}

void MSVCRTTI::findCompleteObjLocators(const RDSegment* segment)
{
    if(HAS_FLAG(segment, SegmentFlags_Bss)) return;

    const RDBlockContainer* bc = RDDocument_GetBlocks(m_document, segment->address);
    if(!bc) return;

    rd_status("Searching RTTICompleteObjectLocator @ " + std::string(segment->name));

    RDBlockContainer_Each(bc, [](const RDBlock* b, void* userdata) {
        if(IS_TYPE(b, BlockType_Code)) return true;

        auto* thethis = reinterpret_cast<MSVCRTTI*>(userdata);
        RDBufferView view;
        if(!RDContext_GetBlockView(thethis->m_context, b, &view)) return true;

        for( ; view.size >= sizeof(RTTICompleteObjectLocator); RDBufferView_Advance(&view, 1)) {
            const auto* pobjloc = reinterpret_cast<const RTTICompleteObjectLocator*>(view.data);
            const auto* ptypedesc = reinterpret_cast<const RTTITypeDescriptor32*>(RD_AddrPointer(thethis->m_loader, pobjloc->pTypeDescriptor));
            if(!ptypedesc) continue;

            const char* pname = reinterpret_cast<const char*>(&ptypedesc->name);
            if(!pname || std::strncmp(pname, MSVC_CLASS_PREFIX, MSVC_CLASS_PREFIX_LENGTH)) continue;
            thethis->m_completeobjs.push_back(pobjloc);
        }

        return true;
    }, this);

    if(!m_completeobjs.empty()) rd_log("Found " + std::to_string(m_completeobjs.size()) + " RTTI Objects");
}
