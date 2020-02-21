//
// 2020-02-21, jjuiddong
// effector
//
#pragma once


namespace evc
{

	interface iEffector
	{
		virtual void Signal(const float deltaSeconds, const double signal) = 0;
	};

}
