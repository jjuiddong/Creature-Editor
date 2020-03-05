
#include "stdafx.h"
#include "genomemanager.h"


cGenomeManager::cGenomeManager()
{
}

cGenomeManager::~cGenomeManager()
{
}


bool cGenomeManager::Init()
{

	return true;
}


// read genome file, and update select creature genome
// update phenotype view
// return update creature count
uint cGenomeManager::SetGenomeSelectCreature(const StrPath &fileName)
{
	evc::cGenome genome;
	if (!genome.Read(fileName))
		return 0;
	if (genome.m_dnas.empty())
		return 0;
	if (g_pheno->m_selects.empty())
		return 0;

	set<evc::cCreature*> creatures;
	for (auto &id : g_pheno->m_selects)
	{
		evc::cCreature *creature = g_pheno->FindCreatureContainNode(id);
		if (!creature)
			continue;
		creatures.insert(creature);
	}

	return SetGenomeSelectCreature(creatures, genome);
}


// update genome select creature
// update phenotype view
// return update creature count
uint cGenomeManager::SetGenomeSelectCreature(const evc::cGenome &genome
	, const uint dnaIdx //= 0
)
{
	set<evc::cCreature*> creatures;
	for (auto &id : g_pheno->m_selects)
	{
		evc::cCreature *creature = g_pheno->FindCreatureContainNode(id);
		if (!creature)
			continue;
		creatures.insert(creature);
	}

	return SetGenomeSelectCreature(creatures, genome, dnaIdx);
}


// update creature genome
// return update creature count
uint cGenomeManager::SetGenomeSelectCreature(set<evc::cCreature*> &creatures
	, const evc::cGenome &genome
	, const uint dnaIdx //= 0
)
{
	if (genome.m_dnas.size() <= dnaIdx)
		return 0; // error occrred!!

	uint cnt = 0;
	for (auto &creature : creatures)
	{
		if (creature->m_nodes.empty())
			continue;
		ai::cNeuralNet *nn = creature->m_nodes[0]->m_nn;
		if (!nn)
			continue;

		const evc::cGenome::sDna &dna = genome.m_dnas[dnaIdx];
		if (dna.inputCnt != nn->m_numInputs)
			continue; // error occurred!!
		if (dna.outputCnt != nn->m_numOutputs)
			continue; // error occurred!!
		if (dna.layerCnt < 2)
			continue; // error occurred!!

		ai::cNeuralNet *newNN = new ai::cNeuralNet(dna.inputCnt
			, dna.outputCnt, dna.layerCnt - 1, dna.outputCnt);
		newNN->PutWeights(dna.chromo);

		delete nn; // update neuralnetwork
		creature->m_nodes[0]->m_nn = newNN;
		++cnt;
	}

	return cnt;
}
