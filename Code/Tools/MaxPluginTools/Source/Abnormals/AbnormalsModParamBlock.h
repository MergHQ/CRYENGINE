#pragma once

namespace eAbnormalsModParamBlock
{
	enum Type
	{
		abnormals_param_block
	};
}

namespace eAbnormalsModRollout
{
	enum Type
	{
		channels = 0,
		general = 1,
		info = 3,
		auto_chamfer = 4,
	};
}

namespace eAbnormalsModParam
{
	enum Type
	{
		color_group_1,
		color_group_2,
		color_group_3,
		color_clear,
		load_auto,
		load_groups_from,
		load_groups_uv_channel,
		save_auto,
		save_groups_to,
		save_groups_uv_channel,
		show_normals,
		show_normals_size,
		load_legacy_colors,
		auto_chamfer,
		auto_chamfer_source,
		auto_chamfer_method,
		auto_chamfer_radius,
		auto_chamfer_segments,
		overlay_opacity,
		clear_smoothing,
	};
}
extern ParamBlockDesc2 abnormalsModParamBlockDesc;
