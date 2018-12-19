#include "Overlook.h"


namespace Overlook {

void MovingAverageEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<MovingAverage>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void MovingAverageEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}

void MACDEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<MovingAverageConvergenceDivergence>()).AddArg(period).AddArg(period*2);
	cl_sym.Init();
	cl_sym.Refresh();
}

void MACDEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos <= 0)
		output = 0;
	else {
		int code = 0;
		for(int i = 0; i < 4; i++) {
			ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, i);
			bool ena = lbl.enabled.Get(tf_pos);
			bool cur = lbl.signal.Get(tf_pos);
			if (ena) code |= 1 << (i * 2 + 0);
			if (cur) code |= 1 << (i * 2 + 1);
		}
		output = code;
	}
}

void BBEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<BollingerBands>()).AddArg(period).AddArg(0).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void BBEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}


void PSAREvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<ParabolicSAR>());
	cl_sym.Init();
	cl_sym.Refresh();
}

void PSAREvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}

void CCIEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<CommodityChannelIndex>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void CCIEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}

void DeMarkerEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<DeMarker>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void DeMarkerEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}


void MomentumEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<Momentum>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void MomentumEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}




void RSIEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<RelativeStrengthIndex>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void RSIEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}





void RVIEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<RelativeVigorIndex>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void RVIEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}



void StochasticEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<StochasticOscillator>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void StochasticEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}



void AcceleratorEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<AcceleratorOscillator>());
	cl_sym.Init();
	cl_sym.Refresh();
}

void AcceleratorEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}



void ChannelEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<ChannelOscillator>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void ChannelEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}



void ScissorsChannelEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<ScissorChannelOscillator>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void ScissorsChannelEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}



void OnlineMinimalLabelEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<OnlineMinimalLabel>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void OnlineMinimalLabelEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}




void LaguerreEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<Laguerre>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void LaguerreEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}




void QQEEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<QuantitativeQualitativeEstimation>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void QQEEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}





void TickBalanceEvent::Init() {
	System& sys = GetSystem();
	cl_sym.AddSymbol(sys.GetSymbol(GetSymbol()));
	cl_sym.AddTf(tf);
	cl_sym.AddIndi(sys.Find<TickBalanceOscillator>()).AddArg(period);
	cl_sym.Init();
	cl_sym.Refresh();
}

void TickBalanceEvent::Start(int pos, int& output) {
	DataBridgeCommon& dbc = GetDataBridgeCommon();
	const Index<Time>& fast_idx = dbc.GetTimeIndex(fast_tf);
	const Index<Time>& idx = dbc.GetTimeIndex(tf);
	Time t = fast_idx[pos];
	int tf_pos = idx.Find(t);
	if (tf_pos == -1)
		output = 0;
	else {
		ConstLabelSignal& lbl = cl_sym.GetLabelSignal(0, 0, 0);
		output = 1 + lbl.signal.Get(tf_pos);
	}
}


}
