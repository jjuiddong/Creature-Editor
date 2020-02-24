//
// 2020-02-21, jjuiddong
// effector
//
#pragma once


namespace evc
{

	DECLARE_ENUM(eEffectorType, Muscle, Sound, Scent);


	interface iEffector
	{
		virtual void Signal(const float deltaSeconds, const double signal) = 0;

		eEffectorType::Enum m_type;
	};

}
