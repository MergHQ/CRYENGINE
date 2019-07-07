// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditToolPanel.h"

class CEditTool;
class CLayer;

struct STexturePainterSerializer
{
public:
	STexturePainterSerializer()
		: pEditTool(nullptr)
		, pLayer(nullptr)
		, textureLayerUpdate(false)
	{}

	void YASLI_SERIALIZE_METHOD(Serialization::IArchive& ar);

	CEditTool* pEditTool;
	CLayer*    pLayer;
	bool       textureLayerUpdate;
};

class QTerrainLayerButtons : public QWidget
{
public:
	QTerrainLayerButtons(QWidget* parent = nullptr);

private:
	void AddTool(CRuntimeClass* pRuntimeClass, const char* text);
};

class QTerrainLayerPanel : public QEditToolPanel
{
public:
	QTerrainLayerPanel(QWidget* parent = nullptr);
	virtual ~QTerrainLayerPanel() override;

private:
	virtual bool CanEditTool(CEditTool* pTool) override;
	virtual void SetTool(CEditTool* pTool) override;
	void         SetLayer(CLayer* pLayer);
	void         OnLayerAboutToDelete(CLayer* pLayer);
	void         LayerChanged(CLayer* pLayer);
	void         BrushChanged(CEditTool* pTool);
	void         AttachProperties();

	void         OnBeginUndo();
	void         OnEndUndo(bool acceptUndo);

	STexturePainterSerializer m_layerSerializer;
};
