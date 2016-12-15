#include "stdafx.h"
#include "GeomDecalParamBlock.h"

#include "GeomDecal.h"
#include "GeomDecalClassDesc.h"
#include "GeomDecalPBAccessor.h"
#define PBLOCK_REF_NO 0

static GeomDecalPBAccessor GeomDecalPBAccessor;

// Param block flags http://help.autodesk.com/view/3DSMAX/2016/ENU/?guid=__files_GUID_FAD66C54_779B_477A_99B2_50B63A2D6A1B_htm
// Multiple roll-ups in one param block: http://help.autodesk.com/view/3DSMAX/2016/ENU/?guid=__files_GUID_1D899F02_FB2B_4A61_A1AA_8DF52B6C6C27_htm
// Param block for the plugin
ParamBlockDesc2 GeomDecalParamBlockDesc(eGeomDecalParamBlock::geom_decal_param_block, _T("geom_decal_param_block"), 0, GeomDecalClassDesc::GetInstance(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF_NO,
	// Map rollups
	1, // Number of rollups
	eGeomDecalRollout::general, IDD_GEOMDECAL_GENERAL, IDS_ROLLUP_GENERAL, 0, 0, NULL,

	// --- General Rollup ---

		// Projection Size
		eGeomDecalParam::projection_size, _T("projection_size"), TYPE_POINT3, P_ANIMATABLE, IDS_BLANK,
		p_default, Point3(100.0f, 100.0f, 100.0f),
		p_range, 0.0f, 999999.0f,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_PROJECTION_SIZE_X, IDC_PROJECTION_SIZE_X_SPN, IDC_PROJECTION_SIZE_Y, IDC_PROJECTION_SIZE_Y_SPN, IDC_PROJECTION_DEPTH, IDC_PROJECTION_DEPTH_SPN, SPIN_AUTOSCALE,
		p_end,

		// Angle Cutoff
		eGeomDecalParam::projection_anglecutoff, _T("projection_anglecutoff"), TYPE_FLOAT, P_ANIMATABLE, IDS_BLANK,
		p_default, 90.0f,
		p_range, 0.0f, 90.0f,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PROJECTION_ANGLECUTOFF, IDC_PROJECTION_ANGLECUTOFF_SPN, 1.0f,
		p_end,

		// Node Pool
		eGeomDecalParam::node_pool, _T("node_pool"), TYPE_INODE_TAB, 0, P_AUTO_UI | P_VARIABLE_SIZE, IDS_NODELIST,
		p_ui, eGeomDecalRollout::general, TYPE_NODELISTBOX, IDC_OBJECTS_LIST, IDC_PICKOBJ, 0, IDC_REMOVEBOBJ,
		p_tooltip, IDS_TIP_NODEPOOL,
		p_end,

		// UV Tiling
		eGeomDecalParam::uv_tiling, _T("uv_tiling"), TYPE_POINT3, P_ANIMATABLE, IDS_BLANK,
		p_default, Point3(1.0f, 1.0f, 1.0f),
		p_range, 0.0f, 1000.0f,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UV_TILE_U, IDC_UV_TILE_U_SPN, IDC_UV_TILE_V, IDC_UV_TILE_V_SPN, 1000, 1000, SPIN_AUTOSCALE,
		p_end,

		// UV Offset
		eGeomDecalParam::uv_offset, _T("uv_offset"), TYPE_POINT3, P_ANIMATABLE, IDS_BLANK,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, 0.0f, 1.0f,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UV_OFFSET_U, IDC_UV_OFFSET_U_SPN, IDC_UV_OFFSET_V, IDC_UV_OFFSET_V_SPN, 1000, 1000, 0.001f,
		p_end,

		// Mesh Push
		eGeomDecalParam::mesh_push, _T("mesh_push"), TYPE_FLOAT, P_ANIMATABLE, IDS_BLANK,
		p_default, 1.0f,
		p_range, 0.0f, 1000.0f,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_MESH_PUSH, IDC_MESH_PUSH_SPN, SPIN_AUTOSCALE,
		p_end,

		// Mesh Material Id
		eGeomDecalParam::mesh_matid, _T("mesh_matid"), TYPE_INT, P_ANIMATABLE, IDS_BLANK,
		p_default, 1,
		p_range, 1, 65535,
		p_ui, eGeomDecalRollout::general, TYPE_SPINNER, EDITTYPE_INT, IDC_MESH_MATID, IDC_MESH_MATID_SPN, 1,
		p_end,

	p_end // End rollouts
);