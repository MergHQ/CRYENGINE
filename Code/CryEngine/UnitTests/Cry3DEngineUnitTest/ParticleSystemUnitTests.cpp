// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <UnitTest.h>

#include <../../Cry3DEngine/StdAfx.h>

#include <ParticleSystem/ParticleSystem.h>
#include <ParticleSystem/ParticleContainer.h>


using namespace pfx2;

#define arraysize(array) (sizeof(array) / sizeof(array[0]))

void ParticleContainerRemovePreserveTest(size_t containerSz, TConstArray<TParticleId> toRemove, TConstArray<bool> doPreserve)
{
	// create particles with SpawnId
	TParticleHeap heap;
	pfx2::CParticleContainer container;
	{
		PUseData useData = NewUseData();
		useData->AddData(EPDT_SpawnId);
		container.SetUsedData(useData, EDD_Particle);
	}
	container.BeginSpawn();
	container.AddElements(containerSz);
	container.EndSpawn();

	IOPidStream spawnIds = container.IOStream(EPDT_SpawnId);
	for (auto i : container.FullRange())
		spawnIds[i] = i;

	// create child container with ParentId
	pfx2::CParticleContainer childContainer;
	{
		PUseData useData = NewUseData();
		useData->AddData(EPDT_ParentId);
		childContainer.SetUsedData(useData, EDD_Particle);
	}
	childContainer.BeginSpawn();
	childContainer.AddElements((containerSz + 1) / 2);
	childContainer.EndSpawn();

	IOPidStream parentIds = childContainer.IOStream(EPDT_ParentId);
	for (auto i : childContainer.FullRange())
		parentIds[i] = i * 2;

	// remove particles
	TParticleIdArray swapIds(heap, container.Size());
	container.RemoveElements(toRemove, doPreserve, swapIds);

	uint newCount = containerSz - toRemove.size();
	REQUIRE(container.FullRange().size() == newCount);

	spawnIds = container.IOStream(EPDT_SpawnId);

	// check if none of the particles are in the toRemove list
	for (auto i : container.FullRange())
	{
		for (auto r : toRemove)
		{
			REQUIRE(spawnIds[i] != r);
		}
	}

	// check that no ids are repeated
	for (auto i : container.FullRange())
	{
		for (auto j : container.FullRange())
		{
			if (i == j)
				continue;
			REQUIRE(spawnIds[i] != spawnIds[j]);
		}
	}

	// check that elements were archived
	if (doPreserve.size())
	{
		auto deadIndex = container.DeadRange().m_begin;
		for (auto id : toRemove)
		{
			if (doPreserve[id])
			{
				REQUIRE(deadIndex < container.DeadRange().m_end);
				uint spawnId = spawnIds[deadIndex++];
				REQUIRE(spawnId == id);
			}
		}
	}

	// check that SwapIds correspond to swapped SpawnIds
	for (TParticleId i = 0; i < containerSz; ++i)
	{ 
		TParticleId i1 = swapIds[i];
		if (i1 != gInvalidId)
		{
			TParticleId spawn = spawnIds[i1];
			REQUIRE(spawn == i);
		}
	}

	// check reparenting of child
	childContainer.Reparent(swapIds, EPDT_ParentId);
	for (auto i : childContainer.FullRange())
	{
		uint parentId0 = i * 2;
		bool removed = stl::find(toRemove, parentId0);
		bool archived = removed && doPreserve.size() && doPreserve[parentId0];

		uint parentId = parentIds[i];
		if (parentId == gInvalidId)
		{
			REQUIRE(removed && !archived);
		}
		else
		{
			REQUIRE(!removed || archived);
			REQUIRE(parentId < container.ExtendedSize());
			uint parentSpawnId = spawnIds[parentId];
			REQUIRE(parentSpawnId == parentId0);
		}
	}
}

void ParticleContainerRemoveTest(size_t containerSz, TConstArray<TParticleId> toRemove, TConstArray<TParticleId> toArchive)
{
	if (!toArchive.size())
		return ParticleContainerRemovePreserveTest(containerSz, toRemove, {});

	TParticleHeap heap;
	THeapArray<bool> doPreserve(heap);
	doPreserve.resize(containerSz, false);
	for (auto id : toArchive)
		doPreserve[id] = true;

	THeapArray<TParticleId> toRemoveFull(heap);
	toRemoveFull.reserve(toRemove.size() + toArchive.size());
	toRemoveFull.append(toRemove);
	toRemoveFull.append(toArchive);
	stl::sort(toRemoveFull, [](TParticleId id) { return id; });

	ParticleContainerRemovePreserveTest(containerSz, toRemoveFull, doPreserve);
}

TEST(CParticleContainerTest, Remove)
{
	gEnv->startProfilingSection = [](SProfilingSection*) -> SSystemGlobalEnvironment::TProfilerSectionEndCallback { return nullptr; };
	gEnv->recordProfilingMarker = [](SProfilingMarker*) {};

	{
		TParticleId toRemove[] = { 3, 4 };
		ParticleContainerRemoveTest(5, toRemove, {});
		ParticleContainerRemoveTest(5, {}, toRemove);
	}

	{
		TParticleId toRemove[] = { 8, 12 };
		ParticleContainerRemoveTest(13, toRemove, {});
		ParticleContainerRemoveTest(13, {}, toRemove);
	}

	{
		TParticleId toRemove[] = { 0, 4 };
		TParticleId toArchive[] = { 2 };
		ParticleContainerRemoveTest(5, toRemove, toArchive);
		ParticleContainerRemoveTest(5, toArchive, toRemove);
	}

	{
		TParticleId toRemove[] = { 4, 7, 9 };
		TParticleId toArchive[] = { 0, 8 };
		ParticleContainerRemoveTest(10, toRemove, toArchive);
		ParticleContainerRemoveTest(10, toArchive, toRemove);
	}

	{
		TParticleId toRemove[] = { 0, 1, 2, 4, 5, 6 };
		ParticleContainerRemoveTest(7, toRemove, {});
		ParticleContainerRemoveTest(7, {}, toRemove);
	}

	{
		TParticleId toRemove[] = { 0 };
		ParticleContainerRemoveTest(1, toRemove, {});
		ParticleContainerRemoveTest(1, {}, toRemove);
	}

	{
		TParticleId toRemove[] = { 0, 1, 2, 4 };
		TParticleId toArchive[] = { 3, 5 };
		ParticleContainerRemoveTest(6, toRemove, toArchive);
		ParticleContainerRemoveTest(6, toArchive, toRemove);
	}
}

#if CRY_PFX2_GROUP_STRIDE == 4

class CParticleContainerSpawnTest : public ::testing::Test
{
protected:
	virtual void SetUp() override
	{
		static_assert(CRY_PFX2_GROUP_STRIDE == 4, "This unit test is assuming vectorization of 4 particles");
		pContainer = std::unique_ptr<pfx2::CParticleContainer>(new pfx2::CParticleContainer());
		useData = NewUseData();
		useData->AddData(EPDT_ParentId);
		pContainer->SetUsedData(useData, EDD_Particle);
	}

	virtual void TearDown() override
	{
		pContainer.reset();
		useData.reset();
	};

	void AddParticles(uint32 count)
	{
		ResetSpawnedParticles();
		pContainer->AddElements(count);
	}

	void ResetSpawnedParticles()
	{
		pContainer->EndSpawn();
	}

	const pfx2::CParticleContainer& GetContainer() const
	{
		return *pContainer;
	}

	void ResetAll()
	{
		pContainer->Clear();
	}

private:
	PUseData useData;
	std::unique_ptr<pfx2::CParticleContainer> pContainer;
};

TEST_F(CParticleContainerSpawnTest, VectorizeSpawn)
{
	// PFX2_TODO : right now new born vectorization is a bit too simple. Should be profiled and tested later
	// particle memory should be segmented.

	const pfx2::CParticleContainer& container = GetContainer();
	AddParticles(6);

	AddParticles(3);
	REQUIRE(container.RealSize() == 9);
	REQUIRE(container.SpawnedRange().m_begin == 8);
	REQUIRE(container.NumSpawned() == 3);
	REQUIRE(container.Capacity() > 11);

	ResetSpawnedParticles();
	REQUIRE(container.Size() == 9);
	REQUIRE(container.SpawnedRange().m_begin == 6);
	REQUIRE(container.NumSpawned() == 3);
	REQUIRE(container.Capacity() > 9);
}

TEST_F(CParticleContainerSpawnTest, RealId)
{
	const pfx2::CParticleContainer& container = GetContainer();

	AddParticles(1);
	REQUIRE(container.RealId(0) == 0);
	ResetSpawnedParticles();

	ResetAll();
	AddParticles(6);
	AddParticles(3);
	REQUIRE(container.RealId(3) == 3);
	REQUIRE(container.RealId(8) == 8);
	REQUIRE(container.RealId(10) == 7);
	ResetSpawnedParticles();
	REQUIRE(container.RealId(3) == 3);
	REQUIRE(container.RealId(8) == 8);
	REQUIRE(container.RealId(6) == 6);
}

class ParticleAttributesTest : public ::testing::Test
{
protected:
	virtual void SetUp() override
	{
		gEnv->pNameTable = &m_nameTable;
	}

	CNameTable m_nameTable;
};

TEST_F(ParticleAttributesTest, All)
{
	auto pTable = std::make_shared<CAttributeTable>();
	CAttributeTable& table = *pTable.get();

	SAttributeDesc attr;
	attr.m_name = "Float";
	attr.m_defaultValue = -3.0f;
	attr.m_maxFloat = 1.0f;
	table.AddAttribute(attr);

	attr.m_name = "Bool";
	attr.m_defaultValue = false;
	table.AddAttribute(attr);

	attr.m_name = "Color";
	attr.m_defaultValue = ColorB(50, 100, 150, 255);
	table.AddAttribute(attr);

	REQUIRE(table.GetAttribute(1).GetType() == IParticleAttributes::ET_Boolean);

	CAttributeInstance inst;
	inst.Reset(pTable);

	REQUIRE(inst.GetNumAttributes() == 3);
	uint id = inst.FindAttributeIdByName("Color");
	REQUIRE(inst.GetAttributeType(id) == IParticleAttributes::ET_Color);
	REQUIRE(inst.GetAsBoolean(id, false) == true);
	REQUIRE(inst.GetAsInteger(id, 0) == 91);
	inst.SetAsFloat(id, 0.5f);
	REQUIRE(IsEquivalent(inst.GetAsFloat(id), 0.5f, 0.002f));

	id = inst.FindAttributeIdByName("Float");
	REQUIRE(inst.GetAsInteger(id, 0) == -3);

	inst.SetAsInteger(id, 2);
	REQUIRE(inst.GetAsFloat(id, 0.0f) == 1.0f);
	inst.ResetValue(id);
	REQUIRE(inst.GetAsFloat(id, 0.0f) == -3.0f);

	IParticleAttributes::TValue value;
	value.SetType(IParticleAttributes::ET_Color);
	REQUIRE(value.Type() == IParticleAttributes::ET_Color);

	auto type = table.GetAttribute(0).GetType();
	DO_FOR_ATTRIBUTE_TYPE(type, T, value = T());
};

#endif // #if CRY_PFX2_GROUP_STRIDE == 4

#ifdef CRY_PFX2_USE_SSE

TEST(ParticleSSE, IndexedLoad4)
{
	using namespace pfx2::detail;
	CRY_ALIGN(16) float stream[] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
	uint32v index;
	floatv expected, got;

	index = _mm_set1_epi32(2);
	expected = _mm_set_ps(2.0f, 2.0f, 2.0f, 2.0f);
	got = LoadIndexed4(stream, index, -1.0f);
	REQUIRE(All(got == expected));

	index = _mm_set_epi32(7, 6, 5, 4);
	expected = _mm_set_ps(7.0f, 6.0f, 5.0f, 4.0f);
	got = LoadIndexed4(stream, index, -1.0f);
	REQUIRE(All(got == expected));

	index = _mm_set_epi32(0, 1, 2, 0);
	expected = _mm_set_ps(0.0f, 1.0f, 2.0f, 0.0f);
	got = LoadIndexed4(stream, index, -1.0f);
	REQUIRE(All(got == expected));

	index = _mm_set_epi32(3, 4, 5, -1);
	expected = _mm_set_ps(3.0f, 4.0f, 5.0f, -1.0f);
	got = LoadIndexed4(stream, index, -1.0f);
	REQUIRE(All(got == expected));

	index = _mm_set_epi32(-1, -1, -1, -1);
	expected = _mm_set_ps(-2.0f, -2.0f, -2.0f, -2.0f);
	got = LoadIndexed4(stream, index, -2.0f);
	REQUIRE(All(got == expected));

	index = _mm_set_epi32(9, 9, 2, 2);
	expected = _mm_set_ps(9.0f, 9.0f, 2.0f, 2.0f);
	got = LoadIndexed4(stream, index, -1.0f);
	REQUIRE(All(got == expected));
}


TEST(CryVectorTest, ChaosKeyTest)
{
	CRndGen randomGenerator;
	randomGenerator.SetState(time(0));

	for (int rep = 0; rep < 8; ++rep)
	{
		uint32 key = randomGenerator.GetRandom(0U, 99999U);
		SChaosKey ch(key);
		SChaosKeyV chv(convert<uint32v>(key));

		uint32 r = ch.Rand();
		uint32v rv = chv.Rand();
		REQUIRE(All(convert<uint32v>(r) == rv));

		float ss = ch.RandSNorm();
		floatv ssv = chv.RandSNorm();
		REQUIRE(All(ToFloatv(ss) == ssv));
	}
}

#endif // #ifdef CRY_PFX2_USE_SSE
