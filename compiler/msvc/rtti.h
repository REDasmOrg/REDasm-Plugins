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
        void findCompleteObjLocators(const RDSegment* segment);
        void findCompleteObjLocators();
        void registerTypes();

    private:
        std::deque<const RTTICompleteObjectLocator*> m_completeobjs;
        RDContext* m_context;
        RDDocument* m_document;
        RDLoader* m_loader;
};

