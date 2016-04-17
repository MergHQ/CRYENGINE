/*
	transfer parameter implementations
*/
inline NSH::NTransfer::STransferParameters::STransferParameters() : 
				backlightingColour(0,0,0),
				groundPlaneBlockValue(1.0f),
				rayTracingBias(0.05f), 
				minDirectBumpCoeffVisibility(0.f),
				sampleCountPerVertex(-1), 
				configType(NSH::NTransfer::TRANSFER_CONFIG_DEFAULT),
				bumpGranularity(false), 
				supportTransparency(false),
				transparencyShadowingFactor(1.0f),
				matrixTargetCS(MATRIX_HEURISTIC_MAX),
				rayCastingThreads(1),
				compressToByte(false)
{}	

