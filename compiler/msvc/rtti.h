#pragma once

#include <rdapi/rdapi.h>
#include <deque>
#include "rtti_types.h"

class MSVCRTTI
{
    public:
        MSVCRTTI(RDContext* ctx);
        void search();

    private:
        std::string objectName(const RTTICompleteObjectLocator* cobjloc) const;
        void findCompleteObjLocators(const RDSegment* segment);
        void findCompleteObjLocators();
        void parseVTable();
        void registerTypes();

    private:
        std::deque<const RTTICompleteObjectLocator*> m_completeobjs;
        rd_ptr<RDType> m_baseclassdescr;
        RDContext* m_context;
        RDDocument* m_document;
        RDLoader* m_loader;
};

