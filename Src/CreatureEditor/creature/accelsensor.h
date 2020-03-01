//
// 2020-03-01, jjuiddong
// accelerometer sensor
//	- output
//		- joint accel
//
#pragma once


namespace evc
{

	class cAccelSensor : public iSensor
	{
	public:
		cAccelSensor() : m_joint(nullptr), m_output(1, 0.f)
			, m_oldVelocity(0){
		}
		virtual ~cAccelSensor() {}

		bool Create(phys::cJoint *joint) {
			m_joint = joint;
			m_type = eSensorType::Angular;
			return true;
		}

		// iSensor iterface override
		virtual const vector<double>& GetOutput() override {
			RETV(!m_joint, m_output);
			// todo: divide delta time
			const float velocity = m_joint->GetDriveVelocity();
			m_output[0] = velocity - m_oldVelocity;
			m_oldVelocity = velocity;
			return m_output;
		}

		virtual uint GetOutputCount() override { return m_output.size(); }
		virtual phys::cJoint* GetJoint() override { return m_joint; }


	public:
		phys::cJoint *m_joint; // reference
		float m_oldVelocity;
		vector<double> m_output;
	};

}
