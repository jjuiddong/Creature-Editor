//
// 2020-02-25, jjuiddong
// evolution view
//
#pragma once


class cEvolutionView : public framework::cDockWindow
{
public:
	cEvolutionView(const StrId &name);
	virtual ~cEvolutionView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnEventProc(const sf::Event &evt) override;


public:
	int m_spawnSize;
	int m_epochSize;
	float m_epochTime;
	int m_generation;
	int m_grabBestFit;
};
