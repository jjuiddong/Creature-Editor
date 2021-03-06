//
// 2020-02-21, jjuiddong
// sensor interface
//
#pragma once


namespace evc
{

	DECLARE_ENUM(eSensorType, Angular, Limit, Velocity, Accel, Photo, Sound, Smell);

	interface iSensor
	{
		virtual const vector<double>& GetOutput() = 0;
		virtual uint GetOutputCount() = 0;
		virtual phys::cJoint* GetJoint() = 0;

		eSensorType::Enum m_type;
	};

}
