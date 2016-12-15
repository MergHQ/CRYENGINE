#include "stdafx.h"
#include "SuperArrayParamBlock.h"

#include "SuperArray.h"
#include "SuperArrayClassDesc.h"
#include "SuperArrayPBAccessor.h"
#define PBLOCK_REF_NO 0

static SuperArrayPBAccessor superArrayPBAccessor;

// Param block flags http://help.autodesk.com/view/3DSMAX/2016/ENU/?guid=__files_GUID_FAD66C54_779B_477A_99B2_50B63A2D6A1B_htm
// Multiple roll-ups in one param block: http://help.autodesk.com/view/3DSMAX/2016/ENU/?guid=__files_GUID_1D899F02_FB2B_4A61_A1AA_8DF52B6C6C27_htm
// Param block for the plugin
ParamBlockDesc2 superArrayParamBlockDesc(eSuperArrayParamBlock::super_array_param_block, _T("super_array_param_block"), 0, SuperArrayClassDesc::GetInstance(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF_NO,
	// Map rollups
	4, // Number of rollups
	eSuperArrayRollout::general, IDD_SUPERARRAY_GENERAL, IDS_ROLLUP_GENERAL, 0, 0, NULL,
	eSuperArrayRollout::rotation, IDD_SUPERARRAY_ROTATION, IDS_ROLLUP_ROTATION, 0, 0, NULL,
	eSuperArrayRollout::offset, IDD_SUPERARRAY_OFFSET, IDS_ROLLUP_OFFSET, 0, 0, NULL,
	eSuperArrayRollout::scale, IDD_SUPERARRAY_SCALE, IDS_ROLLUP_SCALE, 0, 0, NULL,
	

	// --- General Rollup ---

		// Master Seed
		eSuperArrayParam::master_seed, _T("master_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_MASTER_SEED, IDC_MASTER_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

		// Copy type (distance or count)
		eSuperArrayParam::copy_type, _T("copy_type"), TYPE_INT, 0, IDS_COPIES,
		p_default, 1,
		p_range, 0, 1,
		p_ui, eSuperArrayRollout::general, TYPE_RADIO, 2, IDC_COPY_DIST_RAD, IDC_COPY_COUNT_RAD,
		p_end,

		// Copy distance
		eSuperArrayParam::copy_distance, _T("copy_distance"), TYPE_FLOAT, P_ANIMATABLE, IDS_COPY_DISTANCE,
		p_default, 100.0f,
		p_range, 0.0f, 999999.0f,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_COPIES_DIST, IDC_COPIES_DIST_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_COPYDISTANCE,
		p_end,
	
		// Copy exact distance
		eSuperArrayParam::copy_distance_exact, _T("copy_distance_exact"), TYPE_BOOL, P_ANIMATABLE, IDS_COPY_EXACT,
		p_default, FALSE,
		p_ui, eSuperArrayRollout::general, TYPE_SINGLECHEKBOX, IDC_COPIES_DIST_EXACT,
		p_tooltip, IDS_TIP_COPYDISTANCE_EXACT,
		p_end,

		// Copy count
		eSuperArrayParam::copy_count, _T("copy_count"), TYPE_INT, P_ANIMATABLE, IDS_COPY_COUNT,
		p_default, 10,
		p_range, 1, SAFETY_MAX_COPIES,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_COPIES_COUNT, IDC_COPIES_COUNT_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_COPYCOUNT,
		p_end,

		// Copy Direction
		eSuperArrayParam::copy_direction, _T("copy_direction"), TYPE_INT, 0, IDS_COPY_DIRECTION,
		p_default, 0,
		p_range, 0, 2,
		p_ui, eSuperArrayRollout::general, TYPE_RADIO, 3, IDC_COPY_DIR_X_RAD, IDC_COPY_DIR_Y_RAD, IDC_COPY_DIR_Z_RAD,
		p_tooltip, IDS_TIP_COPYDIRECTION,
		p_end,

		// Spacing type (pivot or bounds)
		eSuperArrayParam::spacing_type, _T("spacing_type"), TYPE_INT, 0, IDS_SPACING_TYPE,
		p_default, 0,
		p_range, 0, 1,
		p_ui, eSuperArrayRollout::general, TYPE_RADIO, 2, IDC_SPACING_BOUNDS_RAD, IDC_SPACING_PIVOT_RAD,
		p_end,

		// Spacing pivot
		eSuperArrayParam::spacing_pivot, _T("spacing_pivot"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPACING_PIVOT,
		p_default, 10.0f,
		p_range, 0.0001f, 999999.0f,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_SPACING_PIVOT, IDC_SPACING_PIVOT_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SPACING_PIVOT,
		p_end,

		// Spacing bounds
		eSuperArrayParam::spacing_bounds, _T("spacing_bounds"), TYPE_FLOAT, P_ANIMATABLE, IDS_SPACING_BOUNDS,
		p_default, 0.0f,
		p_range, -999.0f, 999999.0f,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_SPACING_BOUNDS, IDC_SPACING_BOUNDS_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SPACING_BOUNDS,
		p_end,

		// Node pool
		eSuperArrayParam::node_pool, _T("node_pool"), TYPE_INODE_TAB, 0, P_AUTO_UI | P_VARIABLE_SIZE, IDS_NODELIST,
		p_ui, eSuperArrayRollout::general, TYPE_NODELISTBOX, IDC_OBJECTS_LIST, IDC_PICKOBJ, 0, IDC_REMOVEBOBJ,
		p_tooltip, IDS_TIP_NODEPOOL,
		p_end,

		// Pool seed
		eSuperArrayParam::pool_seed, _T("pool_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::general, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_POOL_SEED, IDC_POOL_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

	// --- Rotation Rollup ---

		// Base Rotation
		eSuperArrayParam::base_rot, _T("base_rot"), TYPE_POINT3, P_ANIMATABLE, IDS_ROT_INCREMENTAL,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, -360.0f, 360.0f,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BASE_ROT_X, IDC_BASE_ROT_X_SPN, IDC_BASE_ROT_Y, IDC_BASE_ROT_Y_SPN, IDC_BASE_ROT_Z, IDC_BASE_ROT_Z_SPN, 1.0f,
		p_tooltip, IDS_TIP_ROT_BASE,
		p_end,

		// Incremental Rotation
		eSuperArrayParam::incremental_rot, _T("incremental_rot"), TYPE_POINT3, P_ANIMATABLE, IDS_ROT_INCREMENTAL,
		p_default, Point3(0.0f,0.0f,0.0f),
		p_range, -360.0f, 360.0f,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_INITIAL_ROT_X, IDC_INITIAL_ROT_X_SPN, IDC_INITIAL_ROT_Y, IDC_INITIAL_ROT_Y_SPN, IDC_INITIAL_ROT_Z, IDC_INITIAL_ROT_Z_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_INCREMENTALROT,
		p_end,

		// Positive checkboxes
		eSuperArrayParam::rand_rot_pos_x, _T("rand_rot_pos_x"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_POS_X,
		p_end,

		eSuperArrayParam::rand_rot_pos_y, _T("rand_rot_pos_y"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_POS_Y,
		p_end,

		eSuperArrayParam::rand_rot_pos_z, _T("rand_rot_pos_z"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_POS_Z,
		p_end,

		// Negative checkboxes
		eSuperArrayParam::rand_rot_neg_x, _T("rand_rot_neg_x"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_NEG_X,
		p_end,

		eSuperArrayParam::rand_rot_neg_y, _T("rand_rot_neg_y"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_NEG_Y,
		p_end,

		eSuperArrayParam::rand_rot_neg_z, _T("rand_rot_neg_z"), TYPE_BOOL, P_ANIMATABLE, IDS_ROT_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::rotation, TYPE_SINGLECHEKBOX, IDC_RAND_ROT_NEG_Z,
		p_end,

		// Min Random Rotation spinner
		eSuperArrayParam::rand_rot_min, _T("rand_rot_min"), TYPE_POINT3, P_ANIMATABLE, IDS_MIN_ROT,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, 0.0f, 360.0f,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_ROT_MIN_X, IDC_RAND_ROT_MIN_X_SPN, IDC_RAND_ROT_MIN_Y, IDC_RAND_ROT_MIN_Y_SPN, IDC_RAND_ROT_MIN_Z, IDC_RAND_ROT_MIN_Z_SPN, 1.0f,
		p_tooltip, IDS_TIP_RANDROT_MIN,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Max Random Rotation spinner
		eSuperArrayParam::rand_rot_max, _T("rand_rot_max"), TYPE_POINT3, P_ANIMATABLE, IDS_MAX_ROT,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, 0.0f, 360.0f,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_ROT_MAX_X, IDC_RAND_ROT_MAX_X_SPN, IDC_RAND_ROT_MAX_Y, IDC_RAND_ROT_MAX_Y_SPN, IDC_RAND_ROT_MAX_Z, IDC_RAND_ROT_MAX_Z_SPN, 1.0f,
		p_tooltip, IDS_TIP_RANDROT_MAX,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Random Rotation distribution spinner
		eSuperArrayParam::rand_rot_dist, _T("rand_rot_dist"), TYPE_FLOAT, P_ANIMATABLE, IDS_DISTRIBUTION,
		p_default, 1.0f,
		p_ms_default, 1.0f,
		p_range, 0.01f, 100.0f,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_ROT_DISTRIBUTION, IDC_RAND_ROT_DISTRIBUTION_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RAND_DISTRIBUTION,
		p_end,

		// Random rotation seed spinner
		eSuperArrayParam::rand_rot_seed, _T("rand_rot_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::rotation, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_RAND_ROT_SEED, IDC_RAND_ROT_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

	// --- Scale Rollup ---

		// Random flip x
		eSuperArrayParam::rand_flip_x, _T("rand_flip_x"), TYPE_BOOL, P_ANIMATABLE, IDS_FLIP_AXIS,
		p_default, FALSE,
		p_ui, eSuperArrayRollout::scale, TYPE_SINGLECHEKBOX, IDC_RAND_FLIP_X,
		p_end,

		// Random flip y
		eSuperArrayParam::rand_flip_y, _T("rand_flip_y"), TYPE_BOOL, P_ANIMATABLE, IDS_FLIP_AXIS,
		p_default, FALSE,
		p_ui, eSuperArrayRollout::scale, TYPE_SINGLECHEKBOX, IDC_RAND_FLIP_Y,
		p_end,

		// Random flip z
		eSuperArrayParam::rand_flip_z, _T("rand_flip_z"), TYPE_BOOL, P_ANIMATABLE, IDS_FLIP_AXIS,
		p_default, FALSE,
		p_ui, eSuperArrayRollout::scale, TYPE_SINGLECHEKBOX, IDC_RAND_FLIP_Z,
		p_end,

		// Random flip seed
		eSuperArrayParam::rand_flip_seed, _T("rand_flip_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::scale, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_RAND_FLIP_SEED, IDC_RAND_FLIP_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

		// Min Scale Rotation spinner
		eSuperArrayParam::rand_scale_min, _T("rand_scale_min"), TYPE_POINT3, P_ANIMATABLE, IDS_SCALE_MIN,
		p_default, Point3(1.0f, 1.0f, 1.0f),
		p_range, 0.0f, 100.0f,
		p_ui, eSuperArrayRollout::scale, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_SCALE_MIN_X, IDC_RAND_SCALE_MIN_X_SPN, IDC_RAND_SCALE_MIN_Y, IDC_RAND_SCALE_MIN_Y_SPN, IDC_RAND_SCALE_MIN_Z, IDC_RAND_SCALE_MIN_Z_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RANDSCALE_MIN,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Max Scale Rotation spinner
		eSuperArrayParam::rand_scale_max, _T("rand_scale_max"), TYPE_POINT3, P_ANIMATABLE, IDS_SCALE_MAX,
		p_default, Point3(1.0f, 1.0f, 1.0f),
		p_range, 0.0f, 100.0f,
		p_ui, eSuperArrayRollout::scale, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_SCALE_MAX_X, IDC_RAND_SCALE_MAX_X_SPN, IDC_RAND_SCALE_MAX_Y, IDC_RAND_SCALE_MAX_Y_SPN, IDC_RAND_SCALE_MAX_Z, IDC_RAND_SCALE_MAX_Z_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RANDSCALE_MAX,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Random Scale distribution spinner
		eSuperArrayParam::rand_scale_dist, _T("rand_scale_dist"), TYPE_FLOAT, P_ANIMATABLE, IDS_DISTRIBUTION,
		p_default, 1.0f,
		p_range, 0.01f, 100.0f,
		p_ui, eSuperArrayRollout::scale, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_SCALE_DISTRIBUTION, IDC_RAND_SCALE_DISTRIBUTION_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RAND_DISTRIBUTION,
		p_end,

		// Random Scale seed spinner
		eSuperArrayParam::rand_scale_seed, _T("rand_scale_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::scale, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_RAND_SCALE_SEED, IDC_RAND_SCALE_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

	// --- Offset Rollup ---

		// Positive checkboxes
		eSuperArrayParam::rand_offset_pos_x, _T("rand_offset_pos_x"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_POS_X,
		p_end,

		eSuperArrayParam::rand_offset_pos_y, _T("rand_offset_pos_y"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_POS_Y,
		p_end,

		eSuperArrayParam::rand_offset_pos_z, _T("rand_offset_pos_z"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_POSITIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_POS_Z,
		p_end,

		// Negative checkboxes
		eSuperArrayParam::rand_offset_neg_x, _T("rand_offset_neg_x"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_NEG_X,
		p_end,

		eSuperArrayParam::rand_offset_neg_y, _T("rand_offset_neg_y"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_NEG_Y,
		p_end,

		eSuperArrayParam::rand_offset_neg_z, _T("rand_offset_neg_z"), TYPE_BOOL, P_ANIMATABLE, IDS_OFFSET_NEGATIVE,
		p_default, TRUE,
		p_ui, eSuperArrayRollout::offset, TYPE_SINGLECHEKBOX, IDC_RAND_OFFSET_NEG_Z,
		p_end,

		// Min Random Rotation spinner
		eSuperArrayParam::rand_offset_min, _T("rand_offset_min"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSET_MIN,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, 0.0f, 9999.0f,
		p_ui, eSuperArrayRollout::offset, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_RAND_OFFSET_MIN_X, IDC_RAND_OFFSET_MIN_X_SPN, IDC_RAND_OFFSET_MIN_Y, IDC_RAND_OFFSET_MIN_Y_SPN, IDC_RAND_OFFSET_MIN_Z, IDC_RAND_OFFSET_MIN_Z_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RANDOFFSET_MIN,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Max Random Rotation spinner
		eSuperArrayParam::rand_offset_max, _T("rand_offset_max"), TYPE_POINT3, P_ANIMATABLE, IDS_OFFSET_MAX,
		p_default, Point3(0.0f, 0.0f, 0.0f),
		p_range, 0.0f, 9999.0f,
		p_ui, eSuperArrayRollout::offset, TYPE_SPINNER, EDITTYPE_POS_UNIVERSE, IDC_RAND_OFFSET_MAX_X, IDC_RAND_OFFSET_MAX_X_SPN, IDC_RAND_OFFSET_MAX_Y, IDC_RAND_OFFSET_MAX_Y_SPN, IDC_RAND_OFFSET_MAX_Z, IDC_RAND_OFFSET_MAX_Z_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RANDOFFSET_MAX,
		p_accessor, &superArrayPBAccessor,
		p_end,

		// Random Rotation distribution spinner
		eSuperArrayParam::rand_offset_dist, _T("rand_offset_dist"), TYPE_FLOAT, P_ANIMATABLE, IDS_DISTRIBUTION,
		p_default, 1.0f,
		p_range, 0.01f, 100.0f,
		p_ui, eSuperArrayRollout::offset, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_RAND_OFFSET_DISTRIBUTION, IDC_RAND_OFFSET_DISTRIBUTION_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_RAND_DISTRIBUTION,
		p_end,

		// Random offsetation seed spinner
		eSuperArrayParam::rand_offset_seed, _T("rand_offset_seed"), TYPE_INT, P_ANIMATABLE, IDS_SEED,
		p_default, 1,
		p_ui, eSuperArrayRollout::offset, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_RAND_OFFSET_SEED, IDC_RAND_OFFSET_SEED_SPN, SPIN_AUTOSCALE,
		p_tooltip, IDS_TIP_SEED,
		p_end,

	p_end // End rollouts
);