
#include "stdafx.h"
#include "evolutionmanager.h"


cEvolutionManager::cEvolutionManager()
	: m_state(eState::Wait)
	, m_curEpochSize(0)
	, m_incT(0.f)
	, m_graphIdx(0)
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
	m_creatures.clear();
	m_graphIdx = 0;
	ZeroMemory(m_fitnessGraph, sizeof(m_fitnessGraph));
	ZeroMemory(m_avrGraph, sizeof(m_avrGraph));
	ZeroMemory(m_grabBestAvrGraph, sizeof(m_grabBestAvrGraph));
	
	g_pheno->m_generationCnt = param.generation;
	SpawnCreatures();
	SaveCreatureGenomes();
	return true;
}


// stop evolution simulation
bool cEvolutionManager::StopEvolution()
{
	if (eState::Wait == m_state)
		return false; // already stop

	m_state = eState::Wait;
	g_pheno->ClearCreature();
	g_nn->SetCurrentCreature(nullptr);
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
		m_genetic.InitGenome();

		double avr = 0.f;
		double bestFit = FLT_MIN;
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

			// fitness graph
			avr += dist;
			if (bestFit < genome.fitness)
				bestFit = genome.fitness;
			
			m_genetic.AddGenome(genome);
		}

		m_fitnessGraph[m_graphIdx] = (float)bestFit;
		m_avrGraph[m_graphIdx] = (float)(avr / g_pheno->m_creatures.size());

		m_genetic.Epoch(m_param.grabBestFit); // next generation

		double bestAvr = 0.f;
		for (uint i = 0; i < m_param.grabBestFit; ++i)
			bestAvr += m_genetic.m_genomes[i].fitness;

		if (m_param.grabBestFit > 0)
			m_grabBestAvrGraph[m_graphIdx] = (float)(bestAvr / (double)m_param.grabBestFit);
		m_graphIdx = (m_graphIdx + 1) % ARRAYSIZE(m_fitnessGraph);

		// finish evolution?
		if (m_curEpochSize >= m_param.epochSize)
		{
			m_state = eState::Wait;
			return true;
		}

		m_creatures.clear();
		g_pheno->ClearCreature();
		SpawnCreatures(m_genetic.GetGenomes());

		// temporal selection first creature
		if (g_nn->m_creature)
			g_nn->SetCurrentCreature(g_pheno->m_creatures.empty()?
				nullptr : g_pheno->m_creatures[0]);

		++m_curEpochSize;
		m_incT = 0.f;

		SaveCreatureGenomes();
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


// save creature genomes
bool cEvolutionManager::SaveCreatureGenomes()
{
	StrPath fileName;
	fileName.Format("%s%s_%s_%d.gen"
		, g_evolutionResourcePath.c_str()
		, common::GetCurrentDateTime().c_str()
		, m_param.creatureFileName.GetFileNameExceptExt().c_str()
		, m_curEpochSize);

	evc::cGenome allGenome;
	allGenome.m_name = m_param.creatureFileName.GetFileNameExceptExt().c_str();
	allGenome.m_dnas.reserve(m_param.spawnSize);
	for (auto &creature : g_pheno->m_creatures)
	{
		ai::cNeuralNet *nn = creature->GetNeuralNetwork();
		if (!nn)
			continue;

		evc::cGenome::sDna dna;
		dna.layerCnt = nn->m_layers.size();
		dna.inputCnt = nn->m_numInputs;
		dna.outputCnt = nn->m_numOutputs;
		dna.fitness = 0.f;
		dna.chromo = nn->GetWeights();
		allGenome.m_dnas.push_back(dna);
	}
	allGenome.Write(fileName);

	return true;
}
