
#include "stdafx.h"
#include "muscleeffector.h"

using namespace evc;


cMuscleEffector::cMuscleEffector()
	: m_joint(nullptr)
	, m_incT(0.f)
{
}

cMuscleEffector::~cMuscleEffector()
{
}


bool cMuscleEffector::Create(phys::cJoint *joint)
{
	m_joint = joint;
	return true;
}


// iEffector interface override
void cMuscleEffector::Signal(const float deltaSeconds, const double signal)
{
	RET(!m_joint);
	m_incT += deltaSeconds;

	if (m_incT > 0.3f)
	{
		m_incT = 0.f;

		//m_joint->SetDriveVelocity( common::clamp2((float)signal*50.f, -3.f, 3.f));
		m_joint->SetDriveVelocity((signal < 0.f)? -3.f : 3.f);
		m_joint->m_actor0->WakeUp();
		m_joint->m_actor1->WakeUp();
	}
}
