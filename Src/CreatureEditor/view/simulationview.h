//
// 2020-01-31, jjuiddong
// Simulation View
//
#pragma once


class cSimulationView : public framework::cDockWindow
{
public:
	cSimulationView(const StrId &name);
	virtual ~cSimulationView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:

};
