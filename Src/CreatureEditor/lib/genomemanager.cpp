
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
	if (genome.m_genomes.empty())
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

	uint cnt = 0;
	for (auto &creature : creatures)
	{
		if (creature->m_nodes.empty())
			continue;
		ai::cNeuralNet *nn = creature->m_nodes[0]->m_nn;
		if (!nn)
			continue;

		const evc::cGenome::sGenome &gen = genome.m_genomes[0];
		if (gen.inputCnt != nn->m_numInputs)
			continue; // error occurred!!
		if (gen.outputCnt != nn->m_numOutputs)
			continue; // error occurred!!
		if (gen.layerCnt < 2)
			continue; // error occurred!!

		ai::cNeuralNet *newNN = new ai::cNeuralNet(gen.inputCnt
			, gen.outputCnt, gen.layerCnt - 1, gen.outputCnt);
		newNN->PutWeights(gen.chromo);

		delete nn; // update neuralnetwork
		creature->m_nodes[0]->m_nn = newNN;
		++cnt;
	}

	return cnt;
}
