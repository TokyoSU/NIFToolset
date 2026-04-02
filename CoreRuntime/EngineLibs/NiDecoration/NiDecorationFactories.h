// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#ifndef NIDECORATIONFACTORIES_H
#define NIDECORATIONFACTORIES_H

#include "NiTFactory.h"

#include "NiDecorationLibType.h"

#include "NiDecorationFunctor.h"
#include "NiDecorationGenerator.h"

template <class T> class NiTFunctorFactory : public NiTFactory<T>
{
public:
    typedef bool (*ValidateFunction)(NiObject*);

    NiTFunctorFactory()
    {
        m_pkValidators = NiNew NiTFixedStringMap<ValidateFunction>;
    };

    ~NiTFunctorFactory()
    {
        if (m_pkValidators)
        {
            m_pkValidators->RemoveAll();
            NiDelete m_pkValidators;
            m_pkValidators = 0;
        }
    };

    inline bool IsValidTargetType(const NiFixedString& kName,
        NiObject* pkTarget)
    {
        ValidateFunction pfnFunc;
        EE_ASSERT(m_pkValidators);
        if (!m_pkValidators->GetAt(kName, pfnFunc))
        {
            // validator not registered
            return NULL;
        }

        return pfnFunc(pkTarget);
    }

    inline void RegisterValidator(NiFixedString& kName, ValidateFunction pfnFunc)
    {
        EE_ASSERT(m_pkValidators);
        m_pkValidators->SetAt(kName, pfnFunc); 
    }

protected:
    NiTFixedStringMap<ValidateFunction>* m_pkValidators;
};

class NIDECORATION_ENTRY NiDecorationFactories
{
public:
    static NiTFunctorFactory<NiDecorationFunctorBase*>* GetFunctorFactory();
    static NiTFactory<NiDecorationGenerator*>* GetGeneratorFactory();

    // *** begin Emergent internal use only ***
    static void _SDMInit();
    static void _SDMShutdown();
    static void _SDMRegister();
    // *** end Emergent internal use only ***

private:
    static NiTFunctorFactory<NiDecorationFunctorBase*>* ms_pkFunctorFactory;
    static NiTFactory<NiDecorationGenerator*>* ms_pkGeneratorFactory;
};

// Factory Registration Macros
#define NiFactoryRegisterDecorationFunctor(functorclass) \
    NiFactoryRegisterPersistent(functorclass, \
        NiDecorationFactories::GetFunctorFactory());\
    NiDecorationFactories::GetFunctorFactory()->RegisterValidator(\
        functorclass::FUNCTOR_NAME, functorclass::IsValidTargetType); 

#define NiFactoryRegisterDecorationGenerator(generatorclass) \
    NiFactoryRegisterPersistent(generatorclass, \
    NiDecorationFactories::GetGeneratorFactory())

// Factory Declaration Macros
#define NiFactoryDeclareDecorationFunctor(functorclass) \
class functorclass##Creator \
{   \
public: \
    static void RegisterPersistent( \
    NiTFactory<NiDecorationFunctorBase*>* pkFactory) \
{   \
    NiFixedString kName(functorclass::FUNCTOR_NAME); \
    pkFactory->RegisterPersistent( \
    kName, \
    functorclass##Creator::Create, \
    functorclass##Creator::Destroy); \
}   \
private:    \
    static NiDecorationFunctorBase* Create(const char*)   \
{   \
    return NiNew NiTDecorationFunctor<functorclass>;    \
}   \
    static void Destroy(NiDecorationFunctorBase* pkDelete) \
{   \
    NiDelete ((NiTDecorationFunctor<functorclass>*)pkDelete);  \
}   \
}; 

#define NiFactoryDeclareDecorationGenerator(generatorclass) \
class generatorclass##Creator \
{   \
public: \
    static void RegisterPersistent( \
    NiTFactory<NiDecorationGenerator*>* pkFactory) \
{   \
    NiFixedString kName(generatorclass::GENERATOR_NAME); \
    pkFactory->RegisterPersistent( \
    kName, \
    generatorclass##Creator::Create, \
    generatorclass##Creator::Destroy); \
}   \
private:    \
    static NiDecorationGenerator* Create(const char*)  \
{   \
    generatorclass* pkGenerator = NiNew generatorclass;\
    return pkGenerator;    \
}   \
    static void Destroy(NiDecorationGenerator* pkDelete) \
{   \
    NiDelete ((generatorclass*)pkDelete);  \
}   \
}; 

#endif // NIDECORATIONFACTORIES_H