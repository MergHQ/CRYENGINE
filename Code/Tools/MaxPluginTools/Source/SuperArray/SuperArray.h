#pragma once

#include "SuperArrayParamBlock.h"

#define SUPER_ARRAY_CLASS_NAME "SuperArray"
#define SUPER_ARRAY_CATEGORY "Crytek"
#define SUPER_ARRAY_CLASS_ID Class_ID(0x431b432b, 0x49443198)

#define SAFETY_MAX_COPIES 100 // A limit on the number of copies, just incase something goes wrong.

namespace eSpacingType
{
	enum Type
	{
		bounds = 0,
		pivot = 1
	};
}

namespace eCopyType
{
	enum Type
	{
		distance = 0,
		count = 1
	};
}

class SuperArray : public SimpleObject2
{
public:
	SuperArray(BOOL loading);
	virtual ~SuperArray();

	static IObjParam *ip; //Access to the interface

	// Instance variables for param block values.

	// General
	int masterSeed;
	int copyType;
	float copyDistance;
	BOOL copyDistanceExact;
	int copyCount;
	int copyDirection;
	int spacingType;
	float spacingPivot;
	float spacingBounds;
	MaxSDK::Array<TriObject*> objectPool;
	MaxSDK::Array<BOOL> objectsNeedCleanup;
	int poolSeed;

	// Rotation
	Point3 baseRotation;
	Point3 incrementalRotation;
	BOOL randomRotationPositive[3];
	BOOL randomRotationNegative[3];
	Point3 randomRotationMin;
	Point3 randomRotationMax;
	float randomRotationDistribution;
	int randomRotationSeed;

	// Scale	
	Point3 randomScaleMin;
	Point3 randomScaleMax;
	float randomScaleDistribution;
	int randomScaleSeed;
	BOOL randomFlipX;
	BOOL randomFlipY;
	BOOL randomFlipZ;
	int randomFlipSeed;

	// Offset
	BOOL randomOffsetPositive[3];
	BOOL randomOffsetNegative[3];
	Point3 randomOffsetMin;
	Point3 randomOffsetMax;
	float randomOffsetDistribution;
	int randomOffsetSeed;

	// Random generators; using multiple randoms so changes to one setting don't affect another (for example, changing number of clones wont affect random rotations)
	Random meshRandom; // For selecting random mesh from pool
	Random scaleRandom; // For generating random scales
	Random flipRandom;
	Random offsetRandom;
	Random rotationRandom; // For generating random rotations

	void UpdateObjectPool(TimeValue t);
	void ClearObjectPool();
	void UpdateMemberVarsFromParamBlock(TimeValue t);
	void UpdateControls(HWND hWnd, eSuperArrayRollout::Type rollout);
	void SeedRandoms();
	Point3 GetRandomRotation();
	Point3 GetRandomScale();
	Point3 GetRandomOffset();

	// ---- From BaseObject ----
	virtual const TCHAR* GetObjectName() override { return GetString(IDS_SUPERARRAY); }

	// ---- From SimpleObject ----	
	virtual void BuildMesh(TimeValue t) override;
	virtual void InvalidateUI() override;
	virtual CreateMouseCallBack* GetCreateMouseCallBack() override;

	// ---- From RefTargetHandle ----
	virtual RefTargetHandle Clone(RemapDir& remap) override;

	// ---- From Animateable ----
	Class_ID ClassID() { return SUPER_ARRAY_CLASS_ID; }
	SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_SUPERARRAY); }
	virtual void BeginEditParams (IObjParam *ip, ULONG flags, Animatable *prev) override;
	virtual void EndEditParams (IObjParam *ip, ULONG flags, Animatable *next) override;	

	// ---- From BaseObject ----
	int	NumParamBlocks() override { return 1; }	// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int i) override { return pblock2; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) override
	{ 
		return (pblock2->ID() == id) ? pblock2 : NULL; // return id'd ParamBlock
	} 
};