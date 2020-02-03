
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_3dView(nullptr)
	, m_peditorView(nullptr)
	, m_physSync(nullptr)
	, m_pheno(nullptr)
{
}

cGlobal::~cGlobal()
{
	Clear();
}


bool cGlobal::Init(graphic::cRenderer &renderer)
{
	if (!m_physics.InitializePhysx())
		return false;

	m_physSync = new phys::cPhysicsSync();
	m_physSync->Create(&m_physics);

	evc::InitializeEvc(&m_physics, m_physSync);

	return true;
}


graphic::cRenderer& cGlobal::GetRenderer()
{
	return m_3dView->GetRenderer();
}


void cGlobal::Clear()
{
	evc::ReleaseEvc();
	m_physics.Clear();
}
