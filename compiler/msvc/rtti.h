#pragma once

#include <rdapi/rdapi.h>
#include <unordered_set>
#include "rtti_types.h"

class MSVCRTTI
{
    public:
        MSVCRTTI(RDContext* ctx);
        void search();

    private:
        std::string objectName(const RTTICompleteObjectLocator* pobjloc) const;
        const RTTICompleteObjectLocator* findObjectLocator(rd_address vtableaddress, const u32** ppvtable) const;
        void createVTable(const u32* pvtable, const RTTICompleteObjectLocator* pobjloc);
        bool createType(const RTTICompleteObjectLocator* pobjloc);
        bool createHierarchy(rd_address address);
        void checkVTable(rd_address vtableaddress);
        void checkTypeInfo();
        void registerTypes();

    private:
        std::unordered_set<rd_address> m_vtables, m_donebases;
        rd_ptr<RDType> m_baseclassdescr;
        RDContext* m_context;
        RDDocument* m_document;
        RDLoader* m_loader;
};

