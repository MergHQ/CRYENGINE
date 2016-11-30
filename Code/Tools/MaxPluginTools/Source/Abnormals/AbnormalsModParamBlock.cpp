#include "stdafx.h"
#include "AbnormalsModParamBlock.h"
#include "AbnormalsMod.h"
#include "AbnormalsModClassDesc.h"
#define PBLOCK_REF_NO 10

// Param block for the plugin
ParamBlockDesc2 abnormalsModParamBlockDesc(eAbnormalsModParamBlock::abnormals_param_block, _T("abnormals_param_block"), 0, AbnormalsModClassDesc::GetInstance(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF_NO,
	// Map rollups
	4, // Number of rollups
	// Enum, Dialog ID, String ID, Hidden(bool), rolled up (bool), and ???
	eAbnormalsModRollout::general, IDD_ABNORMALSMOD_EDITGROUPS, IDS_ROLLUP_GENERAL, FALSE, FALSE, NULL,
	eAbnormalsModRollout::auto_chamfer, IDD_ABNORMALSMOD_AUTOCHAMFER, IDS_ROLLUP_AUTOCHAMFER, FALSE, TRUE, NULL,
	eAbnormalsModRollout::channels, IDD_ABNORMALSMOD_CHANNELS, IDS_ROLLUP_CHANNELS, FALSE, TRUE, NULL,
	eAbnormalsModRollout::info, IDD_MODIFIER_INFO, IDS_ROLLUP_INFO, FALSE, FALSE, NULL,

	// --- General Rollup ---

	eAbnormalsModParam::show_normals, _T("show_normals"), TYPE_BOOL, P_ANIMATABLE, IDS_BLANK,
	p_default, FALSE,
	p_ui, eAbnormalsModRollout::general, TYPE_SINGLECHEKBOX, IDC_SHOW_NORMALS,
	p_end,

	eAbnormalsModParam::show_normals_size, _T("show_normals_size"), TYPE_WORLD, P_ANIMATABLE, IDS_BLANK,
	p_default, 1.0f,
	p_range, 0.0f, 999999.0f,
	p_ui, eAbnormalsModRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_SHOW_NORMALS_SIZE, IDC_SHOW_NORMALS_SIZE_SPN, SPIN_AUTOSCALE,
	p_end,

	eAbnormalsModParam::overlay_opacity, _T("overlay_opacity"), TYPE_FLOAT, P_ANIMATABLE, IDS_BLANK,
	p_default, 1.0f,
	p_range, 0.0f, 1.0f,
	p_ui, eAbnormalsModRollout::general, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OVERLAY_OPACITY, IDC_OVERLAY_OPACITY_SPN, SPIN_AUTOSCALE,
	p_end,

	eAbnormalsModParam::clear_smoothing, _T("clear_smoothing"), TYPE_BOOL, P_ANIMATABLE, IDS_BLANK,
	p_default, FALSE,
	p_ui, eAbnormalsModRollout::general, TYPE_SINGLECHEKBOX, IDC_CLEAR_SMOOTHING,
	p_end,

	// --- Auto Chamfer Rollup ---

	eAbnormalsModParam::auto_chamfer, _T("auto_chamfer"), TYPE_BOOL, P_ANIMATABLE | P_RESET_DEFAULT, IDS_BLANK,
	p_default, FALSE,
	p_ui, eAbnormalsModRollout::auto_chamfer, TYPE_SINGLECHEKBOX, IDC_AUTO_CHAMFER,
	p_end,

	eAbnormalsModParam::auto_chamfer_source, _T("auto_chamfer_source"), TYPE_INT, 0, IDS_BLANK,
	p_default, 0,
	p_range, 0, 1,
	p_ui, eAbnormalsModRollout::auto_chamfer, TYPE_INTLISTBOX, IDC_CHAMFER_SOURCE, 2, IDS_CHAMFER_SOURCE_HARDEDGES, IDS_CHAMFER_SOURCE_SELECTEDEDGES,
	p_end,

	eAbnormalsModParam::auto_chamfer_method, _T("auto_chamfer_method"), TYPE_INT, 0, IDS_BLANK,
	p_default, 0,
	p_range, 0, 1,
	p_ui, eAbnormalsModRollout::auto_chamfer, TYPE_RADIO, 2, IDC_AUTO_CHAMFER_STANDARD, IDC_AUTO_CHAMFER_QUAD,
	p_end,

	eAbnormalsModParam::auto_chamfer_radius, _T("auto_chamfer_radius"), TYPE_WORLD, P_ANIMATABLE, IDS_BLANK,
	p_default, 1.0f,
	p_range, 0.0f, 999999.0f,
	p_ui, eAbnormalsModRollout::auto_chamfer, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_CHAMFER_RADIUS, IDC_CHAMFER_RADIUS_SPN, SPIN_AUTOSCALE,
	p_end,

	eAbnormalsModParam::auto_chamfer_segments, _T("auto_chamfer_segments"), TYPE_INT, P_ANIMATABLE, IDS_BLANK,
	p_default, 1,
	p_range, 1, 100,
	p_ui, eAbnormalsModRollout::auto_chamfer, TYPE_SPINNER, EDITTYPE_INT, IDC_CHAMFER_SEGMENTS, IDC_CHAMFER_SEGMENTS_SPN, SPIN_AUTOSCALE,
	p_end,

	// --- Channels Rollup ---

	eAbnormalsModParam::color_group_1, _T("color_group_1"), TYPE_RGBA, P_ANIMATABLE, IDS_BLANK,
	p_default, COLVAL_8BIT(103, 158, 33),
	p_ui, eAbnormalsModRollout::channels, TYPE_COLORSWATCH, IDC_COLOR_GROUP_1,
	p_end,

	eAbnormalsModParam::color_group_2, _T("color_group_2"), TYPE_RGBA, P_ANIMATABLE, IDS_BLANK,
	p_default, DEFAULT_GROUP2_COLOR,
	p_ui, eAbnormalsModRollout::channels, TYPE_COLORSWATCH, IDC_COLOR_GROUP_2,
	p_end,

	eAbnormalsModParam::color_group_3, _T("color_group_3"), TYPE_RGBA, P_ANIMATABLE, IDS_BLANK,
	p_default, DEFAULT_GROUP3_COLOR,
	p_ui, eAbnormalsModRollout::channels, TYPE_COLORSWATCH, IDC_COLOR_GROUP_3,
	p_end,

	eAbnormalsModParam::color_clear, _T("color_clear"), TYPE_RGBA, P_ANIMATABLE, IDS_BLANK,
	p_default, DEFAULT_GROUP4_COLOR,
	p_ui, eAbnormalsModRollout::channels, TYPE_COLORSWATCH, IDC_COLOR_CLEAR,
	p_end,

	// Load from

	eAbnormalsModParam::load_auto, _T("load_auto"), TYPE_BOOL, P_ANIMATABLE | P_RESET_DEFAULT, IDS_BLANK,
	p_default, TRUE,
	p_ui, eAbnormalsModRollout::channels, TYPE_SINGLECHEKBOX, IDC_LOAD_AUTO,
	p_end,

	eAbnormalsModParam::load_groups_from, _T("load_from"), TYPE_INT, 0, IDS_BLANK,
	p_default, 0,
	p_range, 0, 4,
	p_ui, eAbnormalsModRollout::channels, TYPE_RADIO, 5, IDC_LOAD_ILLUM_RAD, IDC_LOAD_COLOR_RAD, IDC_LOAD_OPAC_RAD, IDC_LOAD_UV_RAD, IDC_LOAD_NONE,
	p_end,

	eAbnormalsModParam::load_legacy_colors, _T("load_legacy_colors"), TYPE_BOOL, P_ANIMATABLE, IDS_BLANK,
	p_default, FALSE,
	p_ui, eAbnormalsModRollout::channels, TYPE_SINGLECHEKBOX, IDC_LOAD_LEGACY_COLORS,
	p_end,

	eAbnormalsModParam::load_groups_uv_channel, _T("load_uv_channel"), TYPE_INT, P_ANIMATABLE, IDS_BLANK,
	p_default, 3,
	p_range, 2, 99,
	p_ui, eAbnormalsModRollout::channels, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_LOAD_UV_CHANNEL_NUM, IDC_LOAD_UV_CHANNEL_NUM_SPN, 1,
	p_end,	

	// Save to

	eAbnormalsModParam::save_auto, _T("save_auto"), TYPE_BOOL, P_ANIMATABLE | P_RESET_DEFAULT, IDS_BLANK,
	p_default, TRUE,
	p_ui, eAbnormalsModRollout::channels, TYPE_SINGLECHEKBOX, IDC_SAVE_AUTO,
	p_end,

	eAbnormalsModParam::save_groups_to, _T("save_to"), TYPE_INT, 0, IDS_BLANK,
	p_default, -1,
	p_range, 0, 3,
	p_ui, eAbnormalsModRollout::channels, TYPE_RADIO, 4, IDC_SAVE_ILLUM_RAD, IDC_SAVE_COLOR_RAD, IDC_SAVE_OPAC_RAD, IDC_SAVE_UV_RAD,
	p_end,

	eAbnormalsModParam::save_groups_uv_channel, _T("save_uv_channel"), TYPE_INT, P_ANIMATABLE, IDS_BLANK,
	p_default, 3,
	p_range, 2, 99,
	p_ui, eAbnormalsModRollout::channels, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_SAVE_UV_CHANNEL_NUM, IDC_SAVE_UV_CHANNEL_NUM_SPN, 1,
	p_end,

	p_end // End rollouts
);