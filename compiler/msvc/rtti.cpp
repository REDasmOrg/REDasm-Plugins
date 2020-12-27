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
    RDDocument_EachFunction(m_document, [](rd_address address, void* userdata) {
        rd_statusaddress("Searching vtables", address);

        auto* thethis = reinterpret_cast<MSVCRTTI*>(userdata);
        rd_ptr<RDILFunction> il(RDILFunction_Create(thethis->m_context, address));
        if(!il) return true;

        size_t c = RDILFunction_Size(il.get());

        for(size_t i = 0; i < c; i++) {
            auto* expr = RDILFunction_GetExpression(il.get(), i);
            if(!expr) break;
            if(!RDILExpression_Match(expr, "[cnst]=cnst") && !RDILExpression_Match(expr, "[reg]=cnst")) continue;

            RDILValue val;
            expr = RDILExpression_Extract(expr, "src:cnst");
            if(!expr || !RDILExpression_GetValue(expr, &val) || !RD_IsAddress(thethis->m_loader, val.address)) continue;
            thethis->checkVTable(val.address);
        }

        return true;
    }, this);

    this->checkTypeInfo();
    if(!m_vtables.empty()) rd_log("Found " + std::to_string(m_vtables.size()) + " RTTI Objects");
}

std::string MSVCRTTI::objectName(const RTTICompleteObjectLocator* pobjloc) const
{
    auto* ptypedescr = reinterpret_cast<const RTTITypeDescriptor32*>(RD_AddrPointer(m_loader, pobjloc->pTypeDescriptor));
    return RD_Demangle(("?" + std::string(reinterpret_cast<const char*>(&ptypedescr->name)).substr(4) + "6A@Z").c_str());
}

const RTTICompleteObjectLocator* MSVCRTTI::findObjectLocator(rd_address vtableaddress, const u32** ppvtable) const
{
    const u32* pvtable = reinterpret_cast<const u32*>(RD_AddrPointer(m_loader, vtableaddress));
    if(!pvtable) return nullptr;

    const auto* pobjloc = reinterpret_cast<const RTTICompleteObjectLocator*>(RD_AddrPointer(m_loader, *(pvtable - 1)));
    if(!pobjloc || !pobjloc->pClassHierarchyDescriptor || !RD_IsAddress(m_loader, pobjloc->pClassHierarchyDescriptor)) return nullptr;

    if(ppvtable) *ppvtable = pvtable;
    return pobjloc;
}

bool MSVCRTTI::createType(const RTTICompleteObjectLocator* pobjloc)
{
    auto loc = RD_AddressOf(m_loader, pobjloc);
    if(!loc.valid) return false;

    RDDocument_AddTypeName(m_document, loc.address, DB_RTTICOMPLOBJLOCATOR_Q);

    if(RD_IsAddress(m_loader, pobjloc->pTypeDescriptor))
        RDDocument_AddTypeName(m_document, pobjloc->pTypeDescriptor, DB_RTTITYPEDESCR_Q);

    return this->createHierarchy(pobjloc->pClassHierarchyDescriptor);
}

bool MSVCRTTI::createHierarchy(rd_address address)
{
    if(m_donebases.count(address)) return true;
    m_donebases.insert(address);

    auto* pchdescr = reinterpret_cast<RTTIClassHierarchyDescriptor*>(RD_AddrPointer(m_loader, address));
    if(!pchdescr) return false;

    RDDocument_AddTypeName(m_document, address, DB_RTTIHIERARCHYDESCR_Q);
    if(!pchdescr->numBaseClasses) return false;

    for(size_t i = 0; i < pchdescr->numBaseClasses; i++)
    {
        rd_address a = pchdescr->pBaseClassArray + (i * sizeof(u32));
        RDDocument_AddPointer(m_document, a, SymbolType_Data, nullptr);

        u32* p = reinterpret_cast<u32*>(RD_AddrPointer(m_loader, a));
        if(!p) continue;
        RDDocument_AddType(m_document, *p, m_baseclassdescr.get());

        auto* pbaseclassdescr = reinterpret_cast<RTTIBaseClassDescriptor*>(RD_AddrPointer(m_loader, *p));
        if(!pbaseclassdescr) continue;
        RDDocument_AddTypeName(m_document, pbaseclassdescr->pTypeDescriptor, DB_RTTITYPEDESCR_Q);
        this->createHierarchy(pbaseclassdescr->pClassDescriptor);
    }

    return true;
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

void MSVCRTTI::createVTable(const u32* pvtable, const RTTICompleteObjectLocator* pobjloc)
{
    if(!this->createType(pobjloc)) return;

    auto loc = RD_AddressOf(m_loader, pvtable - 1);
    if(loc.valid) RDDocument_AddPointer(m_document, loc.address, SymbolType_Data, (this->objectName(pobjloc) + "_rtti").c_str());

    while(pvtable && *pvtable && RD_IsAddress(m_loader, *pvtable))
    {
        RDSegment s;
        if(!RDDocument_GetSegmentAddress(m_document, *pvtable, &s) || !HAS_FLAG(&s, SegmentFlags_Code)) break;

        auto loc = RD_AddressOf(m_loader, pvtable);
        if(!loc.valid) break;

        std::string vtablename = this->objectName(pobjloc) + "_vtable_" + rd_tohex(loc.address);
        std::string vmethodname = this->objectName(pobjloc) + "::vsub_" + rd_tohex(*pvtable);

        RDDocument_AddPointer(m_document, loc.address, SymbolType_Data, vtablename.c_str());
        RDContext_ScheduleFunction(m_context, *pvtable, vmethodname.c_str());
        pvtable++;
    }
}

void MSVCRTTI::checkVTable(rd_address vtableaddress)
{
    if(m_vtables.count(vtableaddress)) return;

    const u32* pvtable = nullptr;
    const auto* pobjloc = this->findObjectLocator(vtableaddress, &pvtable);
    if(!pobjloc) return;

    const auto* ptypedesc = reinterpret_cast<const RTTITypeDescriptor32*>(RD_AddrPointer(m_loader, pobjloc->pTypeDescriptor));
    if(!ptypedesc) return;

    const char* pname = reinterpret_cast<const char*>(&ptypedesc->name);
    if(!pname || std::strncmp(pname, MSVC_CLASS_PREFIX, MSVC_CLASS_PREFIX_LENGTH)) return;

    this->createVTable(pvtable, pobjloc);
    m_vtables.insert(vtableaddress);
}

void MSVCRTTI::checkTypeInfo()
{
    if(m_vtables.empty()) return;

    const u32* pvtable = nullptr;
    const auto* pobjloc = this->findObjectLocator(*m_vtables.begin(), &pvtable);
    if(!pobjloc) return;

    auto* ptypeinfodescr = reinterpret_cast<RTTITypeDescriptor32*>(RD_AddrPointer(m_loader, pobjloc->pTypeDescriptor));
    if(!ptypeinfodescr || !RD_IsAddress(m_loader, ptypeinfodescr->pVFTable)) return;
    this->checkVTable(ptypeinfodescr->pVFTable); // type_info's vtable
}
