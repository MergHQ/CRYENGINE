#include "stdafx.h"

#include "FlowPaintParamBlock.h"

#include "FlowPaintClassDesc.h"

ParamBlockDesc2 flowPaintParamBlockDesc(eFlowPaintParamBlock::flowpaint_param_block, _T("flowpaint_param_block"), 0, FlowPaintTestClassDesc::GetInstance(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
	// Map rollups
	1, // Number of rollups
	// Enum, Dialog ID, String ID, Hidden(bool), rolled up (bool), and ???
	eFlowPaintRollout::general, IDD_PANEL, IDS_ROLLUP_GENERAL, FALSE, FALSE, NULL,

	// --- General Rollup ---
	eFlowPaintParam::show_flow_length, _T("show_flow_length"), TYPE_WORLD, P_ANIMATABLE, IDS_BLANK,
	p_default, 1.0f,
	p_range, 0.0f, 999999.0f,
	p_ui, eFlowPaintRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_SHOW_FLOW_SIZE, IDC_SHOW_FLOW_SIZE_SPN, SPIN_AUTOSCALE,
	p_end,

	p_end
);