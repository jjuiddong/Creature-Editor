//
// 2020-03-4, jjuiddong
// genome manager
//	- genome read/write
//	- set creature genome
//
#pragma once


class cGenomeManager
{
public:
	cGenomeManager();
	virtual ~cGenomeManager();

	bool Init();
	uint SetGenomeSelectCreature(const StrPath &fileName);
	uint SetGenomeSelectCreature(const evc::cGenome &genome, const uint dnaIdx = 0);
	uint SetGenomeSelectCreature(set<evc::cCreature*> &creatures
		, const evc::cGenome &genome, const uint dnaIdx = 0);


public:
};
