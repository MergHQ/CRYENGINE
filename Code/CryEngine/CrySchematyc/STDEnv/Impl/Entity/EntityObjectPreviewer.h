// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
class CEntityObjectPreviewer : public IObjectPreviewer
{
public:

	// IObjectPreviewer
	virtual void     SerializeProperties(Serialization::IArchive& archive) override;
	virtual ObjectId CreateObject(const SGUID& classGUID) const override;
	virtual ObjectId ResetObject(ObjectId objectId) const override;
	virtual void     DestroyObject(ObjectId objectId) const override;
	virtual Sphere   GetObjectBounds(ObjectId objectId) const override;
	virtual void     RenderObject(const IObject& object, const SRendParams& params, const SRenderingPassInfo& passInfo) const override;
	// ~IObjectPreviewer
};
} // Schematyc
