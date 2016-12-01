#include "stdafx.h"
#include "AbnormalsMod.h"
#include "AbnormalsModClassDesc.h"
#include "AbnormalsModParamBlock.h"
#include "AbnormalsModDlgProcs.h"
#include "AbnormalsModLocalData.h"
#include "AbnormalsModActions.h"
#include "AbnormalsModRestoreObj.h"
#include "AppDataChunk.h"
#include <modstack.h>
#include <math.h>
#include "Max.h"
#include "maxscript\UI\rollouts.h"
#include "MNChamferData10.h"

#define MAP_NONE -10
#define DEFAULT_CHANNEL 3 // Default to UV channel 3
#define MIN_UV_CHANNEL 2 // Minimum UV channel to save to is 2, so it won't conflict with primary UVs on channel 1.

AbnormalsMod::AbnormalsMod() : SubSelectionPolyMod(SEL_FACE | SEL_EDGE | SEL_ELEMENT)
{
	ip = NULL;
	generalGroupsDlgProc = NULL;
	editGroupsDlgProc = NULL;
	showingGroupColors = false;
	abnormalsActionCallback = NULL;
	data = NULL;
	AbnormalsModClassDesc::GetInstance()->MakeAutoParamBlocks(this);

	// Create materials used for rendering the overlay colors.
	overlayMaterials.append(Material());
	overlayMaterials[0].Kd = DEFAULT_GROUP1_COLOR;
	overlayMaterials.append(Material());
	overlayMaterials[1].Kd = DEFAULT_GROUP2_COLOR;
	overlayMaterials.append(Material());
	overlayMaterials[2].Kd = DEFAULT_GROUP3_COLOR;
	overlayMaterials.append(Material());
	overlayMaterials[3].Kd = DEFAULT_GROUP4_COLOR;

	for (int i = 0; i < ABNORMALS_NUM_GROUPS; i++)
	{
		overlayMaterials[i].opacity = 1.0f;
		overlayMaterials[i].selfIllum = 0.2f;
		overlayMaterials[i].Ka = overlayMaterials[i].Kd;
		overlayMaterials[i].shinStrength = 0.7f;
		overlayMaterials[i].shininess = 8.0f;
	}

	selectionMaterial = Material();
	selectionMaterial.Kd = COLVAL_8BIT(208, 51, 51);
	selectionMaterial.opacity = 0.5f;
}

AbnormalsMod::~AbnormalsMod() {}

void AbnormalsMod::ApplyGroupsToMesh(MNMesh* mesh, int channel)
{
	// If groups not modified, nothing to do here.
	if (!data->GetGroupsModified())
		return;

	if (channel >= mesh->MNum()) // If the channel is greater than the number of maps we have.
	{
		// Expand the number of maps to create up to the channel we need.
		mesh->SetMapNum(channel + 1);
	}

	MNMap* map = mesh->M(channel);

	// Initialize map if not yet initialized
	if (map->numf != mesh->numf)
	{
		InitializeMap(channel,mesh,true);
	}

	for (int g = 0; g < ABNORMALS_NUM_GROUPS; g++)
	{
		// Clear this flag before we start, just incase it's been set from something lower in the stack.
		mesh->ClearFFlags(MN_EDITPOLY_OP_SELECT);

		BitArray groupFaces = data->GetGroupFaces(g);

		for (int f = 0; f < mesh->numf; f++)
		{
			if (f < groupFaces.GetSize())
			{
				// Set correct flags on faces
				if (groupFaces[f])
					mesh->F(f)->SetFlag(MN_EDITPOLY_OP_SELECT, true);
			}
		}

		UVVert vertValue = GetGroupValue(g);

		// Assign color to faces we set flags on earlier.
		mesh->SetFaceColor(vertValue, channel, MN_EDITPOLY_OP_SELECT);

		// Clear all of this flag, so the last applied group won't affect anything down the line.
		mesh->ClearFFlags(MN_EDITPOLY_OP_SELECT);
		// Need to call after modifying vertex colors
		mesh->InvalidateHardwareMesh();
	}
}

void AbnormalsMod::ConvertFromLegacyColors(MNMesh* mesh, int channel)
{
	for (int g = 0; g < ABNORMALS_NUM_GROUPS; g++)
	{
		// Clear this flag before we start, just incase it's been set from something lower in the stack.
		mesh->ClearFFlags(MN_EDITPOLY_OP_SELECT);

		Point3 legacyColor = GetGroupValue(g, true);
		Point3 newValue = GetGroupValue(g, false);

		BitArray groupFaces = GetFacesOfColor(mesh, legacyColor, channel, COLOR_VALUE_TOLERANCE);

		for (int f = 0; f < mesh->numf; f++)
		{
			if (f < groupFaces.GetSize())
			{
				// Set correct flags on faces
				if (groupFaces[f])
					mesh->F(f)->SetFlag(MN_EDITPOLY_OP_SELECT, true);
			}
		}
		// Assign color to faces we set flags on earlier.
		mesh->SetFaceColor(newValue, channel, MN_EDITPOLY_OP_SELECT);

		// Clear all of this flag, so the last applied group won't affect anything down the line.
		mesh->ClearFFlags(MN_EDITPOLY_OP_SELECT);
	}
	// Need to call after modifying vertex colors
	mesh->InvalidateHardwareMesh();
}

#define CLASS_ID_AbnormalChannelInfoXTC Class_ID(0x75c0a670, 0x4e09f657)

class AbnormalChannelInfoXTC : public XTCObject
{
public:
	AbnormalChannelInfoXTC() { channel = 0; }
	AbnormalChannelInfoXTC(int aChannel) { channel = aChannel; }
	Class_ID ExtensionID() { return CLASS_ID_AbnormalChannelInfoXTC; }
	XTCObject *Clone() { return new AbnormalChannelInfoXTC(channel); }
	ChannelMask DependsOn() { return TOPO_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL; }
	ChannelMask ChannelsChanged() { return 0; }
	void PreChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline) {}
	void PostChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline) {}
	void DeleteThis() { delete this; }


	int channel;
};

bool AnyLegacyColorsExist(MNMesh* mesh)
{
	if (mesh->M(0) != NULL && mesh->M(0)->f != NULL && mesh->M(0)->v != NULL &&
		(GetFacesOfColor(mesh, DEFAULT_GROUP4_COLOR, 0, COLOR_VALUE_TOLERANCE).AnyBitSet() ||
		GetFacesOfColor(mesh, DEFAULT_GROUP1_COLOR, 0, COLOR_VALUE_TOLERANCE).AnyBitSet() ||
		GetFacesOfColor(mesh, DEFAULT_GROUP2_COLOR, 0, COLOR_VALUE_TOLERANCE).AnyBitSet() ||
		GetFacesOfColor(mesh, DEFAULT_GROUP3_COLOR, 0, COLOR_VALUE_TOLERANCE).AnyBitSet()))
		return true;
	else
		return false;
}

INode* GetNodeFromObject(BaseObject* obj) {
	ULONG handle = 0;
	obj->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
	INode *node = GetCOREInterface()->GetINodeByHandle(handle);
	return node;
}

int AbnormalsMod::GetAutoLoadChannel(ObjectState *os)
{
	if (!os->obj->IsSubClassOf(polyObjectClassID))
		return 0; // Invalid object type.

	loadLegacyColors = false;

	PolyObject *pobj = (PolyObject*)os->obj;

	// Check for XTC object created from an abnormals modifier lower in the stack that has the channel
	AbnormalChannelInfoXTC* XTC = NULL;
	for (int i = pobj->NumXTCObjects() - 1; i >= 0; i--)
	{
		if (pobj->GetXTCObject(i)->ExtensionID() == CLASS_ID_AbnormalChannelInfoXTC)
		{
			XTC = (AbnormalChannelInfoXTC*)pobj->GetXTCObject(i);
			// Return the first on we find (which will be the latest added, since we're iterating backwards).
			return XTC->channel;
		}
	}

	// Check for an app data chunk on our node that has the channel.
	INode* node = FindNodeFromRefTarget(this);
	AppDataChunk* chunk = node->GetAppDataChunk(node->ClassID(), node->SuperClassID(), ID_ABNORMALS_MOD_CHANNEL_DATA);
	
	const wchar_t* name = node->GetName();

	if (chunk != NULL)
	{
		int chunkData = *(int*)chunk->data;

		// It seems in some cases, an invalid map number can be saved into the channel data.
		// Ensure the data is within the valid range before returning.
		if (chunkData < -NUM_HIDDENMAPS || chunkData > 99)
			return MAP_NONE;
		else
			return chunkData;
	}

	// Check to see if maybe the object is using legacy colors.
	if (AnyLegacyColorsExist(&pobj->GetMesh()))
	{
		loadLegacyColors = true;
		return 0; // 0 is color channel
	}

	// Could not find any previously authored groups, so we're working from a fresh model.
	return MAP_NONE;
}

void AbnormalsMod::UpdateMemberVarsFromParamBlock(TimeValue t, ObjectState *os)
{
	// NOTE: Current value MUST be checked,  before calling SetValue, and only call SetValue if the current value is different.
	// Otherwise, if this modifier is applied to multiple instances, they will always be marked invalid, and recomputed every frame.

	// TODO: actually make use of interval.
	Interval ivalid = FOREVER; // Start with valid forever, then widdle it down as we access pblock values.
	
	pblock2->GetValue(eAbnormalsModParam::show_normals, t, showNormals, ivalid);
	pblock2->GetValue(eAbnormalsModParam::show_normals_size, t, showNormalSize, ivalid);


	pblock2->GetValue(eAbnormalsModParam::overlay_opacity, t, overlayOpacity, ivalid);
	for (int i = 0; i < ABNORMALS_NUM_GROUPS; i++)
		overlayMaterials[i].opacity = overlayOpacity;

	pblock2->GetValue(eAbnormalsModParam::clear_smoothing, t, clearSmoothing, ivalid);

	pblock2->GetValue(eAbnormalsModParam::auto_chamfer, t, autoChamfer, ivalid);
	pblock2->GetValue(eAbnormalsModParam::auto_chamfer_source, t, (int&)chamferSource, ivalid);
	
	if (chamferSource == AbnormalsMod::SelectedEdges) // Only standard chamfer is supported when using selected edges.
	{
		chamferMethod = AbnormalsMod::Standard;

		AutoChamferMethod selectedMethod;
		pblock2->GetValue(eAbnormalsModParam::auto_chamfer_method, t, (int&)selectedMethod, ivalid);
		if (selectedMethod != chamferMethod)
			pblock2->SetValue(eAbnormalsModParam::auto_chamfer_method, t, (int&)chamferMethod);
	}
	else
		pblock2->GetValue(eAbnormalsModParam::auto_chamfer_method, t, (int&)chamferMethod, ivalid);

	pblock2->GetValue(eAbnormalsModParam::auto_chamfer_radius, t, chamferRadius, ivalid);
	pblock2->GetValue(eAbnormalsModParam::auto_chamfer_segments, t, chamferSegments, ivalid);

	int loadFromChannelSelection;
	pblock2->GetValue(eAbnormalsModParam::load_groups_from, t, loadFromChannelSelection, ivalid);

	int loadFromUVSelection;
	pblock2->GetValue(eAbnormalsModParam::load_groups_uv_channel, t, loadFromUVSelection, ivalid);
	pblock2->GetValue(eAbnormalsModParam::load_auto, t, autoLoadEnabled, ivalid);
	
	if (!autoLoadEnabled)
	{
		pblock2->GetValue(eAbnormalsModParam::load_legacy_colors, t, loadLegacyColors, ivalid);
		loadFromChannel = SelectionToMapChannel(loadFromChannelSelection, loadFromUVSelection);
	}
	else // Figure out what channel to use based on where we have abnormal groups stored in
	{
		loadFromChannel = GetAutoLoadChannel(os);
		BOOL currentLoadFromUVSetting;
		int autoLoadFromChannelSelection = MapChannelToSelection(loadFromChannel, currentLoadFromUVSetting);
		if (autoLoadFromChannelSelection != loadFromChannelSelection)
			pblock2->SetValue(eAbnormalsModParam::load_groups_from, t, autoLoadFromChannelSelection);

		if (loadFromChannel > 0 && loadFromUVSelection != currentLoadFromUVSetting) // UV specified
			pblock2->SetValue(eAbnormalsModParam::load_groups_uv_channel, t, currentLoadFromUVSetting);

		BOOL loadLegacyColorsSelection;
		pblock2->GetValue(eAbnormalsModParam::load_legacy_colors, t, loadLegacyColorsSelection, ivalid);
		if (loadLegacyColorsSelection != loadLegacyColorsSelection)
			pblock2->SetValue(eAbnormalsModParam::load_legacy_colors, t, loadLegacyColors);
	}

	int saveToChannelSelection;
	pblock2->GetValue(eAbnormalsModParam::save_groups_to, t, saveToChannelSelection, ivalid);
	int saveToUVSelection;
	pblock2->GetValue(eAbnormalsModParam::save_groups_uv_channel, t, saveToUVSelection, ivalid);

	pblock2->GetValue(eAbnormalsModParam::save_auto, t, autoSaveEnabled, ivalid);

	if (!autoSaveEnabled)
	{
		saveToChannel = SelectionToMapChannel(saveToChannelSelection, saveToUVSelection);
	}
	else // If auto, save back into same channel as we're loading from.
	{
		if (loadFromChannel == MAP_NONE || (loadFromChannel == 0 && loadLegacyColors))
			saveToChannel = DEFAULT_CHANNEL;
		else
			saveToChannel = loadFromChannel;

		// Update paramblock to reflect what we're actually using.
		BOOL currentSaveToUVSetting;
		int currentSaveToChannelSetting = MapChannelToSelection(saveToChannel, currentSaveToUVSetting);

		if (saveToChannelSelection != currentSaveToChannelSetting)
		pblock2->SetValue(eAbnormalsModParam::save_groups_to, t, currentSaveToChannelSetting);

		if (saveToChannel > 0 && currentSaveToUVSetting != saveToUVSelection) // UV specified
			pblock2->SetValue(eAbnormalsModParam::save_groups_uv_channel, t, currentSaveToUVSetting);
	}
}

void SetButtonEnabled(HWND hWnd, UINT32 id, bool enabled)
{
	ICustButton* but = GetICustButton(GetDlgItem(hWnd, id));
	if (but)
	{
		but->Enable(enabled);
		ReleaseICustButton(but);
	}
}

void AbnormalsMod::UpdateControls(HWND hWnd, eAbnormalsModRollout::Type rollout)
{
	switch (rollout)
	{
	case eAbnormalsModRollout::channels:
	{		
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_NONE), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_ILLUM_RAD), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_COLOR_RAD), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_LEGACY_COLORS), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_OPAC_RAD), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_UV_RAD), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_UV_CHANNEL_NUM), !autoLoadEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_LOAD_UV_CHANNEL_NUM_SPN), !autoLoadEnabled);

		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_ILLUM_RAD), !autoSaveEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_COLOR_RAD), !autoSaveEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_OPAC_RAD), !autoSaveEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_UV_RAD), !autoSaveEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_UV_CHANNEL_NUM), !autoSaveEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_SAVE_UV_CHANNEL_NUM_SPN), !autoSaveEnabled);
		break;
	}
	case eAbnormalsModRollout::auto_chamfer:
	{
		EnableWindow(GetDlgItem(hWnd, IDC_AUTO_CHAMFER_STANDARD), autoChamfer);
		EnableWindow(GetDlgItem(hWnd, IDC_AUTO_CHAMFER_QUAD), autoChamfer && chamferSource != AbnormalsMod::SelectedEdges);
		EnableWindow(GetDlgItem(hWnd, IDC_CHAMFER_RADIUS), autoChamfer);
		EnableWindow(GetDlgItem(hWnd, IDC_CHAMFER_RADIUS_SPN), autoChamfer);
		EnableWindow(GetDlgItem(hWnd, IDC_CHAMFER_SEGMENTS), autoChamfer);
		EnableWindow(GetDlgItem(hWnd, IDC_CHAMFER_SEGMENTS_SPN), autoChamfer);
		break;
	}
	case eAbnormalsModRollout::general:
	{
		EnableWindow(GetDlgItem(hWnd, IDC_SHOW_NORMALS_SIZE), showNormals);
		EnableWindow(GetDlgItem(hWnd, IDC_SHOW_NORMALS_SIZE_SPN), showNormals);

		// Update group edit buttons (1,2,3,clear) enabled states
		SubObjectType* currentSubSelection = GetCurrentSubSelection();
		// In Face or Element subobject mode
		const bool buttonsEnabled = GetCurrentSubSelectionType() == SubSelectionPolyMod::SEL_ELEMENT || GetCurrentSubSelectionType() == SubSelectionPolyMod::SEL_FACE;

		SetButtonEnabled(hWnd, IDC_ASSIGNGROUP_1, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_ASSIGNGROUP_2, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_ASSIGNGROUP_3, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_ASSIGNGROUP_4, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_SELECTGROUP_1, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_SELECTGROUP_2, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_SELECTGROUP_3, buttonsEnabled);
		SetButtonEnabled(hWnd, IDC_SELECTGROUP_4, buttonsEnabled);

		break;
	}
	}
}

void AbnormalsMod::NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index)
{
	// Iterate over all entries of the modifier stack.
	for (int m = 0; m < derObj->NumModifiers(); m++)
	{
		// Get current modifier.
		Modifier* ModifierPtr = derObj->GetModifier(m);

		// Is this abnormals modifier?
		if (ModifierPtr->ClassID() == ABNORMALS_MOD_CLASS_ID)
		{
			// If we are the topmost abnormals modifier, then update the app data chunk with our settings.
			if (ModifierPtr == this)
			{
				// Write our "save to" channel to an app data chunk on the node.
				AppDataChunk* chunk = node->GetAppDataChunk(node->ClassID(), node->SuperClassID(), ID_ABNORMALS_MOD_CHANNEL_DATA);				
				if (chunk != NULL)
				{
					// Update data stored in existing chunk
					int* saveToChannelData = (int*)chunk->data;
					*saveToChannelData = saveToChannel;
				}
				else
				{
					// Add new app data chunk with the new channel.
					int* saveToChannelData = (int*)MAX_malloc(sizeof(saveToChannel));
					*saveToChannelData = saveToChannel;
					node->AddAppDataChunk(node->ClassID(), node->SuperClassID(), ID_ABNORMALS_MOD_CHANNEL_DATA, sizeof(saveToChannel), saveToChannelData);
				}
			}
			// Return, since we've found the topmost abnormals modifier. If it wasn't us, then we don't want to save the appdata.
			return;
		}
	}
}

void AbnormalsMod::UpdateDisplayCache(MNMesh* mesh)
{
	MNNormalSpec* normSpec = NULL;
	if (mesh)
		normSpec = mesh->GetSpecifiedNormals();

	// Reserve the maimum number of normals and triangles we could possibly need.
	int maxNumTris = mesh->numv * 2;
	int maxNumNormals = maxNumTris * 3;

	for (int i = 0; i < NUM_ELEMENTS(overlayTris); i++)
		overlayTris[i].setLengthUsed(maxNumTris);

	for (int i = 0; i < NUM_ELEMENTS(debugNormals); i++)
		debugNormals[i].setLengthUsed(maxNumNormals);

	int overlayTrisCount[NUM_ELEMENTS(overlayTris)] = { 0 };
	int debugNormalsCount[NUM_ELEMENTS(debugNormals)] = { 0 };

	overlayEdges.removeAll();
	overlaySelectedEdges.removeAll();

	if (mesh && normSpec)
	{
		BitArray group1Faces = GetFacesOfColor(mesh, Color(GROUP_1_VALUE, GROUP_1_VALUE, GROUP_1_VALUE), saveToChannel, COLOR_VALUE_TOLERANCE);
		BitArray group2Faces = GetFacesOfColor(mesh, Color(GROUP_2_VALUE, GROUP_2_VALUE, GROUP_2_VALUE), saveToChannel, COLOR_VALUE_TOLERANCE);
		BitArray group3Faces = GetFacesOfColor(mesh, Color(GROUP_3_VALUE, GROUP_3_VALUE, GROUP_3_VALUE), saveToChannel, COLOR_VALUE_TOLERANCE);

		BitArray faceSelection;
		mesh->getFaceSel(faceSelection);

		for (DWORD f = 0; f < mesh->numf; f++)
		{
			MNFace* face = mesh->F(f);
			int group = 0;
			if (group1Faces[f])
				group = 0;
			else if (group2Faces[f])
				group = 1;
			else if (group3Faces[f])
				group = 2;
			else
				group = 3; // Clear

			for (int corner = 0; corner < face->deg; corner++)
			{
				const Point3 position = mesh->V(face->vtx[corner])->p;
				const Point3 normal = normSpec->GetNormal(f, corner);
				
				debugNormals[group][debugNormalsCount[group]] = DebugNormal(position, position + (normal * showNormalSize));
				debugNormalsCount[group] ++;
			}

			Tab<int> tris;
			face->GetTriangles(tris);

			for (int t = 0; t < face->TriNum(); t++)
			{
				const int v1 = face->vtx[tris[t * 3]];
				const int v2 = face->vtx[tris[t * 3 + 1]];
				const int v3 = face->vtx[tris[t * 3 + 2]];
				const Point3 n1 = normSpec->GetNormal(f, tris[t * 3]);
				const Point3 n2 = normSpec->GetNormal(f, tris[t * 3 + 1]);
				const Point3 n3 = normSpec->GetNormal(f, tris[t * 3 + 2]);

				overlayTris[group][overlayTrisCount[group]] = OverlayTri(mesh->V(v1)->p, mesh->V(v2)->p, mesh->V(v3)->p, n1, n2, n3);
				overlayTrisCount[group] ++;

				// If the face is selected, add it to the selection debug list.
				if (faceSelection[f])
				{
					overlayTris[NUM_ELEMENTS(overlayTris) - 1][overlayTrisCount[NUM_ELEMENTS(overlayTris) - 1]] = OverlayTri(mesh->V(v1)->p, mesh->V(v2)->p, mesh->V(v3)->p, n1, n2, n3);
					overlayTrisCount[NUM_ELEMENTS(overlayTris) - 1] ++;
				}
			}
		}

		// Set how many we've actually used.
		for (int i = 0; i < NUM_ELEMENTS(debugNormals); i++)
			debugNormals[i].setLengthUsed(debugNormalsCount[i]);

		for (int i = 0; i < NUM_ELEMENTS(overlayTris); i++)
			overlayTris[i].setLengthUsed(overlayTrisCount[i]);

		const bool edgeSubObjectMode = GetCurrentSubSelectionType() == SEL_EDGE || GetCurrentSubSelectionType() == SEL_BORDER;
		const bool faceSubObjectMode = GetCurrentSubSelectionType() == SEL_FACE || GetCurrentSubSelectionType() == SEL_ELEMENT;

		BitArray edgeSelection;
		mesh->getEdgeSel(edgeSelection);
		for (DWORD e = 0; e < mesh->nume; e++)
		{
			bool edgeSelected = false;

			if (edgeSubObjectMode)
			{
				edgeSelected = edgeSelection[e];
			}
			else if (faceSubObjectMode)
			{
				// Either face selected
				edgeSelected = faceSelection[mesh->e[e].f1] || (mesh->e[e].f2 != -1 && faceSelection[mesh->e[e].f2]);
			}
			
			if (edgeSelected)
			{
				overlaySelectedEdges.append(mesh->v[mesh->e[e].v1].p);
				overlaySelectedEdges.append(mesh->v[mesh->e[e].v2].p);
			}
			else
			{
				overlayEdges.append(mesh->v[mesh->e[e].v1].p);
				overlayEdges.append(mesh->v[mesh->e[e].v2].p);
			}
			
		}
	}
}

int AbnormalsMod::SelectionToMapChannel(int channelType, int uvChannel)
{
	if (uvChannel < MIN_UV_CHANNEL || uvChannel > 99)
		uvChannel = MIN_UV_CHANNEL;

	switch (channelType)
	{
	case 0: // Illumination
		return MAP_SHADING;
	case 1: // Color
		return 0;
	case 2: // Opacity
		return MAP_ALPHA;
	case 3: // UV
		return uvChannel;
	case 4:
		return MAP_NONE;
	default:
		return DEFAULT_CHANNEL;
	}
}

int AbnormalsMod::MapChannelToSelection(int channel, int& outUVChannel)
{
	switch (channel)
	{
	case MAP_SHADING:
		return 0; // Illumination
	case MAP_ALPHA:
		return 2; // Opacity
	case MAP_NONE:
		return 4; // None
	case 0:
		return 1; // Color
	default:
		outUVChannel = min(max(1, channel), 99);
		return 3; // UV
	}
}

UVVert AbnormalsMod::GetGroupValue(int group, bool legacyColors /*= false*/)
{
	// Support getting the old group colors that were applied from the old script.
	if (legacyColors)
	{
		switch (group)
		{
		case 0:
			return UVVert(DEFAULT_GROUP1_COLOR.r, DEFAULT_GROUP1_COLOR.g, DEFAULT_GROUP1_COLOR.b);
		case 1:
			return UVVert(DEFAULT_GROUP2_COLOR.r, DEFAULT_GROUP2_COLOR.g, DEFAULT_GROUP2_COLOR.b);
		case 2:
			return UVVert(DEFAULT_GROUP3_COLOR.r, DEFAULT_GROUP3_COLOR.g, DEFAULT_GROUP3_COLOR.b);
		case 3:
		default:
			return UVVert(DEFAULT_GROUP4_COLOR.r, DEFAULT_GROUP4_COLOR.g, DEFAULT_GROUP4_COLOR.b);
		}
	}
	else
	{
		switch (group)
		{
		case 0:
			return UVVert(GROUP_1_VALUE, GROUP_1_VALUE, GROUP_1_VALUE);
		case 1:
			return UVVert(GROUP_2_VALUE, GROUP_2_VALUE, GROUP_2_VALUE);
		case 2:
			return UVVert(GROUP_3_VALUE, GROUP_3_VALUE, GROUP_3_VALUE);
		case 3:
		default:
			return UVVert(GROUP_4_VALUE, GROUP_4_VALUE, GROUP_4_VALUE);
		}
	}
}

void AbnormalsMod::ActivateSubobjSel(int level, XFormModes &modes)
{
	SubSelectionPolyMod::ActivateSubobjSel(level, modes);

	// Show group colors if we're in a sub-object mode.
	showingGroupColors = (GetCurrentSubSelectionType() != SubSelectionPolyMod::SEL_OBJECT);

	editGroupsDlgProc->UpdateEditGroupsDlg(NULL);
}

class DummyXTC : public XTCObject
{
public:
	DummyXTC() { }
	Class_ID ExtensionID() { return Class_ID(0x730a33d7, 0x27246c35); }
	XTCObject *Clone() { return new DummyXTC(); }
	ChannelMask DependsOn() { return TOPO_CHANNEL | GEOM_CHANNEL; }
	ChannelMask ChannelsChanged() { return 0; }
	void PreChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline) {}
	void PostChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline) {}
	void DeleteThis() { delete this; }
};

bool AbnormalsMod::AssignGroupToSelectedFaces(int group, bool registerUndo)
{
	if (data != NULL)
	{
		BitArray selection;

		if (data->GetMesh()->getFaceSel(selection))
		{
			if (registerUndo)
			{
				theHold.Begin();
				theHold.Put(new AbnormalsModGroupRestoreObj(this, data, group));
			}
			data->SetFacesToGroup(selection, group);
			// Invalidate mesh and redraw
			NotifyDependents(FOREVER, PART_VERTCOLOR, REFMSG_CHANGE);
			ip->RedrawViews(ip->GetTime());
			
			if (registerUndo)
				theHold.Accept(_T("Apply abNormal Group"));
		}
		else
			return false;
	}
	else
		return false;
}

void AbnormalsMod::SelectGroup(int group, TimeValue t)
{
	BitArray faces = GetFacesOfColor(data->mesh, GetGroupValue(group), saveToChannel, COLOR_VALUE_TOLERANCE);
	data->SetFaceSel(faces, this, t);
	LocalDataChanged();
}

void AbnormalsMod::GenerateChamferNormals(MNMesh* mesh, MaxSDK::Array<UVVert> groupColors, int channel)
{
	const static float colorTolerance = 1.0f / 256.0f;

	MNNormalSpec* normalSpec = mesh->GetSpecifiedNormals();

	// Reset all normals to be unspecified, rebuild, and recompute.
	normalSpec->ResetNormals(false);
	normalSpec->BuildNormals();
	normalSpec->SetAllExplicit(false);
	normalSpec->SpecifyNormals(false);
	normalSpec->ComputeNormals();

	// Now set all normals to explicit.
	normalSpec->SetAllExplicit(true);

	MNMesh workingMesh = *mesh; // Make a copy of the input mesh.

	for (DWORD i = groupColors.length() - 1; i > 0; i--)
	{
		// Delete faces of group color on working mesh
		BitArray facesToDelete = GetFacesOfColor(&workingMesh, groupColors[i], channel, colorTolerance);
		DeleteFaces(workingMesh,facesToDelete);

		bool facesToDel[50];
		for (int f = 0; f < 50; f++)
		{
			if (f < facesToDelete.GetSize())
				facesToDel[f] = facesToDelete[f];
		}

		// Reset normals
		workingMesh.GetSpecifiedNormals()->SetAllExplicit(false);
		workingMesh.GetSpecifiedNormals()->ComputeNormals();

		// Copy normals of all remaining faces on working mesh to the source mesh
		CopyNormals(workingMesh, *mesh);
	}

	// For some reason calling normalSpec->SetAllExplicit() doesn't work. So manually make a selection of all normals, and set explicit.
	BitArray allNormalSelection = BitArray(normalSpec->GetNumNormals());
	for (DWORD i = 0; i < normalSpec->GetNumNormals(); i++)
		allNormalSelection.Set(i);

	normalSpec->MakeNormalsExplicit(false, &allNormalSelection, true);
}

BOOL AbnormalsMod::DependOnTopology(ModContext &mc)
{
	AbnormalsModLocalData *d = (AbnormalsModLocalData*)mc.localData;
	if (d == NULL)
		return false;

	// Depends on topology only if some groups have been changed with this modifier.
	return d->GetGroupsModified(); 
}

static bool AutoChamfer(MNMesh* mesh, AbnormalsMod::AutoChamferSource source, AbnormalsMod::AutoChamferMethod method, float radius, int segments, int workingChannel)
{
	if (mesh == NULL || radius <= 0 || segments <= 0)
		return false;

	float tension = 0.5f; // Always use 0.5 tension.
	BitArray edgesToChamfer;

	switch (source)
	{
	case AbnormalsMod::HardEdges:
	{
		edgesToChamfer = GetHardEdges(mesh);
		break;
	}
	case AbnormalsMod::SelectedEdges:
	{
		mesh->getEdgeSel(edgesToChamfer);
		break;
	}
	default:
		return false; // Invalid source
	}

	if (!edgesToChamfer.AnyBitSet())
		return false; // No edges to chamfer!

	// If all faces are currently set to clear, move them to group 0
	BitArray existingClearedFaces = GetFacesOfColor(mesh, AbnormalsMod::GetGroupValue(CLEAR_GROUP_INDEX), workingChannel, COLOR_VALUE_TOLERANCE);
	bool allCleared = true;
	for (DWORD i = 0; i < existingClearedFaces.GetSize(); i++)
		allCleared = allCleared & existingClearedFaces[i];

	if (allCleared)
	{
		mesh->ClearEFlags(MN_EDITPOLY_OP_SELECT);

		for (DWORD i = 0; i < existingClearedFaces.GetSize(); i++)
				mesh->f[i].SetFlag(MN_EDITPOLY_OP_SELECT);

		mesh->SetFaceColor(AbnormalsMod::GetGroupValue(0), workingChannel, MN_EDITPOLY_OP_SELECT);
	}

	// Store info about our mesh before the chamfer.
	DWORD oldFaceCount = mesh->numf;
	BitArray previousFaceSelection;
	mesh->getFaceSel(previousFaceSelection);
	BitArray previousEdgeSelection;
	mesh->getEdgeSel(previousEdgeSelection);

	// Flag all the faces we are chamfering with the MN_EDITPOLY_OP_SELECT flag.
	DWORD l_flag = MN_EDITPOLY_OP_SELECT;
	bool sucess = false;

	mesh->ClearEFlags(MN_EDITPOLY_OP_SELECT);

	for (DWORD i = 0; i < edgesToChamfer.GetSize(); i++)
		if (edgesToChamfer[i])
			mesh->E(i)->SetFlag(MN_EDITPOLY_OP_SELECT);

	MNChamferData10 chamferData = MNChamferData10(MNChamferData());
	// Do the actual chamfer
	if (method == AbnormalsMod::Quad) // Quad chamfer
	{
		MN_QCHAM_TYPE output = QCHAM_CHAMFEREDOBJECT;
		sucess = mesh->QuadChamfer(radius, segments, tension, output, l_flag);

		
	}
	else // Standard chamfer
	{
		IMNMeshUtilities10* meshUtils = static_cast<IMNMeshUtilities10*>(mesh->GetInterface(IMNMESHUTILITIES10_INTERFACE_ID));

		if (meshUtils != NULL)
			sucess = meshUtils->ChamferEdges(l_flag, chamferData, false, segments);

		if (sucess)
		{
			Tab<UVVert> mapDelta;
			for (int mapChannel = -NUM_HIDDENMAPS; mapChannel<mesh->numm; mapChannel++) 
			{
				if (mesh->M(mapChannel)->GetFlag(MN_DEAD)) 
					continue;

				chamferData.GetMapDelta(*mesh, mapChannel, radius, mapDelta);

				for (int i = 0; i<mapDelta.Count(); i++) 
					mesh->M(mapChannel)->v[i] += mapDelta[i];
			}

			Tab<Point3> vertexDelta;
			chamferData.GetDelta(radius, vertexDelta);

			for (int i = 0; i<vertexDelta.Count(); i++) 
				mesh->P(i) += vertexDelta[i];

			// Since standard chamfer doesn't set MN_FACE_CREATED, set it on all faces greater than the number of faces previously.
			// This seems to work, because all new faces created by the chamfer are appended to the face array.
			for (int i = oldFaceCount; i < mesh->numf; i++)
				mesh->f[i].SetFlag(MN_FACE_CREATED);
		}
		else 
			return false;
	}

	// Restore old selection
	mesh->FaceSelect(previousFaceSelection);
	mesh->EdgeSelect(previousEdgeSelection);

	// Set all created faces to clear group.
	mesh->SetFaceColor(AbnormalsMod::GetGroupValue(CLEAR_GROUP_INDEX), workingChannel, MN_FACE_CREATED);
	
	// If quad chamfer or regular chamfer with more than one segment, all "long" faces should be set to group 3 for proper smoothing.	
	if (segments > 1 || method == AbnormalsMod::Quad)
	{
		mesh->ClearFFlags(MN_EDITPOLY_OP_SELECT);
		const float faceEdgeThreshold = radius;
		for (DWORD f = 0; f < mesh->numf; f++)
		{
			if (mesh->f[f].GetFlag(MN_FACE_CREATED) && LongestEdge(mesh, f, true) > faceEdgeThreshold)
			{
				mesh->f[f].SetFlag(MN_EDITPOLY_OP_SELECT);
			}
		}
		mesh->SetFaceColor(AbnormalsMod::GetGroupValue(CLEAR_GROUP_INDEX - 1), workingChannel, MN_EDITPOLY_OP_SELECT);
	}	

	// Clear smoothing groups for all new faces.
	for (int i = oldFaceCount; i < mesh->numf; i++)
	{
		if (mesh->f[i].GetFlag(MN_FACE_CREATED))
			mesh->f[i].smGroup = 0;
	}

	if (source == AbnormalsMod::HardEdges) // Smooth entire mesh.
	{
		mesh->AutoSmooth(180.0f * DEG_TO_RAD, false, false);
	}
	else // Smooth just the chamfered edges.
	{		
		// Get the selection after the chamfer, this is the border of the chamfer.
		BitArray afterSelection;
		mesh->getEdgeSel(afterSelection);
		// Do a ring selection.
		BitArray ringSelection = afterSelection;
		mesh->SelectEdgeRing(ringSelection);
		
		// Remove any edges that are not used by a newly created face.
		for (int e = 0; e < ringSelection.GetSize(); e++)
		{
			if (ringSelection[e])
			{
				MNEdge& edge = mesh->e[e];
				if (!(mesh->f[edge.f1].GetFlag(MN_FACE_CREATED) || mesh->f[edge.f2].GetFlag(MN_FACE_CREATED)))
					ringSelection.Clear(e);
			}
		}

		// Do a loop selection.
		mesh->EdgeSelect(ringSelection);
		BitArray loopSelection = ringSelection;
		mesh->SelectEdgeLoop(loopSelection);

		// Now set edges to smooth.
		mesh->ClearEFlags(MN_EDITPOLY_OP_SELECT);
		for (int e = 0; e < loopSelection.GetSize(); e++)
		{
			if (loopSelection[e])
				mesh->e[e].SetFlag(MN_EDITPOLY_OP_SELECT);
		}

		mesh->EdgeSelect(loopSelection);
		// Smooth chamfered edges.
		PerformMakeSmoothEdges(*mesh, MN_EDITPOLY_OP_SELECT);
	}

	// Restore old selection
	mesh->FaceSelect(previousFaceSelection);
	//mesh->EdgeSelect(previousEdgeSelection);
	return sucess;
}

void AbnormalsMod::ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node)
{
	SubSelectionPolyMod::ModifyObject(t, mc, os, node);

	if (!os->obj->IsSubClassOf(polyObjectClassID))
		return;

	PolyObject *pobj = (PolyObject*)os->obj;
	MNMesh& mesh = pobj->GetMesh();

	// Update data
	data = (AbnormalsModLocalData*)mc.localData;

	MaxSDK::Array<UVVert> groupColors = MaxSDK::Array<UVVert>(4);
	groupColors[0] = GetGroupValue(0);
	groupColors[1] = GetGroupValue(1);
	groupColors[2] = GetGroupValue(2);
	groupColors[3] = GetGroupValue(3);

	UpdateMemberVarsFromParamBlock(t,os);

	if (loadFromChannel == MAP_NONE)
	{
		InitializeMap(saveToChannel, &mesh, true);
	}
	else if (loadFromChannel != saveToChannel)
	{
		if (!CopyMap(&mesh, loadFromChannel, saveToChannel))
		{
			// If copy fails, then just wipe the map.
			InitializeMap(saveToChannel, &mesh, true);
		}
	}

	if (loadLegacyColors)
	{
		// Convert legacy colors to new values.
		ConvertFromLegacyColors(&mesh, saveToChannel);
	}

	// Apply groups authored in this modifier to vertex channels
	ApplyGroupsToMesh(&mesh, saveToChannel);

	if (autoChamfer) // Do the auto-chamfer if it's enabled.
	{
		AutoChamfer(&mesh, chamferSource, chamferMethod, chamferRadius, chamferSegments, saveToChannel);
	}

	// Clear smoothing groups if specified
	if (clearSmoothing)
	{
		mesh.AutoSmooth(180.0f * DEG_TO_RAD, false, false);
	}

	// Ensure we have normals specified and built on our mesh, so they're there to modify.
	mesh.SpecifyNormals();
	mesh.GetSpecifiedNormals()->BuildNormals();

	// All good to go, do the normal modifications based on the groups embedded in the vertex channel.
	GenerateChamferNormals(&mesh, groupColors, saveToChannel);

	// Add an XTC object which tells and subsequent abnormals modifiers where we've stored our gorups.
	pobj->AddXTCObject(new AbnormalChannelInfoXTC(saveToChannel));

	// Make sure we don't get our normals deleted by a TriObjectNormalXTC
	for (int i = pobj->NumXTCObjects() - 1; i >= 0; i--)
	{
		if (pobj->GetXTCObject(i)->ExtensionID() == kTriObjNormalXTCID)
		{
			// Apparently calling triObject->RemoveXTCObject(i); causes a crash, and it keeps resetting our normals, soooo....
			// *SUPER* hacky hack. Swap the vtable of the existing TriObjectNormalXTC to my dummy one that doesn't do anything.
#ifdef _WIN64
#define PTR_VAR unsigned long long // For 64 bit pointers
#elif _WIN32
#define PTR_VAR unsigned int // For 32 bit pointers
#else
#error Unknown pointer size
#endif

			DummyXTC* dummyXTC = new DummyXTC();
			XTCObject* triObjectNormalXTC = (TriObjectNormalXTC*)pobj->GetXTCObject(i);

			PTR_VAR& dummyXTC_VtablePointer = *(PTR_VAR*)((void*)dummyXTC);
			PTR_VAR& normalXTC_VtablePointer = *(PTR_VAR*)((void*)triObjectNormalXTC);

			normalXTC_VtablePointer = dummyXTC_VtablePointer;
			delete dummyXTC;

#undef PTR_VAR
		}
	}

	// Update the cache used for display, hit-testing, painting:
	if (!data->GetMesh())
		data->SetCache(mesh);
	else
		*(data->GetMesh()) = MNMesh(mesh);

	UpdateDisplayCache(&mesh);
}

#define CHUNKID_GROUP1 0x1c3b
#define CHUNKID_GROUP2 0xbfc
#define CHUNKID_GROUP3 0x675d
#define CHUNKID_GROUP4 0x5f75

IOResult AbnormalsMod::SaveLocalData(ISave *isave, LocalModData *ld)
{
	IOResult superResult = SubSelectionPolyMod::SaveLocalData(isave, ld);
	if (superResult != IO_OK)
		return superResult;
	
	AbnormalsModLocalData *d = (AbnormalsModLocalData*)ld;

	isave->BeginChunk(CHUNKID_GROUP1);
	d->GetGroupFaces(0).Save(isave);
	isave->EndChunk();

	isave->BeginChunk(CHUNKID_GROUP2);
	d->GetGroupFaces(1).Save(isave);
	isave->EndChunk();

	isave->BeginChunk(CHUNKID_GROUP3);
	d->GetGroupFaces(2).Save(isave);
	isave->EndChunk();

	isave->BeginChunk(CHUNKID_GROUP4);
	d->GetGroupFaces(3).Save(isave);
	isave->EndChunk();

	return IO_OK;
}

void AbnormalsMod::LoadLocalDataChunk(ILoad *iload, PolyMeshSelData *d)
{
	SubSelectionPolyMod::LoadLocalDataChunk(iload, d);

	AbnormalsModLocalData* data = (AbnormalsModLocalData*)d; // This should have been filled in by super.
	BitArray tempArray;

	USHORT currentID = iload->CurChunkID();

	switch (iload->CurChunkID())
	{
	case CHUNKID_GROUP1:
		tempArray.Load(iload);
		data->SetFacesToGroup(tempArray, 0);
		break;
	case CHUNKID_GROUP2:
		tempArray.Load(iload);
		data->SetFacesToGroup(tempArray, 1);
		break;
	case CHUNKID_GROUP3:
		tempArray.Load(iload);
		data->SetFacesToGroup(tempArray, 2);
		break;
	case CHUNKID_GROUP4:
		tempArray.Load(iload);
		data->SetFacesToGroup(tempArray, 3);
		break;
	}
}

int AbnormalsMod::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext *mc)
{
	AbnormalsModLocalData* abnormalData = (AbnormalsModLocalData*)mc->localData;

	MNMesh* mesh = abnormalData->GetMesh();
	MNNormalSpec* normSpec = NULL;

	if (mesh)
		normSpec = mesh->GetSpecifiedNormals();

	GraphicsWindow *gw = vpt->getGW();
	ViewExp13* viewExp13 = (ViewExp13*)vpt;
	Matrix3 tm = inode->GetObjectTM(t);
	int savedLimits = gw->getRndLimits();
	gw->setRndLimits(savedLimits & ~GW_ILLUM);
	gw->setTransform(tm);

	// Draw debug normals if enabled.
	if (showNormals && mesh && normSpec)
	{
		Point3 normalLine[2];
		gw->startSegments();

		// Iterate backwards, so lower groups draw ontop of higher groups.
		for (int i = ABNORMALS_NUM_GROUPS - 1; i >= 0; i--)
		{
			switch (i)
			{
			case 0:
				gw->setColor(LINE_COLOR, DEFAULT_GROUP1_COLOR);
				break;
			case 1:
				gw->setColor(LINE_COLOR, DEFAULT_GROUP2_COLOR);
				break;
			case 2:
				gw->setColor(LINE_COLOR, DEFAULT_GROUP3_COLOR);
				break;
			default:
				gw->setColor(LINE_COLOR, DEFAULT_GROUP4_COLOR);
				break;
			}

			for (DWORD n = 0; n < debugNormals[i].length(); n++)
			{
				gw->segment(&debugNormals[i][n].start,1);
			}
		}
		gw->endSegments();
	}

	Point3 litCols[3];

	gw->setRndLimits(savedLimits | GW_ILLUM | GW_SPECULAR);

	// Draw overlay triangles with appropriate materials to visualize groups.
	if (showingGroupColors && !vpt->IsWire() && mesh && normSpec && vpt->IsActive())
	{
		gw->startTriangles();
		gw->setTransparency(GW_TRANSPARENCY | GW_TRANSPARENT_PASS);
		// Draw colored overlay triangles.
		for (int i = 0; i < NUM_ELEMENTS(overlayTris) - 1; i++)
		{
			gw->setMaterial(overlayMaterials[i]);

			for (DWORD t = 0; t < overlayTris[i].length(); t++)
			{
				gw->triangleN(overlayTris[i][t].pos, overlayTris[i][t].norms, overlayTris[i][t].norms);
			}
		}
		// Redraw Selected faces (native selected faces get covered by overlay)
		if (gw->getRndLimits() & GW_SHADE_SEL_FACES && (GetCurrentSubSelectionType() == SEL_FACE || GetCurrentSubSelectionType() == SEL_ELEMENT))
		{
			gw->setMaterial(selectionMaterial);
			gw->setRndLimits(savedLimits & ~GW_ILLUM); // Disable illumination for selection overlay.
			for (DWORD t = 0; t < overlayTris[NUM_ELEMENTS(overlayTris) - 1].length(); t++)
			{
				gw->triangleN(overlayTris[NUM_ELEMENTS(overlayTris) - 1][t].pos, overlayTris[NUM_ELEMENTS(overlayTris) - 1][t].norms, overlayTris[NUM_ELEMENTS(overlayTris) - 1][t].norms);
			}
		}

		gw->endTriangles();

		// Redraw mesh edges overtop of overlay mesh.
		if (viewExp13->GetEdgedFaces())
		{
#ifdef MESH_CAGE_BACKFACE_CULLING
			if (savedLimits & GW_BACKCULL) 
				mesh->UpdateBackfacing(gw, true);
#endif
			gw->startSegments();

			gw->setColor(LINE_COLOR, Point3(1, 1, 1));
			for (DWORD i = 0; i < overlayEdges.length(); i += 2)
			{
				gw->segment(&overlayEdges[i], 1);
			}

			gw->endSegments();
		}

		// Draw selected edges even if not in edged faces mode
		gw->setColor(LINE_COLOR, Point3(1, 0, 0));
		for (DWORD i = 0; i < overlaySelectedEdges.length(); i += 2)
		{
			gw->segment(&overlaySelectedEdges[i], 1);
		}
	}

	// Reset render limits
	gw->setRndLimits(savedLimits);
	// Purposely don't call super, we don't want orange edges drawn when show end result is enabled.
	//return SubSelectionPolyMod::Display(t, inode, vpt, flags, mc);
	return 0;
}

RefTargetHandle AbnormalsMod::Clone(RemapDir& remap)
{
	AbnormalsMod* newmod = new AbnormalsMod();
	newmod->ReplaceReference(0, remap.CloneRef(pblock2));
	BaseClone(this, newmod, remap);
	return newmod;
}

RefResult AbnormalsMod::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate)
{
	switch (message)
	{
		case REFMSG_CHANGE:
			abnormalsModParamBlockDesc.InvalidateUI();
			/*if (pmapParam && pmapParam->GetParamBlock() == pblock2) 
			{
				pmapParam->Invalidate();
				
			}*/
			break;
	}
	return REF_SUCCEED;
}

void AbnormalsMod::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	this->ip = ip;
	Modifier::BeginEditParams(ip, flags, prev);
	AbnormalsModClassDesc::GetInstance()->BeginEditParams(ip, this, flags, prev);

	// Activate keyboard shortcuts
	if (!abnormalsActionCallback)
	{
		abnormalsActionCallback = new AbnormalModActionCallback(this);
		int result = ip->GetActionManager()->ActivateActionTable(abnormalsActionCallback, ABNORMALS_MOD_ACTION_TABLE_ID);
	}

	generalGroupsDlgProc = new AbnormalsModChannelsDlgProc(this);
	editGroupsDlgProc = new AbnormalsModEditGroupsDlgProc(this);
	autoChamferDlgProc = new AbnormalsModAutoChamferDlgProc(this);
	abnormalsModParamBlockDesc.SetUserDlgProc(eAbnormalsModRollout::channels, generalGroupsDlgProc);
	abnormalsModParamBlockDesc.SetUserDlgProc(eAbnormalsModRollout::general, editGroupsDlgProc);
	abnormalsModParamBlockDesc.SetUserDlgProc(eAbnormalsModRollout::auto_chamfer, autoChamferDlgProc);

	// Call super
	SubSelectionPolyMod::BeginEditParams(ip, flags, prev);
}

void AbnormalsMod::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	Modifier::EndEditParams(ip, flags, next);
	AbnormalsModClassDesc::GetInstance()->EndEditParams(ip, this, flags, next);
	generalGroupsDlgProc = NULL;
	editGroupsDlgProc = NULL;
	autoChamferDlgProc = NULL;
	showingGroupColors = false;

	// Deactivate keyboard shortcuts
	if (abnormalsActionCallback) 
	{
		int result = ip->GetActionManager()->DeactivateActionTable(abnormalsActionCallback, ABNORMALS_MOD_ACTION_TABLE_ID);
		delete abnormalsActionCallback;
		abnormalsActionCallback = NULL;
	}

	// Call super
	SubSelectionPolyMod::EndEditParams(ip, flags, next);

	this->ip = NULL;
}