//
// 2020-02-21, jjuiddong
// sensor interface
//
#pragma once


namespace evc
{

	interface iSensor
	{
		virtual const vector<double>& GetOutput() = 0;
		virtual uint GetOutputCount() = 0;
	};

}
