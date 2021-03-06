//
// 2020-02-22, jjuiddong
// velocity sensor
//	- output
//		- joint drive velocity
//
#pragma once


namespace evc
{

	class cVelocitySensor : public iSensor
	{
	public:
		cVelocitySensor() : m_joint(nullptr), m_output(1, 0.f) {
		}
		virtual ~cVelocitySensor() {}

		bool Create(phys::cJoint *joint) {
			m_joint = joint;
			m_type = eSensorType::Velocity;
			return true;
		}

		// iSensor iterface override
		virtual const vector<double>& GetOutput() override {
			RETV(!m_joint, m_output);
			m_output[0] = m_joint->GetDriveVelocity();
			return m_output;
		}

		virtual uint GetOutputCount() override { return m_output.size(); }
		virtual phys::cJoint* GetJoint() override { return m_joint; }


	public:
		phys::cJoint *m_joint; // reference
		vector<double> m_output;
	};

}
