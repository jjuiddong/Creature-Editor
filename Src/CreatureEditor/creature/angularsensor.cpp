
#include "stdafx.h"
#include "angularsensor.h"

using namespace evc;


cAngularSensor::cAngularSensor()
	: m_joint(nullptr)
	, m_output(1,0)
{
}

cAngularSensor::~cAngularSensor()
{
}


bool cAngularSensor::Create(phys::cJoint *joint)
{
	m_joint = joint;
	return true;
}


// iSensor iterface override
const vector<double>& cAngularSensor::GetOutput()
{
	RETV(!m_joint, m_output);

	m_output[0] = m_joint->GetRelativeAngle();
	return m_output;
}


uint cAngularSensor::GetOutputCount() {
	return 1;
}
