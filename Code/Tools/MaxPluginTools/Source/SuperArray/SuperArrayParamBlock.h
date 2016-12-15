#pragma once

#include "iparamb2.h"

namespace eSuperArrayParamBlock
{
	enum Type
	{
		super_array_param_block
	};
}

namespace eSuperArrayRollout
{
	enum Type
	{
		general = 0,
		rotation,
		offset,
		scale,
	};
}

namespace eSuperArrayParam
{
	enum Type
	{
		master_seed = 0,
		copy_type,
		copy_count,
		copy_direction,
		copy_distance,
		copy_distance_exact,
		node_pool,
		pool_seed,
		spacing_type,
		spacing_pivot,
		spacing_bounds,
		incremental_rot,
		rand_rot_pos_x,
		rand_rot_pos_y,
		rand_rot_pos_z,
		rand_rot_neg_x,
		rand_rot_neg_y,
		rand_rot_neg_z,
		rand_rot_max,
		rand_rot_min,
		rand_rot_dist,
		rand_rot_seed,
		rand_flip_x,
		rand_flip_y,
		rand_flip_z,
		rand_flip_seed,
		rand_scale_max,
		rand_scale_min,
		rand_scale_dist,
		rand_scale_seed,
		rand_offset_pos_x,
		rand_offset_pos_y,
		rand_offset_pos_z,
		rand_offset_neg_x,
		rand_offset_neg_y,
		rand_offset_neg_z,
		rand_offset_min,
		rand_offset_max,
		rand_offset_dist,
		rand_offset_seed,
		base_rot,
	};
}
extern ParamBlockDesc2 superArrayParamBlockDesc;