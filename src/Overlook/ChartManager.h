#ifndef _Overlook_ChartManager_h_
#define _Overlook_ChartManager_h_

#include <CtrlLib/CtrlLib.h>
using namespace Upp;



namespace Overlook {

// Main view. Shows subwindows of graphs and ctrls.
class ChartManager : public SubWindows {
	friend class Chart;
	
public:

	// Ctors
	typedef ChartManager CLASSNAME;
	ChartManager();
	
	
	// Main funcs
	void Init();
	Chart& AddChart();
	SpeculationCtrl& AddSpeculation();
	void CloseBacktestWindows(bool exclude_indicators=true);
	void RefreshWindows();

	// Get funcs
	Chart* GetVisibleChart() {return dynamic_cast<Chart*>(GetVisibleSubWindowCtrl());}
	Chart* GetGroup(int i) {return dynamic_cast<Chart*>(SubWindows::Get(i).GetSubWindowCtrl());}
	SpeculationCtrl* GetSpeculation(int i) {return dynamic_cast<SpeculationCtrl*>(SubWindows::Get(i).GetSubWindowCtrl());}
	int GetGroupCount() const {return SubWindows::GetCount();}
	
	
	
};

}

#endif
