//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- evc global variable, functions
//
#pragma once


namespace evc
{

	// evc global variable class
	class cEvc
	{
	public:
		cEvc(phys::cPhysicsEngine *phys, phys::cPhysicsSync *sync);

		phys::cPhysicsEngine *m_phys; // reference
		phys::cPhysicsSync *m_sync; // reference
	};


	cEvc* InitializeEvc(phys::cPhysicsEngine *phys
		, phys::cPhysicsSync *sync);

	bool ReleaseEvc();

	// global variable for evc working
	extern cEvc *g_evc;

}

