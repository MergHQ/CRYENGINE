// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryRenderer/IFlares.h>
#include <Util/Variable.h>

class CLensFlareElementTree;
class CLensFlareView;

class CLensFlareElement : public _i_reference_target_t
{
public:

	typedef _smart_ptr<CLensFlareElement>    LensFlareElementPtr;
	typedef std::vector<LensFlareElementPtr> LensFlareElementList;

	CLensFlareElement();
	virtual ~CLensFlareElement();

	CVarBlock* GetProperties() const
	{
		return m_vars;
	}

	void       OnInternalVariableChange(IVariable* pVar);

	bool       IsEnable();
	void       SetEnable(bool bEnable);
	EFlareType GetOpticsType();

	bool       GetName(string& outName) const
	{
		IOpticsElementBasePtr pOptics = GetOpticsElement();
		if (pOptics == NULL)
			return false;
		outName = pOptics->GetName();
		return true;
	}

	bool                  GetShortName(string& outName) const;
	IOpticsElementBasePtr GetOpticsElement() const
	{
		return m_pOpticsElement;
	}
	void SetOpticsElement(IOpticsElementBasePtr pOptics)
	{
		m_pOpticsElement = pOptics;
		UpdateProperty(m_pOpticsElement);
	}

private:

	void                   UpdateLights();
	void                   UpdateProperty(IOpticsElementBasePtr pOptics);

	CLensFlareElementTree* GetLensFlareTree() const;
	CLensFlareView*        GetLensFlareView() const;

private:

	IOpticsElementBasePtr m_pOpticsElement;
	CVarBlockPtr          m_vars;
};
