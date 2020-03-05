//
// 2020-02-26, jjuiddong
// evolution simulation manager
//
#pragma once


// evolution system parameter
struct sEvolutionParam
{
	StrPath creatureFileName;
	uint spawnSize;
	uint epochSize;
	float epochTime;
	uint generation;
	uint grabBestFit;
};


class cEvolutionManager
{
public:
	cEvolutionManager();
	virtual ~cEvolutionManager();

	bool Init();
	bool RunEvolution(const sEvolutionParam &param);
	bool StopEvolution();
	bool Update(const float deltaSeconds);
	

protected:
	void SpawnCreatures(const vector<ai::sGenome> &genomes = vector<ai::sGenome>());
	bool SaveCreatureGenomes();


public:
	enum class eState {Wait, Run};
	
	struct sCreatureData
	{
		Vector3 spawnPos;
	};

	eState m_state;
	sEvolutionParam m_param;
	uint m_curEpochSize;
	float m_incT;
	ai::cGeneticAlgorithm m_genetic;
	map<evc::cCreature*, sCreatureData> m_creatures; // reference

	// evolution monitoring graph
	int m_graphIdx;
	float m_avrGraph[64]; // fitness average
	float m_fitnessGraph[64]; // best fitness
	float m_grabBestAvrGraph[64]; // grab best fitness average
};
