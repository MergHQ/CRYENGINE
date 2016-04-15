// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __IShaderParamCallback_h__
#define __IShaderParamCallback_h__
#pragma once

#include <CryExtension/ICryUnknown.h>

struct ICharacterInstance;
struct IShaderPublicParams;

//! Callback object which can be used to override shader params for the game side.
struct IShaderParamCallback : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IShaderParamCallback, 0x4fb87a5f83f74323, 0xa7e42ca947c549d8);

	// <interfuscator:shuffle>
	//! Setting actual object to be worked on, but should ideally all derive from a same base pointer for characters, rendermeshes, vegetation.
	virtual void                SetCharacterInstance(ICharacterInstance* pCharInstance) {}
	virtual ICharacterInstance* GetCharacterInstance() const                            { return NULL; }

	virtual bool                Init() = 0;

	//! Called just before submitting the render proxy for rendering.
	//! Can be used to setup game specific shader params.
	virtual void SetupShaderParams(IShaderPublicParams* pParams, IMaterial* pMaterial) = 0;

	virtual void Disable(IShaderPublicParams* pParams) = 0;
	// </interfuscator:shuffle>
};

DECLARE_SHARED_POINTERS(IShaderParamCallback);

//! Used to create an extra friendly-named class used to instantiate ShaderParamCallbacks based on lua properties.
#define CREATE_SHADERPARAMCALLBACKUI_CLASS(classname, UIname, guidhigh, guidlow) \
  class classname ## UI : public classname                                       \
  {                                                                              \
                                                                                 \
    CRYINTERFACE_BEGIN()                                                         \
    CRYINTERFACE_ADD(classname)                                                  \
    CRYINTERFACE_END()                                                           \
                                                                                 \
    CRYGENERATE_CLASS(classname ## UI, UIname, guidhigh, guidlow)                \
  };                                                                             \

//! Used to create an extra friendly-named class used to instantiate ShaderParamCallbacks based on lua properties.
#define CRYIMPLEMENT_AND_REGISTER_SHADERPARAMCALLBACKUI_CLASS(classname) \
  CRYREGISTER_CLASS(classname ## UI)                                     \
  classname ## UI::classname ## UI(){}                                   \
  classname ## UI::~classname ## UI(){}                                  \

#endif //__IShaderParamCallback_h__
