// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"

#include "QT/QToolTabManager.h"
#include "Terrain/TerrainLayerUndoObject.h"
#include "Terrain/TerrainManager.h"
#include "TerrainTexturePainter.h"
#include "IEditorImpl.h"

#include <BoostPythonMacros.h>
#include <Controls/QuestionDialog.h>
#include <EditorFramework/Events.h>
#include <FileDialogs/SystemFileDialog.h>
#include <LevelEditor/LevelEditorSharedState.h>
#include <QtUtil.h>

namespace Private_TerrainLayerCommands
{

void PyExportTerrainLayers()
{
	if (0 >= GetIEditorImpl()->GetTerrainManager()->GetLayerCount())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "No layers exist. You have to create some Terrain Layers first.");
		CQuestionDialog::SWarning(QObject::tr("No Layers"), QObject::tr("No layers exist. You have to create some Terrain Layers first."));
		return;
	}

	const QDir dir(QtUtil::GetAppDataFolder());

	CSystemFileDialog::RunParams runParams;
	runParams.initialDir = dir.absolutePath();
	runParams.title = QObject::tr("Export Layers");
	runParams.extensionFilters << CExtensionFilter(QObject::tr("Layer Files (lay)"), "lay");

	const QString filePath = CSystemFileDialog::RunExportFile(runParams, nullptr);
	string path = filePath.toStdString().c_str();

	if (!filePath.isEmpty())
	{
		CryLog("Exporting layer settings to %s", path);

		CXmlArchive ar("LayerSettings");
		GetIEditorImpl()->GetTerrainManager()->SerializeSurfaceTypes(ar);
		GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(ar);
		ar.Save(path);
	}
}

static void PyImportTerrainLayers()
{
	const QDir dir(QtUtil::GetAppDataFolder());

	CSystemFileDialog::RunParams runParams;
	runParams.initialDir = dir.absolutePath();
	runParams.title = QObject::tr("Import Layers");
	runParams.extensionFilters << CExtensionFilter(QObject::tr("Layer Files (lay)"), "lay");

	QString fileName = CSystemFileDialog::RunImportFile(runParams, nullptr);
	string path = fileName.toStdString().c_str();

	if (fileName.isEmpty())
	{
		return;
	}

	CryLog("Importing layer settings from %s", path.GetString());

	CUndo undo("Import Texture Layers");
	GetIEditorImpl()->GetIUndoManager()->RecordUndo(new CTerrainLayersPropsUndoObject);

	CXmlArchive ar;
	if (ar.Load(path))
	{
		GetIEditorImpl()->GetTerrainManager()->SerializeSurfaceTypes(ar);
		GetIEditorImpl()->GetTerrainManager()->SerializeLayerSettings(ar);
	}

	GetIEditorImpl()->GetTerrainManager()->ReloadSurfaceTypes();
}

static void PyCreateNewTerrainLayerAt(int index)
{
	CUndo undo("New Terrain Layer");
	GetIEditorImpl()->GetIUndoManager()->RecordUndo(new CTerrainLayersPropsUndoObject);

	CLayer* pNewLayer = new CLayer;
	pNewLayer->SetLayerName("NewLayer");
	pNewLayer->LoadTexture("%ENGINE%/EngineAssets/Textures/white.dds");
	pNewLayer->AssignMaterial("%ENGINE%/EngineAssets/Materials/material_terrain_default");
	pNewLayer->GetOrRequestLayerId();

	GetIEditorImpl()->GetTerrainManager()->AddLayer(pNewLayer, index);
}

static void PyCreateNewTerrainLayer()
{
	PyCreateNewTerrainLayerAt(-1);
}

static void PyDeleteTerrainLayer()
{
	GetIEditorImpl()->GetTerrainManager()->RemoveSelectedLayer();
}

static void PyDuplicateTerrainLayer()
{
	GetIEditorImpl()->GetTerrainManager()->DuplicateSelectedLayer();
}

static void MoveTerrainLayer(int src, int dest)
{
	GetIEditorImpl()->GetTerrainManager()->MoveLayer(src, dest);
}

static void PyMoveTerrainLayerToTop()
{
	const int index = GetIEditorImpl()->GetTerrainManager()->GetSelectedLayerIndex();
	if (index > 0)
	{
		MoveTerrainLayer(index, 0);
	}
}

static void PyMoveTerrainLayerUp()
{
	const int index = GetIEditorImpl()->GetTerrainManager()->GetSelectedLayerIndex();
	if (index > 0)
	{
		MoveTerrainLayer(index, index - 1);
	}
}

static void PyMoveTerrainLayerDown()
{
	const int index = GetIEditorImpl()->GetTerrainManager()->GetSelectedLayerIndex();
	const int count = GetIEditorImpl()->GetTerrainManager()->GetLayerCount();
	if (count > 1 && index < count - 1)
	{
		MoveTerrainLayer(index, index + 2);
	}
}

static void PyMoveTerrainLayerToBottom()
{
	const int index = GetIEditorImpl()->GetTerrainManager()->GetSelectedLayerIndex();
	const int count = GetIEditorImpl()->GetTerrainManager()->GetLayerCount();
	if (count > 1 && index < count - 1)
	{
		MoveTerrainLayer(index, -1);
	}
}

static void PyFloodTerrainLayer()
{
	CLevelEditorSharedState* pLevelEditor = GetIEditorImpl()->GetLevelEditorSharedState();
	CEditTool* pTool = pLevelEditor->GetEditTool();
	if (!pTool || !pTool->IsKindOf(RUNTIME_CLASS(CTerrainTexturePainter)))
	{
		pTool = new CTerrainTexturePainter();
		pLevelEditor->SetEditTool(pTool);
	}

	CTerrainTexturePainter* pPainterTool = static_cast<CTerrainTexturePainter*>(pTool);
	pPainterTool->Action_StopUndo();
	pPainterTool->Action_Flood();
	pPainterTool->Action_StopUndo();
}

} // namespace Private_TerrainLayerCommands

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyExportTerrainLayers, terrain, export_layers, "Export terrain layers")
REGISTER_EDITOR_COMMAND_TEXT(terrain, export_layers, "Export Layers...")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyImportTerrainLayers, terrain, import_layers, "Import terrain layers")
REGISTER_EDITOR_COMMAND_TEXT(terrain, import_layers, "Import Layers...")

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainLayerCommands::PyCreateNewTerrainLayerAt, terrain, create_layer_at, CCommandDescription("Create a new layer").Param("index", "layer index, -1 = last"))

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyCreateNewTerrainLayer, terrain, create_layer, "Create a new layer")
REGISTER_EDITOR_COMMAND_TEXT(terrain, create_layer, "Create Layer")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyDeleteTerrainLayer, terrain, delete_layer, "Delete selected layer")
REGISTER_EDITOR_COMMAND_TEXT(terrain, delete_layer, "Delete Layer")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyDuplicateTerrainLayer, terrain, duplicate_layer, "Duplicate selected layer")
REGISTER_EDITOR_COMMAND_TEXT(terrain, duplicate_layer, "Duplicate Layer")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyMoveTerrainLayerToTop, terrain, move_layer_to_top, "Move selected layer to Top")
REGISTER_EDITOR_COMMAND_TEXT(terrain, move_layer_to_top, "Move to Top")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyMoveTerrainLayerUp, terrain, move_layer_up, "Move selected layer up")
REGISTER_EDITOR_COMMAND_TEXT(terrain, move_layer_up, "Move Up")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyMoveTerrainLayerDown, terrain, move_layer_down, "Move selected layer down")
REGISTER_EDITOR_COMMAND_TEXT(terrain, move_layer_down, "Move Down")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyMoveTerrainLayerToBottom, terrain, move_layer_to_bottom, "Move selected layer to buttom")
REGISTER_EDITOR_COMMAND_TEXT(terrain, move_layer_to_bottom, "Move to Bottom")

REGISTER_PYTHON_COMMAND(Private_TerrainLayerCommands::PyFloodTerrainLayer, terrain, flood_layer, "Floods the selected layer over the all terrain")
REGISTER_EDITOR_COMMAND_TEXT(terrain, flood_layer, "Flood Layer")

namespace Private_TerrainCommands
{

static void PyRefineTerrainTiles()
{
	auto answer = CQuestionDialog::SQuestion(QObject::tr("Error"),
	                                         QObject::tr("Refine TerrainTexture?\r\n"
	                                                     "(all terrain texture tiles become split in 4 parts so a tile with 2048x2048\r\n"
	                                                     "no longer limits the resolution) You need to save afterwards!"));

	if (QDialogButtonBox::StandardButton::Yes != answer)
	{
		return;
	}

	if (!GetIEditorImpl()->GetTerrainManager()->GetRGBLayer()->RefineTiles())
	{
		CQuestionDialog::SCritical(QObject::tr(""), QObject::tr("TerrainTexture refine failed (make sure current data is saved)"));
	}
	else
	{
		CQuestionDialog::SWarning(QObject::tr(""), QObject::tr("Successfully refined TerrainTexture - Save is now required!"));
	}
}

} // namespace Private_TerrainCommands

REGISTER_PYTHON_COMMAND(Private_TerrainCommands::PyRefineTerrainTiles, terrain, refine_tiles, "Split the tiles into smaller tiles")
REGISTER_EDITOR_COMMAND_TEXT(terrain, refine_tiles, "Refine Terrain Texture Tiles")

namespace Private_TerrainToolsCommands
{
void EnsureTerrainEditorActive()
{
	CTabPaneManager* pPaneManager = CTabPaneManager::GetInstance();
	if (pPaneManager)
	{
		pPaneManager->OpenOrCreatePane("Terrain Editor");
	}
}

void Flatten()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.flatten_tool").SendToKeyboardFocus();
}

void Smooth()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.smooth_tool").SendToKeyboardFocus();
}

void RaiseLower()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.raise_lower_tool").SendToKeyboardFocus();
}

void Duplicate()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.duplicate_tool").SendToKeyboardFocus();
}

void MakeHoles()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.make_holes_tool").SendToKeyboardFocus();
}

void FillHoles()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.fill_holes_tool").SendToKeyboardFocus();
}

void PaintLayer()
{
	EnsureTerrainEditorActive();
	CommandEvent("terrain.paint_texture_tool").SendToKeyboardFocus();
}
}

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::Flatten, terrain, flatten_tool,
                                   CCommandDescription("Switch to terrain flatten tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, flatten_tool, "Terrain Flatten Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::Smooth, terrain, smooth_tool,
                                   CCommandDescription("Switch to terrain smooth tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, smooth_tool, "Terrain Smooth Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::RaiseLower, terrain, raise_lower_tool,
                                   CCommandDescription("Switch to terrain raise/lower tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, raise_lower_tool, "Terrain Raise/Lower Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::Duplicate, terrain, duplicate_tool,
                                   CCommandDescription("Switch to terrain duplicate tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, duplicate_tool, "Terrain Duplicate Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::MakeHoles, terrain, make_holes_tool,
                                   CCommandDescription("Switch to terrain make holes tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, make_holes_tool, "Terrain Make Holes Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::FillHoles, terrain, fill_holes_tool,
                                   CCommandDescription("Switch to terrain fill holes tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, fill_holes_tool, "Terrain Fill Holes Tool", "", "", false)

REGISTER_EDITOR_AND_SCRIPT_COMMAND(Private_TerrainToolsCommands::PaintLayer, terrain, paint_texture_tool,
                                   CCommandDescription("Switch to terrain texture painting tool"))
REGISTER_EDITOR_UI_COMMAND_DESC(terrain, paint_texture_tool, "Terrain Texture Paint Tool", "", "", false)
REGISTER_COMMAND_REMAPPING(edit_mode, terrain_painter, terrain, paint_texture_tool)
