
#include "stdafx.h"
#include "pnode.h"

using namespace evc;


cPNode::cPNode()
	: m_id(common::GenerateId())
	, m_actor(nullptr)
	, m_inputSize(0)
{
}

cPNode::~cPNode()
{
	Clear();
}


// create from genotype node struct
bool cPNode::Create(graphic::cRenderer &renderer, const sGenotypeNode &gnode)
{
	m_gid = gnode.id;
	m_name = gnode.name;
	m_gnode = &gnode;
	return true;
}


// initialize neural net
bool cPNode::InitializeNN(const uint numHidden, const uint neuronsPerHiddenLayer
	, const vector<double> &weights)
{
	if (m_nn)
		return false; // already initialize

	if (m_sensors.empty() || m_effectors.empty())
		return false;

	uint numInputs = 0;
	for (auto &p : m_sensors)
		numInputs += p->GetOutputCount();
	if (numInputs == 0)
		return false;

	m_nn = new ai::cNeuralNet(numInputs, m_effectors.size()
		, numHidden, neuronsPerHiddenLayer);
	if (!weights.empty())
		m_nn->PutWeights(weights);

	m_inputSize = numInputs;

	return true;
}


// update NN
bool cPNode::Update(const float deltaSeconds)
{
	RETV(!m_nn, false);

	vector<double> input;
	input.reserve(m_inputSize);
	for (auto &p : m_sensors)
	{
		const vector<double> &output = p->GetOutput();
		for (auto &val : output)
			input.push_back(val);
	}

	vector<double> output = m_nn->Update(input);
	if (output.size() != m_effectors.size())
		return false; // error occurred!!

	for (uint i = 0; i < m_effectors.size(); ++i)
		m_effectors[i]->Signal(deltaSeconds, output[i]);

	return true;
}


bool cPNode::AddSensor(iSensor *sensor)
{
	if (m_sensors.end() != std::find(m_sensors.begin(), m_sensors.end(), sensor))
		return false; // already exist
	m_sensors.push_back(sensor);
	return true;
}


bool cPNode::RemoveSensor(iSensor *sensor)
{
	if (m_sensors.end() == std::find(m_sensors.begin(), m_sensors.end(), sensor))
		return false; // not found
	common::removevector(m_sensors, sensor);
	return true;
}


bool cPNode::AddEffector(iEffector *effector)
{
	if (m_effectors.end() != std::find(m_effectors.begin(), m_effectors.end(), effector))
		return false; // already exist
	m_effectors.push_back(effector);
	return true;
}


bool cPNode::RemoveEffector(iEffector *effector)
{
	if (m_effectors.end() == std::find(m_effectors.begin(), m_effectors.end(), effector))
		return false; // not found
	common::removevector(m_effectors, effector);
	return true;
}


void cPNode::Clear()
{
	if (m_actor)
	{
		vector<phys::cJoint*> joints = m_actor->m_joints;
		for (auto &joint : joints)
			g_evc->m_sync->RemoveSyncInfo(joint);
		g_evc->m_sync->RemoveSyncInfo(m_actor);
	}

	for (auto &p : m_children)
		delete p;
	m_children.clear();
	m_actor = nullptr;
	m_node = nullptr;

	SAFE_DELETE(m_nn);
	for (auto &p : m_sensors)
		delete p;
	m_sensors.clear();
	for (auto &p : m_effectors)
		delete p;
	m_effectors.clear();
}
