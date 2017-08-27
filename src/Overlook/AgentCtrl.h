#ifndef _Overlook_AgentCtrl_h_
#define _Overlook_AgentCtrl_h_

namespace Overlook {

class SnapshotDraw : public Ctrl {
	int snap_id;
	
public:
	SnapshotDraw();
	
	virtual void Paint(Draw& w);
	void SetSnap(int i) {snap_id = i;}
};

class EquityGraph : public Ctrl {
	
protected:
	TraineeBase* trainee;
	Color clr;
	Vector<Point> polyline;
	Vector<double> last;
	
public:
	typedef EquityGraph CLASSNAME;
	EquityGraph();
	
	virtual void Paint(Draw& w);
	
	void SetTrainee(TraineeBase& trainee) {this->trainee = &trainee; clr = RainbowColor(Randomf());}
	
};

class ResultGraph : public Ctrl {
	TraineeBase* trainee;
	Vector<Point> polyline;
	
public:
	typedef ResultGraph CLASSNAME;
	ResultGraph();
	
	virtual void Paint(Draw& w);
	
	void SetTrainee(TraineeBase& trainee) {this->trainee = &trainee;}
	
};

class HeatmapTimeView : public Ctrl {
	Callback1<Draw*> fn;
	
	Array<Image> lines;
	Vector<double> tmp;
	int mode;
	
public:
	typedef HeatmapTimeView CLASSNAME;
	HeatmapTimeView();
	
	virtual void Paint(Draw& d);
	
	template <class T>
	void PaintTemplate(Draw* dptr, void* agentptr) {
		Draw& d = *dptr;
		T& agent = *(T*)agentptr;
		
		Size sz = GetSize();
		
		int total_output = 0;
		total_output += agent.mul1.output.GetLength();
		total_output += agent.add1.output.GetLength();
		total_output += agent.tanh.output.GetLength();
		total_output += agent.mul2.output.GetLength();
		total_output += agent.add2.output.GetLength();
		tmp.SetCount(total_output);
		
		int cur = 0;
		for(int i = 0; i < agent.mul1.output.GetLength(); i++) tmp[cur++] = agent.mul1.output.Get(i);
		for(int i = 0; i < agent.add1.output.GetLength(); i++) tmp[cur++] = agent.add1.output.Get(i);
		for(int i = 0; i < agent.tanh.output.GetLength(); i++) tmp[cur++] = agent.tanh.output.Get(i);
		for(int i = 0; i < agent.mul2.output.GetLength(); i++) tmp[cur++] = agent.mul2.output.Get(i);
		for(int i = 0; i < agent.add2.output.GetLength(); i++) tmp[cur++] = agent.add2.output.Get(i);
		
		
		ImageBuffer ib(total_output, 1);
		RGBA* it = ib.Begin();
		double max = 0.0;
		
		for(int j = 0; j < tmp.GetCount(); j++) {
			double d = tmp[j];
			double fd = fabs(d);
			if (fd > max) max = fd;
		}
		
		for(int j = 0; j < tmp.GetCount(); j++) {
			double d = tmp[j];
			byte b = fabs(d) / max * 255;
			if (d >= 0)	{
				it->r = 0;
				it->g = 0;
				it->b = b;
				it->a = 255;
			}
			else {
				it->r = b;
				it->g = 0;
				it->b = 0;
				it->a = 255;
			}
			it++;
		}
		
		lines.Add(ib);
		while (lines.GetCount() > sz.cy) lines.Remove(0);
		
		
		ImageDraw id(sz);
		id.DrawRect(sz, Black());
		for(int i = 0; i < lines.GetCount(); i++) {
			Image& ib = lines[i];
			id.DrawImage(0, i, sz.cx, 1, ib);
		}
		
		
		d.DrawImage(0, 0, id);
	}
	
	template <class T>
	void SetAgent(T& t) {this->fn = THISBACK1(PaintTemplate<T>, &t); }
	
};

class TrainingCtrl : public ParentCtrl {
	
protected:
	friend class SnapshotDraw;
	TraineeBase* trainee;
	Splitter hsplit;
	
	Label epoch;
	SnapshotDraw draw;
	ResultGraph reward;
	EquityGraph stats;
	HeatmapTimeView timescroll;
	
	ArrayCtrl list;
	
	Splitter bsplit, vsplit;
	
public:
	typedef TrainingCtrl CLASSNAME;
	TrainingCtrl();
	
	void Data();
	void ApplySettings();
	void SetTrainee(TraineeBase& trainee);
	
};

class SnapshotCtrl : public ParentCtrl {
	Splitter hsplit;
	ArrayCtrl list;
	SnapshotDraw draw;
	
public:
	typedef SnapshotCtrl CLASSNAME;
	SnapshotCtrl();
	
	void Data();
};

class DataCtrl;

class DataGraph : public Ctrl {
	
protected:
	DataCtrl* dc;
	Color clr;
	Vector<Point> polyline;
	Vector<double> last;
	
public:
	typedef DataGraph CLASSNAME;
	DataGraph(DataCtrl* dc);
	
	virtual void Paint(Draw& w);
	
};

class DataCtrl : public ParentCtrl {
	DataGraph graph;
	Label data;
	SliderCtrl timeslider;
	Splitter hsplit;
	ArrayCtrl siglist, trade;
	Vector<int> poslist;
	int last_pos;
	
public:
	typedef DataCtrl CLASSNAME;
	DataCtrl();
	
	void Data();
	void GuiData();
	
	
	Vector<double> equity;
};

}

#endif
