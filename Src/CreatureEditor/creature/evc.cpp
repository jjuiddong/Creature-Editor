
#include "stdafx.h"
#include "evc.h"

using namespace evc;

namespace evc
{
	cEvc *g_evc = nullptr;
}


cEvc::cEvc(phys::cPhysicsEngine *phys, phys::cPhysicsSync *sync) 
	: m_phys(phys)
	, m_sync(sync)
{
}


// initialize evc global variable
cEvc* evc::InitializeEvc(phys::cPhysicsEngine *phys
	, phys::cPhysicsSync *sync)
{
	if (g_evc)
		return g_evc; // already exist

	g_evc = new cEvc(phys, sync);
	return g_evc;
}


// remove evc global variable
bool evc::ReleaseEvc()
{
	SAFE_DELETE(g_evc);
	return true;
}
