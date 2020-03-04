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


public:
};
