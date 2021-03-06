#ifndef _Overlook_Chart_h_
#define _Overlook_Chart_h_

namespace Overlook {

class ChartManager;

class Chart : public SubWindowCtrl {
	
protected:
	friend class Overlook;
	friend class GraphCtrl;
	
	Array<ChartObject> objects;
	Vector<Ptr<CoreItem> > work_queue;
	Array<GraphCtrl> graphs;
	Splitter split;
	FactoryDeclaration decl;
	String title;
	int div = 0;
	int shift = 0;
	int symbol = 0, tf = 0;
	bool right_offset = 0, keep_at_end = 0;
	bool is_refreshing = false;
	Ptr<CoreItem> core;
	DataBridge* bardata = NULL;
	Mutex refresh_lock;
	
	
	void AddSeparateCore(int id, const Vector<double>& settings);
	void AddValueCore(int id, const Vector<double>& settings);
	void SetTimeValueTool(bool enable);
	void GraphMouseMove(Point pt, GraphCtrl* g);
	void Refresh0() {ParentCtrl::Refresh();}
	
public:
	typedef Chart CLASSNAME;
	Chart();
	~Chart();
	
	void PostRefresh() {PostCallback(THISBACK(Refresh0));}
	void ClearCores();
	GraphCtrl& AddGraph(Ptr<CoreIO> src);
	void SetGraph();
	void SetShift(int i) {shift = i;}
	void SetRightOffset(bool enable=true);
	void SetKeepAtEnd(bool enable=true);
	void Settings();
	void RefreshCore();
	void StartRefreshCore();
	void StartThread();
	void RefreshCoreData(bool store_cache);
	void OpenContextMenu() {MenuBar::Execute(THISBACK(ContextMenu));}
	void ContextMenu(Bar& bar);
	
	void Init(int symbol, const FactoryDeclaration& decl, int tf);
	virtual void Start();
	virtual void Data();
	virtual String GetTitle();
	
	ChartObject& AddObject() {return objects.Add();}
	ChartObject& GetObject(int i) {return objects[i];}
	int GetObjectCount() const {return objects.GetCount();}
	void DeleteObject(int i) {objects.Remove(i);}
	
	Core& GetCore() {return *bardata;}
	Color GetBackground() const {return White();}
	Color GetGridColor() const {return GrayColor();}
	int GetSymbol() const {return symbol;}
	int GetTf() const {return tf;}
	int GetWidthDivider() const {return div;}
	int GetShift() const {return shift;}
	bool GetRightOffset() const {return right_offset;}
	bool GetKeepAtEnd() const {return keep_at_end;}
	int GetGraphCount() const {return graphs.GetCount();}
	const GraphCtrl& GetGraph(int i) const {return graphs[i];}
	GraphCtrl& GetGraph(int i) {return graphs[i];}
	
	Chart& SetTimeframe(int tf_id);
	Chart& SetFactory(int f);
	
};



}


#endif
