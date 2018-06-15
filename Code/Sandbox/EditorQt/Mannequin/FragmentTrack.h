// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ICryMannequin.h"
#include "MannequinBase.h"
#include "SequencerTrack.h"

const float LOCK_TIME_DIFF = 0.1f;

//////////////////////////////////////////////////////////////////////////
struct CFragmentKey
	: public CSequencerKey
{
	CFragmentKey()
		:
		context(NULL),
		fragmentID(FRAGMENT_ID_INVALID),
		fragOptionIdx(OPTION_IDX_RANDOM),
		scopeMask(0),
		sharedID(0),
		historyItem(HISTORY_ITEM_INVALID),
		clipDuration(0.0f),
		transitionTime(0.0f),
		tranSelectTime(0.0f),
		tranStartTime(0.0f),
		tranStartTimeValue(0.0f),
		tranStartTimeRelative(0.0f),
		tranFragFrom(FRAGMENT_ID_INVALID),
		tranFragTo(FRAGMENT_ID_INVALID),
		hasFragment(false),
		isLooping(false),
		trumpPrevious(false),
		transition(false),
		tranFlags(0),
		fragIndex(0)
	{
	}
	SScopeContextData* context;
	FragmentID         fragmentID;
	SFragTagState      tagStateFull;
	SFragTagState      tagState;
	uint32             fragOptionIdx;
	ActionScopes       scopeMask;
	uint32             sharedID;
	uint32             historyItem;
	float              clipDuration;
	float              transitionTime;
	float              tranSelectTime;
	float              tranStartTime;
	float              tranStartTimeValue;
	float              tranStartTimeRelative;
	FragmentID         tranFragFrom;
	FragmentID         tranFragTo;
	SFragTagState      tranTagFrom;
	SFragTagState      tranTagTo;
	SFragmentBlendUid  tranBlendUid;
	float              tranLastClipEffectiveStart;
	float              tranLastClipDuration;
	bool               hasFragment;
	bool               isLooping;
	bool               trumpPrevious;
	bool               transition;
	uint8              tranFlags;
	uint8              fragIndex;

	void SetFromQuery(const SBlendQueryResult& query)
	{
		transition = true;
		tranFragFrom = query.fragmentFrom;
		tranFragTo = query.fragmentTo;
		tranTagFrom = query.tagStateFrom;
		tranTagTo = query.tagStateTo;
		tranBlendUid = query.blendUid;
		tranFlags = query.pFragmentBlend->flags;
	}
};

//////////////////////////////////////////////////////////////////////////
class CFragmentIdTrack : public TSequencerTrack<CFragmentKey>
{
public:

	enum ESecondaryKey
	{
		eSK_SELECT_TIME = 1,
		eSK_START_TIME  = 2
	};

	CFragmentIdTrack(SScopeData& scopeData, EMannequinEditorMode editorMode);

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;

	void                      SetHistory(SFragmentHistoryContext& history);

	enum EMenuOptions
	{
		EDIT_FRAGMENT = KEY_MENU_CMD_BASE,
		EDIT_TRANSITION,
		INSERT_TRANSITION,
		FIND_FRAGMENT_REFERENCES,
		FIND_TAG_REFERENCES,
	};

	virtual int     GetNumSecondarySelPts(int key) const override;
	virtual int     GetSecondarySelectionPt(int key, float timeMin, float timeMax) const override;
	virtual int     FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const override;

	virtual void    SetSecondaryTime(int key, int idx, float time) override;
	virtual float   GetSecondaryTime(int key, int idx) const override;

	virtual bool    CanEditKey(int key) const override;
	virtual bool    CanMoveKey(int key) const override;
	virtual bool    CanAddKey(float time) const override;
	virtual bool    CanRemoveKey(int key) const override;

	int             GetNextFragmentKey(int key) const;
	int             GetPrecedingFragmentKey(int key, bool includeTransitions) const;
	int             GetParentFragmentKey(int key) const;

	virtual CString GetSecondaryDescription(int key, int idx) const override;

	virtual void    InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void    ClearKeyMenuOptions(CMenu& menu, int keyID) override {}

	virtual void  OnKeyMenuOption(int menuOption, int keyID) override;

	virtual void  GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float GetKeyDuration(const int key) const override;
	virtual void  SerializeKey(CFragmentKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual void  SelectKey(int keyID, bool select) override;

	void          CloneKey(int nKey, const CFragmentKey& key);
	void          DistributeSharedKey(int keyID);

	virtual void  SetKey(int index, const CSequencerKey* _key) override;

	//! Set time of specified key.
	virtual void      SetKeyTime(int index, float time) override;

	SScopeData&       GetScopeData();
	const SScopeData& GetScopeData() const;

private:
	SScopeData&              m_scopeData;
	SFragmentHistoryContext* m_history;
	EMannequinEditorMode     m_editorMode;

	static uint32            s_sharedKeyID;
	static bool              s_distributingKey;
};

struct SAnimCache
{
	SAnimCache()
		:
		animID(-1),
		pAnimSet(NULL)
	{
	}

	SAnimCache(const SAnimCache& animCache)
	{
		if (animCache.animID >= 0)
		{
			animCache.pAnimSet->AddRef(animCache.animID);
		}
		animID = animCache.animID;
		pAnimSet = animCache.pAnimSet;
	}

	~SAnimCache()
	{
		if (animID >= 0)
		{
			pAnimSet->Release(animID);
		}
	}

	SAnimCache& operator=(const SAnimCache& source)
	{
		Set(source.animID, source.pAnimSet);

		return *this;
	}

	void Set(int _animID, IAnimationSet* _pAnimSet)
	{
		if (animID >= 0)
		{
			pAnimSet->Release(animID);
		}

		if (_animID >= 0)
		{
			_pAnimSet->AddRef(_animID);
		}
		animID = _animID;
		pAnimSet = _pAnimSet;
	}

	int                       animID;
	_smart_ptr<IAnimationSet> pAnimSet;
};

//////////////////////////////////////////////////////////////////////////
struct CClipKey : public CSequencerKey
{
	CClipKey();
	~CClipKey();

	void Set(const SAnimClip& animClip, IAnimationSet* pAnimSet, const EClipType transitionType[SFragmentData::PART_TOTAL]);
	void SetupAnimClip(SAnimClip& animClip, float lastTime, int fragPart);

	// Change animation and automatically update duration & anim location flags
	void         SetAnimation(const char* szAnimName, IAnimationSet* pAnimSet);
	bool         IsAdditive() const { return animIsAdditive; }
	bool         IsLMG() const      { return animIsLMG; }
	const char*  GetDBAPath() const;

	virtual void UpdateFlags() override;

	// Returns the time the animation asset "starts" (in seconds),
	// taking into account that we can shift the animation clip back in time
	// using startTime
	ILINE float GetEffectiveAssetStartTime() const
	{
		return m_time - startTime;
	}

	// Takes into account both playbackSpeed and startTime
	ILINE float ConvertTimeToUnclampedNormalisedTime(const float timeSeconds) const
	{
		const float timeFromEffectiveStart = timeSeconds - GetEffectiveAssetStartTime();

		const float oneLoopDuration = GetOneLoopDuration();
		if (oneLoopDuration < FLT_EPSILON)
			return 0.0f;

		const float unclampedNormalisedTime = timeFromEffectiveStart / oneLoopDuration;
		return unclampedNormalisedTime;
	}

	// Takes into account both playbackSpeed and startTime
	ILINE float ConvertTimeToNormalisedTime(const float timeSeconds) const
	{
		const float unclampedNormalisedTime = ConvertTimeToUnclampedNormalisedTime(timeSeconds);
		const float normalisedTime = unclampedNormalisedTime - (int)unclampedNormalisedTime;

		return normalisedTime;
	}

	// Takes into account both playbackSpeed and startTime
	ILINE float ConvertUnclampedNormalisedTimeToTime(const float unclampedNormalisedTime) const
	{
		const float timeFromStart = GetOneLoopDuration() * unclampedNormalisedTime;
		const float timeSeconds = timeFromStart + GetEffectiveAssetStartTime();

		return timeSeconds;
	}

	// Takes into account both playbackSpeed and startTime
	ILINE float ConvertNormalisedTimeToTime(const float normalisedTime, const int loopIndex = 0) const
	{
		assert(normalisedTime >= 0);
		assert(normalisedTime <= 1.0f);

		const float unclampedNormalisedTime = normalisedTime + loopIndex;
		const float timeSeconds = ConvertUnclampedNormalisedTimeToTime(unclampedNormalisedTime);

		return timeSeconds;
	}

	// Returns the duration in seconds of one loop of the underlying animation.
	// Whether or not the clip is marked as 'looping', it will only count one loop.
	// Takes into account playbackSpeed.
	ILINE float GetOneLoopDuration() const
	{
		if (duration < FLT_EPSILON)
		{
			// when duration is 0, for example when it's a None clip,
			// we want to treat it as 0-length regardless of the playbackSpeed
			return 0.0f;
		}

		if (playbackSpeed < FLT_EPSILON)
		{
			return FLT_MAX;
		}

		return duration / playbackSpeed;
	}

	ILINE float GetDuration() const
	{
		return (duration / max(0.1f, playbackSpeed)) - startTime;
	}

	ILINE float GetAssetDuration() const
	{
		return duration;
	}

	ILINE float GetVariableDurationDelta() const
	{
		if (referenceLength > 0.0f)
		{
			return (duration / max(0.1f, playbackSpeed)) - referenceLength;
		}
		else
		{
			return 0.0f;
		}
	}

	ILINE void Serialize(XmlNodeRef& keyNode, bool bLoading)
	{
		// Variables not serialized:
		// SAnimCache m_animCache; Set on anim set
		// bool animIsAdditive; Set on anim set
		// bool animIsLMG; Set on anim set
		// CString animExtension; Set on anim set
		// const IAnimationSet* m_animSet; Set on anim set

		if (bLoading)
		{
			keyNode->getAttr("historyItem", historyItem);
			keyNode->getAttr("startTime", startTime);
			keyNode->getAttr("playbackSpeed", playbackSpeed);
			keyNode->getAttr("playbackWeight", playbackWeight);
			keyNode->getAttr("blend", blendDuration);
			for (int i = 0; i < MANN_NUMBER_BLEND_CHANNELS; ++i)
			{
				char name[16];
				cry_sprintf(name, "blendChannels%d", i);
				keyNode->getAttr(name, blendChannels[i]);
			}
			keyNode->getAttr("animFlags", animFlags);
			keyNode->getAttr("jointMask", jointMask);
			//			keyNode->getAttr("align",alignToPrevious);
			keyNode->getAttr("clipType", clipType);
			keyNode->getAttr("blendType", blendType);
			keyNode->getAttr("fragIndexBlend", fragIndexBlend);
			keyNode->getAttr("fragIndexMain", fragIndexMain);
			keyNode->getAttr("duration", duration);
			keyNode->getAttr("referenceLength", referenceLength);
		}
		else
		{
			keyNode->setAttr("historyItem", historyItem);
			keyNode->setAttr("startTime", startTime);
			keyNode->setAttr("playbackSpeed", playbackSpeed);
			keyNode->setAttr("playbackWeight", playbackWeight);
			keyNode->setAttr("blend", blendDuration);
			for (int i = 0; i < MANN_NUMBER_BLEND_CHANNELS; ++i)
			{
				char name[16];
				cry_sprintf(name, "blendChannels%d", i);
				keyNode->setAttr(name, blendChannels[i]);
			}
			keyNode->setAttr("animFlags", animFlags);
			keyNode->setAttr("jointMask", jointMask);
			//			keyNode->setAttr("align",alignToPrevious);
			keyNode->setAttr("clipType", clipType);
			keyNode->setAttr("blendType", blendType);
			keyNode->setAttr("fragIndexBlend", fragIndexBlend);
			keyNode->setAttr("fragIndexMain", fragIndexMain);
			keyNode->setAttr("referenceLength", referenceLength);
		}
	}

	uint32   historyItem;
	SAnimRef animRef;
	float    startTime;
	float    playbackSpeed;
	float    playbackWeight;
	float    blendDuration;
	float    blendChannels[MANN_NUMBER_BLEND_CHANNELS];
	int      animFlags;
	uint8    jointMask;
	bool     alignToPrevious;
	uint8    clipType;
	uint8    blendType;
	uint8    fragIndexBlend;
	uint8    fragIndexMain;

protected:
	virtual void GetExtensions(std::vector<CString>& extensions, CString& editableExtension) const override;

	float                duration;
	float                referenceLength;
	SAnimCache           m_animCache;
	bool                 animIsAdditive;
	bool                 animIsLMG;
	CString              animExtension;
	const IAnimationSet* m_animSet;
};

//////////////////////////////////////////////////////////////////////////
class CClipTrack : public TSequencerTrack<CClipKey>
{
public:
	enum EClipKeySecondarySel
	{
		eCKSS_None = 0,
		eCKSS_BlendIn,
	};

	enum EMenuOptions
	{
		FIND_FRAGMENT_REFERENCES = KEY_MENU_CMD_BASE,
	};

	CClipTrack(SScopeContextData* pContext, EMannequinEditorMode editorMode);

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;

	virtual int               GetNumSecondarySelPts(int key) const override;
	virtual int               GetSecondarySelectionPt(int key, float timeMin, float timeMax) const override;
	virtual int               FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const override;
	virtual void              SetSecondaryTime(int key, int idx, float time) override;
	virtual float             GetSecondaryTime(int key, int id) const override;
	virtual bool              CanMoveSecondarySelection(int key, int id) const override;

	virtual void              InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              ClearKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              OnKeyMenuOption(int menuOption, int keyID) override;

	virtual bool              CanAddKey(float time) const override;
	virtual bool              CanRemoveKey(int key) const override;

	virtual bool              CanEditKey(int key) const override;
	virtual bool              CanMoveKey(int key) const override;

	virtual int               CreateKey(float time) override;

	void                      CheckKeyForSnappingToPrevious(int index);

	virtual void              SetKey(int index, const CSequencerKey* _key) override;

	virtual void              SetKeyTime(int index, float time) override;

	virtual void              GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual void              GetTooltip(int key, const char*& description, float& duration) const override;
	virtual float             GetKeyDuration(const int key) const;
	virtual void              SerializeKey(CClipKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual Range             GetTrackDuration() const override;

	void                      SetMainAnimTrack(bool bSet) { m_mainAnimTrack = bSet; }

	const SScopeContextData*  GetContext() const
	{
		return m_pContext;
	}

protected:

	virtual void OnChangeCallback() override
	{
		if (m_pContext)
		{
			m_pContext->changeCount++;
		}
	}

private:
	bool                 m_mainAnimTrack;
	SScopeContextData*   m_pContext;
	EMannequinEditorMode m_editorMode;
};

//////////////////////////////////////////////////////////////////////////
struct CProcClipKey : public CSequencerKey
{
	CProcClipKey();
	IProceduralClipFactory::THash typeNameHash;
	float                         duration;
	float                         blendDuration;
	IProceduralParamsPtr          pParams;
	uint32                        historyItem;
	uint8                         clipType;
	uint8                         blendType;
	uint8                         fragIndexMain;
	uint8                         fragIndexBlend;

	void FromProceduralEntry(const SProceduralEntry& procClip, const EClipType transitionType[SFragmentData::PART_TOTAL]);
	void ToProceduralEntry(SProceduralEntry& procClip, const float lastTime, const int fragPart);

	void UpdateDurationBasedOnParams();

	void Serialize(Serialization::IArchive& ar);
};

//////////////////////////////////////////////////////////////////////////
class CProcClipTrack : public TSequencerTrack<CProcClipKey>
{
public:

	CProcClipTrack(SScopeContextData* pContext, EMannequinEditorMode editorMode);

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;

	virtual int               GetNumSecondarySelPts(int key) const override;
	virtual int               GetSecondarySelectionPt(int key, float timeMin, float timeMax) const override;
	virtual int               FindSecondarySelectionPt(int& key, float timeMin, float timeMax) const override;
	virtual void              SetSecondaryTime(int key, int idx, float time) override;
	virtual float             GetSecondaryTime(int key, int id) const override;
	virtual bool              CanMoveSecondarySelection(int key, int id) const override;

	virtual void              InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              ClearKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              OnKeyMenuOption(int menuOption, int keyID) override;

	virtual bool              CanAddKey(float time) const override;

	virtual bool              CanEditKey(int key) const override;
	virtual bool              CanMoveKey(int key) const override;

	virtual int               CreateKey(float time) override;

	virtual void              GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float             GetKeyDuration(const int key) const;
	virtual void              SerializeKey(CProcClipKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual Range             GetTrackDuration() const override;

protected:
	virtual void OnChangeCallback() override
	{
		if (m_pContext)
		{
			m_pContext->changeCount++;
		}
	}

private:
	SScopeContextData*   m_pContext;
	EMannequinEditorMode m_editorMode;
};

//////////////////////////////////////////////////////////////////////////
struct CTagKey : public CSequencerKey
{
	CTagKey()
		: tagState(TAG_STATE_EMPTY)
	{
	}
	TagState                   tagState;
	CryStackStringT<char, 128> desc;
};

//////////////////////////////////////////////////////////////////////////
class CTagTrack : public TSequencerTrack<CTagKey>
{
public:

	CTagTrack(const CTagDefinition& tagDefinition);

	virtual void              GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float             GetKeyDuration(const int key) const override;

	virtual void              InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              ClearKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              OnKeyMenuOption(int menuOption, int keyID) override;
	virtual void              SerializeKey(CTagKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual void              SetKey(int index, const CSequencerKey* _key) override;

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;

private:
	const CTagDefinition& m_tagDefinition;
};

//////////////////////////////////////////////////////////////////////////
struct CTransitionPropertyKey : public CSequencerKey
{
	enum EProp
	{
		eTP_Select,
		eTP_Start,
		eTP_Transition,
		eTP_Count,
	};

	CTransitionPropertyKey()
		: prop(eTP_Select)
		, refTime(0)
		, duration(0)
		, overrideProps(false)
		, toHistoryItem(HISTORY_ITEM_INVALID)
		, sharedId(0)
		, tranFlags(0)
	{
	}
	SBlendQueryResult blend;
	EProp             prop;
	float             refTime;
	float             duration;
	float             prevClipStart;
	float             prevClipDuration;
	float             overrideStartTime;
	float             overrideSelectTime;
	int               toHistoryItem;
	int               toFragIndex;
	int               sharedId;
	uint8             tranFlags;
	bool              overrideProps;
};

//////////////////////////////////////////////////////////////////////////
class CTransitionPropertyTrack : public TSequencerTrack<CTransitionPropertyKey>
{
public:

	CTransitionPropertyTrack(SScopeData& scopeData);

	virtual void                  GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float                 GetKeyDuration(const int key) const override;

	virtual bool                  CanEditKey(int key) const override;
	virtual bool                  CanMoveKey(int key) const override;
	virtual bool                  CanAddKey(float time) const override { return false; }
	virtual bool                  CanRemoveKey(int key) const override { return false; }

	virtual void                  InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void                  ClearKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void                  OnKeyMenuOption(int menuOption, int keyID) override;
	virtual void                  SerializeKey(CTransitionPropertyKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual void                  SetKey(int index, const CSequencerKey* _key) override;

	virtual ColorB                GetColor() const override;
	virtual const SKeyColour&     GetKeyColour(int key) const override;
	virtual const SKeyColour&     GetBlendColour(int key) const override;

	const int                     GetNumBlendsForKey(int index) const;
	const SFragmentBlend*         GetBlendForKey(int index) const;
	const SFragmentBlend*         GetAlternateBlendForKey(int index, int blendIdx) const;
	void                          UpdateBlendForKey(int index, SFragmentBlend& blend) const;
protected:
	const SScopeData& m_scopeData;
};

//////////////////////////////////////////////////////////////////////////
struct CParamKey : public CSequencerKey
{
	CParamKey()
		:
		historyItem(HISTORY_ITEM_INVALID),
		isLocation(false)
	{
		parameter.value.SetIdentity();
	}

	SMannParameter parameter;
	CString        name;
	uint32         historyItem;
	bool           isLocation;
};

//////////////////////////////////////////////////////////////////////////
class CParamTrack : public TSequencerTrack<CParamKey>
{
public:

	CParamTrack();

	virtual void              GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float             GetKeyDuration(const int key) const override;

	virtual void              InsertKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              ClearKeyMenuOptions(CMenu& menu, int keyID) override;
	virtual void              OnKeyMenuOption(int menuOption, int keyID) override;
	virtual void              SerializeKey(CParamKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;
};
//////////////////////////////////////////////////////////////////////////
struct CFragmentPropertyKey : public CSequencerKey
{
	float actionFinishedTiming = 0.0f; //<! Value of the EFragmentProperty::ActionFinishedTiming property.

	const char* GetDescription() const
	{
		static const char description[] = "Fragment Properties";
		return description;
	}
};

//////////////////////////////////////////////////////////////////////////
class CFragmentPropertyTrack : public TSequencerTrack<CFragmentPropertyKey>
{
public:

	virtual void              GetKeyInfo(int key, const char*& description, float& duration) const override;
	virtual float             GetKeyDuration(const int key) const override;

	virtual bool              CanMoveKey(int key) const { return false; }
	virtual bool              CanAddKey(float time) const override { return false; }
	virtual bool              CanRemoveKey(int key) const override { return false; }

	virtual void              SerializeKey(CFragmentPropertyKey& key, XmlNodeRef& keyNode, bool bLoading) override;

	virtual ColorB            GetColor() const override;
	virtual const SKeyColour& GetKeyColour(int key) const override;
	virtual const SKeyColour& GetBlendColour(int key) const override;

	virtual void              SetKey(int index, const CSequencerKey* key) override;

	virtual Range             GetTrackDuration() const override;
};
