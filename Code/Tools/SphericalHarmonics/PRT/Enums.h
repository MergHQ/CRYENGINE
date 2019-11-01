
#pragma once

namespace NSH
{
	//!< return value for return element calls of ray casting sinks
	typedef enum EReturnSinkValue
	{
		RETURN_SINK_HIT = 0,							 //!< ray has hit
		RETURN_SINK_FAIL = 1,							 //!< ray has failed	
		RETURN_SINK_HIT_AND_EARLY_OUT = 2	 //!< ray has hit and no further hit tests are required
	}EReturnSinkValue;
}
