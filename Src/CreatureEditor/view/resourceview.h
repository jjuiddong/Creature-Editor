//
// 2020-01-24, jjuiddong
// Resource View
//
#pragma once


class cResourceView : public framework::cDockWindow
{
public:
	cResourceView(const StrId &name);
	virtual ~cResourceView();

	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


public:
};
