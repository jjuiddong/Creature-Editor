//
// 2020-02-21, jjuiddong
// muscle effector
//
#pragma once


namespace evc
{

	class cMuscleEffector : public iEffector
	{
	public:
		cMuscleEffector();
		virtual ~cMuscleEffector();

		bool Create(phys::cJoint *joint);

		// iEffector interface override
		virtual void Signal(const float deltaSeconds, const double signal) override;


	public:
		float m_incT;
		phys::cJoint *m_joint; // reference
	};

}
