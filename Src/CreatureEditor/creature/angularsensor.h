//
// 2020-02-21, jjuiddong
// angular sensor
//	- output
//		- joint angle
//
#pragma once


namespace evc
{

	class cAngularSensor : public iSensor
	{
	public:
		cAngularSensor() : m_joint(nullptr), m_output(1, 0.f) {
		}
		virtual ~cAngularSensor() {}

		bool Create(phys::cJoint *joint) {
			m_joint = joint;
			m_type = eSensorType::Angular;
			return true;
		}

		// iSensor iterface override
		virtual const vector<double>& GetOutput() override {
			RETV(!m_joint, m_output);
			m_output[0] = m_joint->m_curAngle * 4.f;
			return m_output;
		}

		virtual uint GetOutputCount() override {return m_output.size();}
		virtual phys::cJoint* GetJoint() override {return m_joint;}


	public:
		phys::cJoint *m_joint; // reference
		vector<double> m_output;
	};

}
