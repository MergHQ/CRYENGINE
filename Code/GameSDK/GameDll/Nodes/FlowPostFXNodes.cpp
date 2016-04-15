// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Game.h"
#include "Nodes/G2FlowBaseNode.h"
#include "Player.h"

struct FXParamsGlobal
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<float>("Global_Brightness", 1.0f),
      InputPortConfig<float>("Global_Contrast", 1.0f),
      InputPortConfig<float>("Global_Saturation", 1.0f),
      InputPortConfig<float>("Global_Sharpening", 1.0f),
      InputPortConfig<float>("Global_ColorC", 0.0f),
      InputPortConfig<float>("Global_ColorM", 0.0f),
      InputPortConfig<float>("Global_ColorY", 0.0f),
      InputPortConfig<float>("Global_ColorK", 0.0f),
      InputPortConfig<float>("Global_ColorHue", 0.0f),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Sets the global PostFX params");
  }
};

struct FXParamsScreenFrost
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool>("ScreenFrost_Active", false),
      InputPortConfig<float>("ScreenFrost_Amount", 0.0f),
      InputPortConfig<float>("ScreenFrost_CenterAmount", 1.0f),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("ScreenFrost");
  }
};

struct FXParamsWaterDroplets
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool>("WaterDroplets_Active", false),
      InputPortConfig<float>("WaterDroplets_Amount", 0.0f),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("WaterDroplets");
  }
};

struct FXParamsGlow
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool>("Glow_Active", false),
      InputPortConfig<float>("Glow_Scale", 0.5f),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Glow");
  }
};

struct FXParamsBloodSplats
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool> ("BloodSplats_Active", false),
      InputPortConfig<int>  ("BloodSplats_Type", false),
      InputPortConfig<float>("BloodSplats_Amount", 1.0f),
      InputPortConfig<bool> ("BloodSplats_Spawn", false),
      InputPortConfig<float>("BloodSplats_Scale", 1.0f),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Bloodsplats");
  }
};

struct FXParamsGlittering
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool> ("Glittering_Active", false),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("Glittering");
  }
};

struct FXParamsSunShafts
{
  static void GetConfiguration(SFlowNodeConfig& config)
  {
    static const SInputPortConfig inputs[] = {
      InputPortConfig<bool> ("SunShafts_Active", false),
      InputPortConfig<int>  ("SunShafts_Type", 0),
      InputPortConfig<float>("SunShafts_Amount", 0.25),
      {0}
    };
    static const SOutputPortConfig outputs[] = {
      {0}
    };
		config.nFlags |= EFLN_TARGET_ENTITY;
    config.pInputPorts = inputs;
    config.pOutputPorts = outputs;
    config.sDescription = _HELP("SunShafts");
  }
};

template<class T>
class CFlowFXNode : public CFlowBaseNode<eNCT_Singleton>
{
public:
  CFlowFXNode( SActivationInfo * pActInfo )
  {
  }

  ~CFlowFXNode()
  {
  }

  virtual void GetConfiguration(SFlowNodeConfig& config)
  {
    T::GetConfiguration(config);
    config.SetCategory(EFLN_ADVANCED);
  }

  virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
  {
    if (event != eFE_Activate)
      return;

		if (InputEntityIsLocalPlayer( pActInfo )) 
    {     
      SFlowNodeConfig config;
      T::GetConfiguration(config);
      I3DEngine* pEngine = gEnv->p3DEngine;
      for (int i = 0; config.pInputPorts[i].name; ++i)
      {
        if (IsPortActive(pActInfo, i))
        {
          const TFlowInputData& anyVal = GetPortAny(pActInfo, i);
          float fVal;
          bool ok = anyVal.GetValueWithConversion(fVal);
          if (ok)
          {
            // set postfx param
            pEngine->SetPostEffectParam(config.pInputPorts[i].name, fVal);
          }
        }
      }
    }
  }

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////

class CFlowControlPlayerHealthEffect : public CFlowBaseNode<eNCT_Singleton>
{
	enum
	{
		INP_ENABLE = 0,
		INP_DISABLE,
		INP_INTENSITY
	};


public:
	CFlowControlPlayerHealthEffect( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void   ("Enable", _HELP("Enable manual control of the player health effects. Warning! while enabled, the normal ingame player health effects are disabled!")),
			InputPortConfig_Void   ("Disable", _HELP("Disable manual control of the player health effects")),
			InputPortConfig<float> ("Intensity", 0, _HELP("amount of the damaged health effect (0..1)")),
			{0}
		};
		config.pInputPorts = inputs;
		config.sDescription = _HELP("When enabled, the intensity of the player damaged health effects will be controlled manually using the 'intensity' input.\nWarning! until this node is disabled again, the normal ingame player health effects will not work!");
		config.SetCategory(EFLN_APPROVED);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				CPlayer* pClientPlayer = (CPlayer*)g_pGame->GetIGameFramework()->GetClientActor();
				if (pClientPlayer)
				{
					if (IsPortActive(pActInfo, INP_ENABLE))
					{
						if (gEnv->IsCutscenePlaying())
							pClientPlayer->GetPlayerHealthGameEffect().SetActive( true );  
						pClientPlayer->GetPlayerHealthGameEffect().ExternalSetEffectIntensity( GetPortFloat( pActInfo, INP_INTENSITY ));
						pClientPlayer->GetPlayerHealthGameEffect().EnableExternalControl( true );
					}
					if (IsPortActive(pActInfo, INP_INTENSITY))
						pClientPlayer->GetPlayerHealthGameEffect().ExternalSetEffectIntensity( GetPortFloat( pActInfo, INP_INTENSITY ));
					if (IsPortActive(pActInfo, INP_DISABLE))
					{
						pClientPlayer->GetPlayerHealthGameEffect().EnableExternalControl( false );
						if (gEnv->IsCutscenePlaying())
							pClientPlayer->GetPlayerHealthGameEffect().SetActive( false );
					}
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}
};



REGISTER_FLOW_NODE_EX("CrysisFX:PostFXGlobal", CFlowFXNode<FXParamsGlobal>, FXParamsGlobal);
REGISTER_FLOW_NODE_EX("CrysisFX:PostFXScreenFrost", CFlowFXNode<FXParamsScreenFrost>, FXParamsScreenFrost);
REGISTER_FLOW_NODE_EX("CrysisFX:PostFXWaterDroplets", CFlowFXNode<FXParamsWaterDroplets>, FXParamsWaterDroplets);
REGISTER_FLOW_NODE_EX("CrysisFX:PostFXBloodSplats", CFlowFXNode<FXParamsBloodSplats>, FXParamsBloodSplats);
REGISTER_FLOW_NODE_EX("CrysisFX:PostFXGlow", CFlowFXNode<FXParamsGlow>, FXParamsGlow);
REGISTER_FLOW_NODE_EX("CrysisFX:PostFXGlittering", CFlowFXNode<FXParamsGlittering>, FXParamsGlittering);
// REGISTER_FLOW_NODE_EX("CrysisFX:PostFXSunShafts", CFlowFXNode<FXParamsSunShafts>, FXParamsSunShafts);
REGISTER_FLOW_NODE( "Image:ControlPlayerHealthEffect", CFlowControlPlayerHealthEffect);
