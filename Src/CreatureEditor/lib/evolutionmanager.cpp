
#include "stdafx.h"
#include "evolutionmanager.h"


cEvolutionManager::cEvolutionManager()
	: m_state(eState::Wait)
	, m_curEpochSize(0)
	, m_incT(0.f)
{
}

cEvolutionManager::~cEvolutionManager()
{
}


bool cEvolutionManager::Init()
{

	return true;
}


// start evolution simuation
bool cEvolutionManager::RunEvolution(const sEvolutionParam &param)
{
	if (eState::Run == m_state)
		return false; // already run

	m_state = eState::Run;
	m_param = param;
	m_curEpochSize = 0;
	m_genetic.InitGenome();
	
	g_pheno->m_generationCnt = param.generation;
	SpawnCreatures();

	return true;
}


// stop evolution simulation
bool cEvolutionManager::StopEvolution()
{
	if (eState::Wait == m_state)
		return false; // already stop

	m_state = eState::Wait;
	g_pheno->ClearCreature();
	m_incT = 0.f;
	return true;
}


// check creature
// epoch next generation
bool cEvolutionManager::Update(const float deltaSeconds)
{
	RETV(m_state == eState::Wait, false);
	m_incT += deltaSeconds;

	// after 5 second, measure creature fitness
	if (m_creatures.empty() && (m_incT > 5.f))
	{
		for (auto &creature : g_pheno->m_creatures)
		{
			if (creature->m_nodes.empty())
				continue;

			sCreatureData data;
			data.spawnPos = creature->m_nodes[0]->m_node->m_transform.pos;
			m_creatures[creature] = data;
		}
	}

	// next generation
	if (m_incT > m_param.epochTime)
	{
		for (auto &creature : g_pheno->m_creatures)
		{
			if (creature->m_nodes.empty())
				continue;
			auto it = m_creatures.find(creature);
			if (m_creatures.end() == it)
				continue; // error occurred!!

			ai::sGenome genome = creature->GetGenome();
			if (genome.chromo.empty())
				continue; // error occurred!!

			// ignore y axis
			const Vector3 p0 = it->second.spawnPos;
			const Vector3 p1 = creature->m_nodes[0]->m_node->m_transform.pos;
			const float dist = Vector3(p0.x, 0.f, p0.z).Distance(Vector3(p1.x, 0, p1.z));
			genome.fitness = dist;
			
			m_genetic.AddGenome(genome);
		}

		m_genetic.Epoch();

		g_pheno->ClearCreature();
		SpawnCreatures(m_genetic.GetGenomes());

		++m_curEpochSize;
		m_incT = 0.f;
	}

	return true;
}


// spawn creatures
void cEvolutionManager::SpawnCreatures(
	const vector<ai::sGenome> &genomes //=vector<ai::sGenome>()
)
{
	const float width = 180.f;
	const float height = 180.f;
	uint div = 1;
	float xgap = width / (float)m_param.spawnSize;
	if (xgap < 10.f)
	{
		div = (int)sqrt((float)m_param.spawnSize);
		xgap = width / div;
	}

	const int spawnFlags = cPhenoTypeManager::eSpawnFlag::Unlock
		| cPhenoTypeManager::eSpawnFlag::UnSelect;
	for (uint i = 0; i < m_param.spawnSize; ++i)
	{
		Vector3 pos;
		if (div == 1)
		{
			pos = Vector3(i * xgap - 90.f, 1.f, 0.f);
		}
		else
		{
			uint x = i % div;
			uint z = i / div;
			pos = Vector3(x * xgap - 90.f, 1.f, z * xgap - 90.f);
		}

		evc::cCreature *creature = g_pheno->ReadCreatureFile(
			m_param.creatureFileName, pos, spawnFlags);
		if (creature && (i < genomes.size()))
			creature->SetGenome(genomes[i]);
	}

}
