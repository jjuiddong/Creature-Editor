//
// 2020-01-25, jjuiddong
// evc : evolved virtual creature
//		- pnode : phenotype node (karlsims94)
//		- tree architecture
//
#pragma once


namespace evc
{
	class cNeuralNet;
	interface iSensor;
	interface iEffector;

	class cPNode
	{
	public:
		cPNode();
		virtual ~cPNode();

		bool Create(graphic::cRenderer &renderer, const sGenotypeNode &gnode);
		bool InitializeNN(const uint numHidden, const uint neuronsPerHiddenLayer
			, const vector<double> &weights);
		bool Update(const float deltaSeconds);

		bool AddSensor(iSensor *sensor);
		bool RemoveSensor(iSensor *sensor);
		bool AddEffector(iEffector *effector);
		bool RemoveEffector(iEffector *effector);
		void Clear();


	public:
		int m_id;
		int m_gid; // genotype node id
		StrId m_name;
		phys::cRigidActor *m_actor; // reference
		graphic::cNode *m_node; // reference
		vector<cPNode*> m_children;

		cNeuralNet *m_nn;
		vector<iSensor*> m_sensors;
		vector<iEffector*> m_effectors;
		uint m_inputSize;
	};

}
