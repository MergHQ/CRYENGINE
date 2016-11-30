#include "stdafx.h"
#include "SuperArray.h"

#include "SuperArrayClassDesc.h"
#include "SuperArrayCreateCallback.h"
#include "SuperArrayParamBlock.h"
#include "SuperArrayDlgProcs.h"

IObjParam* SuperArray::ip = NULL;

SuperArray::SuperArray(BOOL loading) : SimpleObject2()
{
	SuperArrayClassDesc::GetInstance()->MakeAutoParamBlocks(this);
}

SuperArray::~SuperArray()
{
	ClearObjectPool();
}

void SuperArray::UpdateObjectPool(TimeValue t)
{
	ClearObjectPool();

	const int nodeCount = pblock2->Count(eSuperArrayParam::node_pool);
	for (int i = 0; i < nodeCount; i++)
	{
		INode* node;
		pblock2->GetValue(eSuperArrayParam::node_pool, t, node, ivalid, i);

		if (node != NULL)
		{
			ObjectState os = node->EvalWorldState(t);

			if (os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
			{
				GeomObject* obj = (GeomObject*)os.obj;
				TriObject* tri = (TriObject *)obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));

				// Ref tri mesh in pool.
				objectPool.append(tri);
				// We are responsible for deleting tri meshes if they don't equal our object.
				objectsNeedCleanup.append(obj != tri);
			}
		}
	}
}

void SuperArray::ClearObjectPool()
{
	// Clean up tri objects we're responsible for deleting (set in UpdateObjectPool)
	for (int i = 0; i < objectsNeedCleanup.length(); i++)
	{
		if (objectsNeedCleanup[i])
		{
			delete objectPool[i];
			// Using DeleteMe() as shown in documentation examples does not work, and causes a memory leak.
			// http://help.autodesk.com/view/3DSMAX/2016/ENU/?guid=__files_GUID_B2693B67_F56D_4EEB_9FB8_19700D7BAB90_htm
			//objectPool[i]->DeleteMe();
		}
	}

	objectPool.removeAll();
	objectsNeedCleanup.removeAll();
}

void SuperArray::UpdateMemberVarsFromParamBlock(TimeValue t)
{
	ivalid = FOREVER; // Start with valid forever, then widdle it down as we access pblock values.

	// General
	pblock2->GetValue(eSuperArrayParam::master_seed, t, masterSeed, ivalid);
	pblock2->GetValue(eSuperArrayParam::copy_type, t, copyType, ivalid);
	pblock2->GetValue(eSuperArrayParam::copy_distance, t, copyDistance, ivalid);
	pblock2->GetValue(eSuperArrayParam::copy_distance_exact, t, copyDistanceExact, ivalid);
	pblock2->GetValue(eSuperArrayParam::copy_count, t, copyCount, ivalid);
	pblock2->GetValue(eSuperArrayParam::copy_direction, t, copyDirection, ivalid);
	pblock2->GetValue(eSuperArrayParam::spacing_type, t, spacingType, ivalid);
	pblock2->GetValue(eSuperArrayParam::spacing_pivot, t, spacingPivot, ivalid);
	pblock2->GetValue(eSuperArrayParam::spacing_bounds, t, spacingBounds, ivalid);
	UpdateObjectPool(t);
	pblock2->GetValue(eSuperArrayParam::pool_seed, t, poolSeed, ivalid);

	// --- Rotation ---
	pblock2->GetValue(eSuperArrayParam::base_rot, t, baseRotation, ivalid);
	pblock2->GetValue(eSuperArrayParam::incremental_rot, t, incrementalRotation, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_pos_x, t, randomRotationPositive[0], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_pos_y, t, randomRotationPositive[1], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_pos_z, t, randomRotationPositive[2], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_neg_x, t, randomRotationNegative[0], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_neg_y, t, randomRotationNegative[1], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_neg_z, t, randomRotationNegative[2], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_min, t, randomRotationMin, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_max, t, randomRotationMax, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_dist, t, randomRotationDistribution, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_rot_seed, t, randomRotationSeed, ivalid);

	// Convert degrees to radians.
	baseRotation = DegToRad(baseRotation);
	incrementalRotation = DegToRad(incrementalRotation);
	randomRotationMin = DegToRad(randomRotationMin);
	randomRotationMax = DegToRad(randomRotationMax);

	// --- Scale ---
	pblock2->GetValue(eSuperArrayParam::rand_scale_min, t, randomScaleMin, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_scale_max, t, randomScaleMax, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_scale_dist, t, randomScaleDistribution, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_scale_seed, t, randomScaleSeed, ivalid);

	pblock2->GetValue(eSuperArrayParam::rand_flip_x, t, randomFlipX, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_flip_y, t, randomFlipY, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_flip_z, t, randomFlipZ, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_flip_seed, t, randomFlipSeed, ivalid);

	// --- Offset ---
	pblock2->GetValue(eSuperArrayParam::rand_offset_pos_x, t, randomOffsetPositive[0], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_pos_y, t, randomOffsetPositive[1], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_pos_z, t, randomOffsetPositive[2], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_neg_x, t, randomOffsetNegative[0], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_neg_y, t, randomOffsetNegative[1], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_neg_z, t, randomOffsetNegative[2], ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_min, t, randomOffsetMin, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_max, t, randomOffsetMax, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_dist, t, randomOffsetDistribution, ivalid);
	pblock2->GetValue(eSuperArrayParam::rand_offset_seed, t, randomOffsetSeed, ivalid);
}

void SuperArray::UpdateControls(HWND hWnd, eSuperArrayRollout::Type rollout)
{
	switch (rollout)
	{
	case eSuperArrayRollout::general:
	{
		// Copy type
		const bool copyCountEnabled = copyType == eCopyType::count;
		EnableWindow(GetDlgItem(hWnd, IDC_COPIES_COUNT), copyCountEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_COPIES_COUNT_SPN), copyCountEnabled);

		const bool copyDistanceEnabled = copyType == eCopyType::distance;
		EnableWindow(GetDlgItem(hWnd, IDC_COPIES_DIST), copyDistanceEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_COPIES_DIST_SPN), copyDistanceEnabled);
		EnableWindow(GetDlgItem(hWnd, IDC_COPIES_DIST_EXACT), copyDistanceEnabled);

		// Spacing
		const bool pivotSpacing = spacingType == eSpacingType::pivot;
		EnableWindow(GetDlgItem(hWnd, IDC_SPACING_PIVOT), pivotSpacing);
		EnableWindow(GetDlgItem(hWnd, IDC_SPACING_PIVOT_SPN), pivotSpacing);

		const bool boundsSpacing = spacingType == eSpacingType::bounds;
		EnableWindow(GetDlgItem(hWnd, IDC_SPACING_BOUNDS), boundsSpacing);
		EnableWindow(GetDlgItem(hWnd, IDC_SPACING_BOUNDS_SPN), boundsSpacing);

		break;
	}
	case eSuperArrayRollout::offset:
	{
		const bool randXOffsetMatters = randomOffsetPositive[0] || randomOffsetNegative[0];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_X), randXOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_X_SPN), randXOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_X), randXOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_X_SPN), randXOffsetMatters);

		const bool randYOffsetMatters = randomOffsetPositive[1] || randomOffsetNegative[1];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_Y), randYOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_Y_SPN), randYOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_Y), randYOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_Y_SPN), randYOffsetMatters);

		const bool randZOffsetMatters = randomOffsetPositive[2] || randomOffsetNegative[2];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_Z), randZOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MAX_Z_SPN), randZOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_Z), randZOffsetMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_MIN_Z_SPN), randZOffsetMatters);

		const bool somethingMatters = randXOffsetMatters || randYOffsetMatters || randZOffsetMatters;
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_DISTRIBUTION), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_DISTRIBUTION_SPN), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_SEED), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_OFFSET_SEED_SPN), somethingMatters);

		break;
	}
	case eSuperArrayRollout::rotation:
	{
		const bool randXRotationMatters = randomRotationPositive[0] || randomRotationNegative[0];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_X), randXRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_X_SPN), randXRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_X), randXRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_X_SPN), randXRotationMatters);

		const bool randYRotationMatters = randomRotationPositive[1] || randomRotationNegative[1];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_Y), randYRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_Y_SPN), randYRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_Y), randYRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_Y_SPN), randYRotationMatters);

		const bool randZRotationMatters = randomRotationPositive[2] || randomRotationNegative[2];
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_Z), randZRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MAX_Z_SPN), randZRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_Z), randZRotationMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_MIN_Z_SPN), randZRotationMatters);

		const bool somethingMatters = randXRotationMatters || randYRotationMatters || randZRotationMatters;
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_DISTRIBUTION), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_DISTRIBUTION_SPN), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_SEED), somethingMatters);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_ROT_SEED_SPN), somethingMatters);

		break;
	}
	case eSuperArrayRollout::scale:
	{
		bool flipAnyAxis = randomFlipX || randomFlipY || randomFlipZ;
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_FLIP_SEED), flipAnyAxis);
		EnableWindow(GetDlgItem(hWnd, IDC_RAND_FLIP_SEED_SPN), flipAnyAxis);

		break;
	}
	default:
		break;
	}
}

void SuperArray::SeedRandoms()
{
	// Creat and seed a master random to skew seeds.
	Random masterRandom = Random();
	masterRandom.srand(masterSeed);

	// Seed all randoms.
	meshRandom.srand(poolSeed + masterRandom.get());
	rotationRandom.srand(randomRotationSeed + masterRandom.get());
	scaleRandom.srand(randomScaleSeed + masterRandom.get());
	flipRandom.srand(randomFlipSeed + masterRandom.get());
	offsetRandom.srand(randomOffsetSeed + masterRandom.get());
}

Point3 SuperArray::GetRandomRotation()
{
	Point3 eulerRotation = Point3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 3; i++)
	{
		const float randFloat = rotationRandom.getf(); // Get a random value from 0-1
		const bool invertRotation = rotationRandom.get() % 2 == 0; // Important to call this rand even if it's not used, so the angles aren't affected by the pos or neg selection.											   
		
		if (!randomRotationNegative[i] && !randomRotationPositive[i])
			continue;

		// Skew it based on the distribution
		eulerRotation[i] = pow(randFloat, randomRotationDistribution);

		// Bring it into the range of angles specified.
		eulerRotation[i] = (eulerRotation[i] * (randomRotationMax[i] - randomRotationMin[i])) + randomRotationMin[i];

		// Invert this axis' rotation if only negative rotation is specified, or if both positive, negative, and the random are specified, and the random value has come out true.		
		if ((randomRotationNegative[i] && !randomRotationPositive[i]) || (randomRotationPositive[i] && randomRotationNegative[i] && invertRotation))
			eulerRotation[i] *= -1;
	}

	return eulerRotation;
}

Point3 SuperArray::GetRandomScale()
{
	Point3 returnScale = Point3(1.0f, 1.0f, 1.0f);

	for (int i = 0; i < 3; i++)
	{
		// Get a random value from 0-1
		returnScale[i] = scaleRandom.getf();

		// Skew it based on the distribution
		returnScale[i] = pow(returnScale[i], randomScaleDistribution);

		// Bring it into the range specified.
		returnScale[i] = (returnScale[i] * (randomScaleMax[i] - randomScaleMin[i])) + randomScaleMin[i];
	}

	// Invert axis scales if specified. Important to check random before the user selection so the .get() is always called for consistent random generation.
	if ((flipRandom.get() % 2 == 0) && randomFlipX)
		returnScale.x *= -1;

	if ((flipRandom.get() % 2 == 0) && randomFlipY)
		returnScale.y *= -1;

	if ((flipRandom.get() % 2 == 0) && randomFlipZ)
		returnScale.z *= -1;

	return returnScale;
}

Point3 SuperArray::GetRandomOffset()
{
	Point3 offset = Point3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 3; i++)
	{
		// Important to call these rands even if not used, so as to not change the outcome of subsequent axis.
		const float randFloat = offsetRandom.getf();
		const bool invertOffset = offsetRandom.get() % 2 == 0;

		// If we're not doing a positive, or a negative offset, just continue (important to be after rand calls)
		if (!randomOffsetPositive[i] && !randomOffsetNegative[i])
			continue;

		// Skew 0-1 value based on the distribution
		offset[i] = pow(randFloat, randomOffsetDistribution);

		// Bring it into the range specified.
		offset[i] = (offset[i] * (randomOffsetMax[i] - randomOffsetMin[i])) + randomOffsetMin[i];

		// Invert this axis' offset if only negative offset is specified, or if both positive, negative, and the random are specified, and the random value has come out true.
		if ((randomOffsetNegative[i] && !randomOffsetPositive[i]) || (randomOffsetPositive[i] && randomOffsetNegative[i] && invertOffset))
			offset[i] *= -1;
	}

	return offset;
}

// The main function where we actually do the creation of the mesh.
void SuperArray::BuildMesh(TimeValue t)
{
	// Reset current mesh
	mesh = Mesh();

	UpdateMemberVarsFromParamBlock(t);

	if (objectPool.length() <= 0)
		return;

	SeedRandoms();

	float previousBasePos = 0.0f;

	for (int i = 0; i < SAFETY_MAX_COPIES; i++)
	{
		TriObject* triObject = objectPool[meshRandom.rand() % objectPool.length()];

		Mesh* currentMesh = &objectPool[meshRandom.rand() % objectPool.length()]->mesh;

		Point3 scale = GetRandomScale();
		Matrix3 scaleMatrix = Matrix3();
		scaleMatrix.SetScale(scale);

		Point3 rotation = incrementalRotation * i + baseRotation + GetRandomRotation();
		Matrix3 rotationMatrix = Matrix3();
		rotationMatrix.SetRotate(rotation.x, rotation.y, rotation.z);

		Matrix3 rotationScaleMatrix = scaleMatrix * rotationMatrix;

		Point3 transformOffset = Point3(0, 0, 0);

		if (i > 0) // Don't apply spacing to the first mesh.
		{
			// Offset from the previous mesh's location
			if (spacingType == eSpacingType::bounds)
				transformOffset[copyDirection] = mesh.getBoundingBox().Max()[copyDirection] + spacingBounds;
			else if (spacingType == eSpacingType::pivot)
				transformOffset[copyDirection] = previousBasePos + spacingPivot;
		}

		if (spacingType == eSpacingType::bounds) // For bounds spacing, additionally offset by size of bounds (after rotation & scale)
			transformOffset[copyDirection] += -currentMesh->getBoundingBox(&rotationScaleMatrix).Min()[copyDirection];

		// Save previous position before applying random offset
		previousBasePos = transformOffset[copyDirection];

		// Apply random offset
		transformOffset += GetRandomOffset();

		Matrix3 translateMatrix = Matrix3();
		translateMatrix.SetTranslate(transformOffset);

		Matrix3 TransformMatrix = rotationScaleMatrix * translateMatrix;

		// Create a duplicate of the current mesh, transform it, then combine it with the existing mesh.
		Mesh tempMesh = Mesh();
		Mesh duplicate = Mesh(*currentMesh);

		// Transform verts
		for (int v = 0; v < duplicate.numVerts; v++)
			duplicate.verts[v] = duplicate.verts[v] * TransformMatrix;

		// If we have been mirrored, flip normals.
		if (scale.x < 0 ^ scale.y < 0 ^ scale.z < 0)
		{
			for (int f = 0; f < duplicate.numFaces; f++)
			{
				duplicate.FlipNormal(f);
			}
		}

		// Transform normals.
		const MeshNormalSpec* normalSpec = duplicate.GetSpecifiedNormals();
		if (normalSpec != NULL)
		{
			Point3* normals = normalSpec->GetNormalArray();

			for (int n = 0; n < normalSpec->GetNumNormals(); n++)
				normals[n] = (normals[n] * rotationScaleMatrix).Normalize();
		}

		CombineMeshes(tempMesh, mesh, duplicate);
		mesh = tempMesh;

		// Determine if we need to continue
		if (copyType == eCopyType::distance)
		{
			if (mesh.getBoundingBox().Max()[copyDirection] >= copyDistance)
				break;
		}
		else if (copyType == eCopyType::count)
		{
			if (i + 1 >= copyCount)
				break;
		}
		else // shouldn't happen
			break;
	}

	// Scale the mesh exactly to the distance
	if (copyType == eCopyType::distance && copyDistanceExact)
	{
		float currentLength = mesh.getBoundingBox().Width()[copyDirection];

		float desiredScale = 1 / (currentLength / copyDistance);

		Matrix3 scaleMatrix = Matrix3(TRUE);
		scaleMatrix.SetScale(Point3(desiredScale, 1.0f, 1.0f));

		for (int v = 0; v < mesh.numVerts; v++)
		{
			mesh.verts[v] = mesh.verts[v] * scaleMatrix;
		}

		const MeshNormalSpec* normalSpec = mesh.GetSpecifiedNormals();
		if (normalSpec != NULL)
		{
			Point3* normals = normalSpec->GetNormalArray();

			for (int n = 0; n < normalSpec->GetNumNormals(); n++)
				normals[n] = (normals[n] * scaleMatrix).Normalize();
		}
	}
}

void SuperArray::InvalidateUI()
{
	superArrayParamBlockDesc.InvalidateUI();
}

CreateMouseCallBack* SuperArray::GetCreateMouseCallBack()
{
	static SuperArrayCreateCallBack createCallback;
	createCallback.SetObj(this);
	return &createCallback;
}

RefTargetHandle SuperArray::Clone(RemapDir& remap)
{
	SuperArray* newob = new SuperArray(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}

void SuperArray::BeginEditParams(IObjParam * ip, ULONG flags, Animatable * prev)
{
	this->ip = ip;

	SimpleObject2::BeginEditParams(ip, flags, prev);

	SuperArrayClassDesc::GetInstance()->BeginEditParams(ip, this, flags, prev);

	superArrayParamBlockDesc.SetUserDlgProc(eSuperArrayRollout::general, new SuperArrayGeneralDlgProc(this));
	superArrayParamBlockDesc.SetUserDlgProc(eSuperArrayRollout::offset, new SuperArrayOffsetDlgProc(this));
	superArrayParamBlockDesc.SetUserDlgProc(eSuperArrayRollout::rotation, new SuperArrayRotationDlgProc(this));
	superArrayParamBlockDesc.SetUserDlgProc(eSuperArrayRollout::scale, new SuperArrayScaleDlgProc(this));
}

void SuperArray::EndEditParams(IObjParam * ip, ULONG flags, Animatable * next)
{
	SimpleObject2::EndEditParams(ip, flags, next);

	SuperArrayClassDesc::GetInstance()->EndEditParams(ip, this, flags, next);

	ip->ClearPickMode();
	this->ip = NULL;
}