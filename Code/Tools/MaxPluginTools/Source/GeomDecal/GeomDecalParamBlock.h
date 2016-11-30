#pragma once

#include "iparamb2.h"

namespace eGeomDecalParamBlock
{
	enum Type
	{
		geom_decal_param_block
	};
}

namespace eGeomDecalRollout
{
	enum Type
	{
		general = 0,
	};
}

namespace eGeomDecalParam
{
	enum Type
	{
		projection_size = 0,
		node_pool,
		uv_tiling,
		uv_offset,
		mesh_push,
		mesh_matid,
		projection_anglecutoff,
	};
}
extern ParamBlockDesc2 GeomDecalParamBlockDesc;