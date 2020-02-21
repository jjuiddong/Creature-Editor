//
// 2020-02-21, jjuiddong
// angular sensor
//
#pragma once


namespace evc
{

	class cAngularSensor : public iSensor
	{
	public:
		cAngularSensor();
		virtual ~cAngularSensor();

		bool Create(phys::cJoint *joint);

		// iSensor iterface override
		virtual const vector<double>& GetOutput() override;
		virtual uint GetOutputCount() override;


	public:
		phys::cJoint *m_joint; // reference
		vector<double> m_output;
	};

}
