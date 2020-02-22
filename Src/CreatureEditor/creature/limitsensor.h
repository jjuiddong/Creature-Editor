//
// 2020-02-22, jjuiddong
// limit sensor
//	- output
//		- joint contact limit (lower:-1, upper:1, no:0)
//
#pragma once


namespace evc
{

	class cLimitSensor : public iSensor
	{
	public:
		cLimitSensor() : m_joint(nullptr), m_output(1, 0.f) {
		}
		virtual ~cLimitSensor() {}

		bool Create(phys::cJoint *joint) {
			m_joint = joint;
			return true;
		}

		// iSensor iterface override
		virtual const vector<double>& GetOutput() override {
			RETV(!m_joint, m_output);
			switch (m_joint->GetLimitContact())
			{
			case 0: m_output[0] = 0.f; break; // none
			case 1: m_output[0] = -1.f; break; // lower
			case 2: m_output[0] = 1.f; break; // upper
			}
			return m_output;
		}

		virtual uint GetOutputCount() override { return m_output.size(); }


	public:
		phys::cJoint *m_joint; // reference
		vector<double> m_output;
	};

}
