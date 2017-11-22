#include "Overlook.h"


namespace Overlook {
using namespace Upp;





const int label_count = LABELINDI_COUNT;
const int sector_pred_dir_count = 2;
const int sector_label_count = 4;
const int period_begin = 3;
const int fastinput_count = 3;
const int labelpattern_count = 5;
int sector_type_count;

int GetConfCount() {
	if (!sector_type_count) {
		for(int i = period_begin; i < TF_COUNT; i++)
			sector_type_count += (TF_COUNT - i);
	}
	return label_count * sector_pred_dir_count * sector_label_count * fastinput_count * labelpattern_count * sector_type_count;
}

void GetConf(int i, AccuracyConf& conf) {
	int count = GetConfCount();
	ASSERT(i >= 0 && i < count);
	
	conf.id = i;
	
	conf.label_id = i % label_count;
	ASSERT(conf.label_id >= 0 && conf.label_id < label_count);
	i = i / label_count;
	
	int sector_type = i % sector_type_count;
	i = i / sector_type_count;
	int j = 0;
	for(conf.period = period_begin; conf.period < TF_COUNT; conf.period++)
		for(conf.ext = 1; conf.ext <= (TF_COUNT - conf.period); conf.ext++)
			if (j++ == sector_type)
				goto found;
	found:
	ASSERT(conf.period >= period_begin && conf.period < TF_COUNT);
	ASSERT(conf.ext > 0 && conf.ext <= TF_COUNT);
	
	conf.label = i % sector_label_count;
	i = i / sector_label_count;
	
	conf.fastinput = i % fastinput_count;
	i = i / fastinput_count;
	
	conf.labelpattern = i % labelpattern_count;
	i = i / labelpattern_count;
	
	conf.ext_dir = i % sector_pred_dir_count;
	i = i / sector_pred_dir_count;
}













void ForestArea::FillArea(int level, int data_count) {
	int len;
	switch (level) {
		case 0: len = 1 * 5 * 24 * 60 / MAIN_PERIOD_MINUTES; break;
		case 1: len = 2 * 5 * 24 * 60 / MAIN_PERIOD_MINUTES; break;
		case 2: len = 4 * 5 * 24 * 60 / MAIN_PERIOD_MINUTES; break;
		default: len = 8 * 5 * 24 * 60 / MAIN_PERIOD_MINUTES;
	}
	train_begin = Random(data_count - len);
	train_end = train_begin + len;
	do {
		test0_begin = Random(data_count - len);
		test0_end = test0_begin + len;
	} while (abs(test0_begin - train_begin) < 2*len);
	do {
		test1_begin = Random(data_count - len);
		test1_end = test1_begin + len;
	}
	while (abs(test0_begin - train_begin) < 2*len ||
		   abs(test1_begin - train_begin) < 2*len);
}

void ForestArea::FillArea(int data_count) {
	train_begin		= 24*60 / MAIN_PERIOD_MINUTES;
	train_end		= data_count * 2/3;
	test0_begin		= train_end;
	test0_end		= data_count - 1*5*24*60 / MAIN_PERIOD_MINUTES;
	test1_begin		= test0_end;
	test1_end		= data_count;
}
















DqnAdvisor::DqnAdvisor() : dqn_trainer(&data) {
	rf_trainer.options.tree_count		= 100;
	rf_trainer.options.max_depth		= 4;
	rf_trainer.options.tries_count		= 10;
}

void DqnAdvisor::Init() {
	SetCoreSeparateWindow();
	SetCoreMinimum(-1.0);  // normalized
	SetCoreMaximum(+1.0);   // normalized
	
	SetBufferColor(0, Color(28, 42, 255));
	SetBufferColor(1, Color(255, 42, 0));
	SetBufferLineWidth(0, 2);
	SetBufferLineWidth(1, 2);
	
	#if DEBUG_BUFFERS
	for(int i = 0; i < LOCALPROB_DEPTH; i++) {
		RGBA gray_clr = GrayColor(100 + i * 100 / LOCALPROB_DEPTH);
		RGBA red_tint(gray_clr), green_tint(gray_clr);
		red_tint.r		+= 30;
		green_tint.g	+= 30;
		SetBufferColor(main_graphs + i * 2 + 0, green_tint);
		SetBufferColor(main_graphs + i * 2 + 1, red_tint);
	}
	#endif
	
	conf_count = GetConfCount();
	DataBridge* db = dynamic_cast<DataBridge*>(GetInputCore(0, GetSymbol(), GetTf()));
	open_buf = &GetInputBuffer(0, 0);
	spread_point = db->GetPoint();
	ASSERT(spread_point > 0.0);
	
	SetJobCount(3);
	
	SetJob(0, "Source Search")
		.SetBegin		(THISBACK(SourceSearchBegin))
		.SetIterator	(THISBACK(SourceSearchIterator))
		.SetInspect		(THISBACK(SourceSearchInspect))
		.SetCtrl		<SourceSearchCtrl>();
	
	
	SetJob(1, "Random Forest Training")
		.SetBegin		(THISBACK(TrainingRFBegin))
		.SetIterator	(THISBACK(TrainingRFIterator))
		.SetEnd			(THISBACK(TrainingRFEnd))
		.SetInspect		(THISBACK(TrainingRFInspect))
		.SetCtrl		<TrainingRFCtrl>();
	
	
	SetJob(2, "DQN Training")
		.SetBegin		(THISBACK(TrainingDQNBegin))
		.SetIterator	(THISBACK(TrainingDQNIterator))
		.SetEnd			(THISBACK(TrainingDQNEnd))
		.SetInspect		(THISBACK(TrainingDQNInspect))
		.SetCtrl		<TrainingDQNCtrl>();
	
}

void DqnAdvisor::Start() {
	LOG("DqnAdvisor::Start");
	
	/*if (once) {
		//prev_counted = 0;
		Job& job = GetJob(2);
		job.actual = 0;
		job.state = 0;
		once = false;
	}*/
	
	int bars = GetBars();
	Output& out = GetOutput(0);
	for(int i = 0; i < out.buffers.GetCount(); i++) {
		ASSERT(bars == out.buffers[i].GetCount());
	}
	
	if (IsJobsFinished()) {
		int bars = GetBars();
		if (prev_counted < bars)
			RefreshAll();
	}
}

void DqnAdvisor::RefreshAll() {
	TimeStop ts;
	
	RefreshOutputBuffers();
	LOG("DqnAdvisor::Start ... RefreshOutputBuffers " << ts.ToString());
	ts.Reset();
	
	RefreshMain();
	LOG("DqnAdvisor::Start ... RefreshMain " << ts.ToString());
	
	prev_counted = GetBars();
}

bool DqnAdvisor::SourceSearchBegin() {
	SetTrainingArea();
	
	
	// Prepare training functions and output variables
	rflist_pos.SetCount(LOCALPROB_DEPTH);
	rflist_neg.SetCount(LOCALPROB_DEPTH);
	
	
	System& sys = GetSystem();
	int symbol = GetSymbol();
	int tf = GetTf();
	data_count = sys.GetCountTf(tf);
	
	training_rf.Create();
	
	full_mask.SetCount(data_count).One();
	
	search_pts.SetCount(conf_count, 0.0);
	
	return true;
}

bool DqnAdvisor::SourceSearchIterator() {
	GetCurrentJob().SetProgress(opt_counter, conf_count);
	if (opt_counter >= conf_count) {
		SetJobFinished();
		return true;
	}
	
	
	// Use common trainer for performance reasons
	RF& rf = *training_rf;
	
	
	// Get configuration
	AccuracyConf& conf = rf.a;
	GetConf(opt_counter, conf);
	VectorBool& real_mask = rf.c;
	real_mask.SetCount(data_count).One();
	
	
	// Skip duplicate combinations
	// This is the easiest way... the harder way would be to not count duplicates.
	if (conf.ext == 1) {
		if (conf.label >= 2 || conf.labelpattern > 0 || conf.ext_dir) {
			// Iterate next recursively. The next non-duplicate shouldn't be far.
			opt_counter++;
			return SourceSearchIterator();
		}
	}
	
	
	// Refresh masks
	ConstBufferSource bufs;
	
	
	// Get active label
	uint64 active_label_pattern = 0;
	switch (conf.label) {
		case 0: active_label_pattern = 0; break;
		case 1: active_label_pattern = ~(0ULL); break;
		case 2: for (int i = 1; i < 64; i += 2) active_label_pattern |= 1 << i; break;
		case 3: for (int i = 0; i < 64; i += 2) active_label_pattern |= 1 << i; break;
	}
	bool active_label = active_label_pattern & 1;
	Array<RF>& rflist = active_label == false ? rflist_pos : rflist_neg;
	
	
	// Get label indicator. Do AND operation for all layers.
	int layer_count = conf.ext;
	ASSERT(layer_count > 0);
	for(int sid = 0; sid < layer_count; sid++) {
		int tf = conf.GetBaseTf(sid);
		bool sid_active_label = active_label_pattern & (1 << (layer_count - 1 - sid));
		
		// Get label id
		int label_id = 0;
		switch (conf.labelpattern) {
			case 0: label_id = conf.label_id; break;
			case 1: label_id = conf.label_id - sid; break;
			case 2: label_id = conf.label_id + sid; break;
			case 3: label_id = conf.label_id - sid % 2; break;
			case 4: label_id = conf.label_id + sid % 2; break;
		}
		while (label_id < 0)				label_id += LABELINDI_COUNT;
		while (label_id >= LABELINDI_COUNT)	label_id -= LABELINDI_COUNT;
		
		
		// Get filter label
		ConstVectorBool& src_label = GetInputLabel(1 + indi_count + label_id + tf * (indi_count + label_count));
		int count0 = real_mask.GetCount();
		int count1 = src_label.GetCount();
		if (sid_active_label)	real_mask.And(src_label);
		else					real_mask.InverseAnd(src_label);
	}
	
	
	// Test
	bool mask_prev_active		= false;
	int mask_prev_pos			= 0;
	double mask_open			= open_buf->GetUnsafe(area.train_begin);
	double mask_hour_total		= 0.0;
	double mask_change_total	= 1.0;
	double mult_change_total	= 1.0;
	for(int i = area.train_begin; i < area.train_end; i++) {
		bool is_mask =  real_mask.Get(i);
		
		if (is_mask) {
			if (!mask_prev_active) {
				mask_open			= open_buf->GetUnsafe(i);
				mask_prev_active	= true;
				mask_prev_pos		= i;
			}
		} else {
			if (mask_prev_active) {
				double change, cur = open_buf->GetUnsafe(i);
				int len = i - mask_prev_pos;
				double hours = (double)len / 60.0;
				mask_hour_total += hours;
				if (!active_label)
					change = cur / (mask_open + spread_point);
				else
					change = mask_open / (cur + spread_point);
				mask_change_total *= change;
				mask_prev_active = false;
				mask_prev_pos = i;
			}
		}
	}
	mask_change_total -= 1.0;
	conf.test_valuefactor = mask_change_total;
	conf.test_valuehourfactor = mask_hour_total > 0.0 ? mask_change_total / mask_hour_total : 0.0;
	conf.test_hourtotal = mask_hour_total;
	conf.is_processed = true;
	
	
	if (conf.test_valuehourfactor > 0.0) {
		LOG(GetSymbol() << ": " << conf.test_valuehourfactor);
	}
	
	// Sort list
	rflist.Add(training_rf.Detach());
	Sort(rflist, RFSorter());
	training_rf.Attach(rflist.Detach(rflist.GetCount()-1));
	
	
	search_pts[opt_counter] = conf.test_valuehourfactor;
	
	opt_counter++;
	return true;
}

bool DqnAdvisor::SourceSearchInspect() {
	
	INSPECT(rflist_pos.GetCount() == LOCALPROB_DEPTH, "error: Unexpected count of sources");
	INSPECT(rflist_neg.GetCount() == LOCALPROB_DEPTH, "error: Unexpected count of sources");
	
	int invalid_count = 0;
	for(int i = 0; i < rflist_pos.GetCount(); i++)
		if (rflist_pos[i].a.test_valuehourfactor <= 0.0)
			invalid_count++;
	for(int i = 0; i < rflist_neg.GetCount(); i++)
		if (rflist_neg[i].a.test_valuehourfactor <= 0.0)
			invalid_count++;
	if (invalid_count != 0) {
		INSPECT(0, "error: Didn't found sources properly");
		return false;
	}
	//INSPECT(0, "ok: source search seems to be successful");
	
	return true;
}

bool DqnAdvisor::TrainingRFBegin() {
	SetRealArea();
	
	System& sys = GetSystem();
	
	int tf = GetTf();
	int data_count = sys.GetCountTf(tf);
	
	
	full_mask.SetCount(data_count).One();
	
	training_pts.SetCount(2*LOCALPROB_DEPTH, 0.0);
	
	return true;
}

bool DqnAdvisor::TrainingRFIterator() {
	ASSERT(full_mask.GetCount() > 0 && rflist_iter >= 0);
	
	int a = 0, t = 1;
	if (p == 0)	{a = rflist_iter; t = rflist_pos.GetCount() + rflist_neg.GetCount();}
	else		{a = rflist_pos.GetCount() + rflist_iter; t = rflist_pos.GetCount() + rflist_neg.GetCount();}
	GetCurrentJob().SetProgress(a, t);
	
	
	ConstBufferSource bufs;
	
	Array<RF>& rflist = p == 0 ? rflist_pos : rflist_neg;
	
	RF& rf = rflist[rflist_iter];
	AccuracyConf& conf = rf.a;
	VectorBool& real_mask = rf.c;
	LOG(GetSymbol() << ": " << rflist_iter << ": " << p << ": test_valuehourfactor: " << conf.test_valuehourfactor);
	
	
	
	rf_trainer.forest.memory.Attach(&rf.b);
	
	FillBufferSource(conf, bufs);
	rf_trainer.Process(area, bufs, real_mask, full_mask);
	
	conf.stat = rf_trainer.stat;
	LOG(GetSymbol() << ": SOURCE " << rflist_iter << ": " << p << ": train_accuracy: " << conf.stat.train_accuracy << " test_accuracy: " << conf.stat.test0_accuracy);
	
	rf_trainer.forest.memory.Detach();
	
	
	training_pts[rflist_iter + p * LOCALPROB_DEPTH] = conf.stat.test0_accuracy;
	
	
	rflist_iter++;
	if (rflist_iter >= rflist.GetCount()) {
		p++;
		rflist_iter = 0;
		if (p >= 2)
			SetJobFinished();
	}
	return true;
}

bool DqnAdvisor::TrainingRFEnd() {
	ForceSetCounted(0);
	RefreshOutputBuffers();
	return true;
}

bool DqnAdvisor::TrainingRFInspect() {
	
	int invalid_count = 0;
	for(int i = 0; i < rflist_pos.GetCount(); i++)
		if (rflist_pos[i].a.stat.test0_accuracy <= 0.50)
			invalid_count++;
	for(int i = 0; i < rflist_neg.GetCount(); i++)
		if (rflist_neg[i].a.stat.test0_accuracy <= 0.50)
			invalid_count++;
	INSPECT(invalid_count == 0, "warning: bad accuracy in " + IntStr(invalid_count) + " of " + IntStr(LOCALPROB_DEPTH*2));
	
	return true;
}



bool DqnAdvisor::TrainingDQNBegin() {
	SetRealArea();
	
	int bars = GetBars();
	data.SetCount(bars);
	GetOutput(0).label.SetCount(bars);
	
	RefreshOutputBuffers();
	
	dqntraining_pts.SetCount(bars, 0);
	
	return true;
}

bool DqnAdvisor::TrainingDQNIterator() {
	GetCurrentJob().SetProgress(dqn_round, dqn_max_rounds);
	
	ASSERT(!data.IsEmpty());
	
	Buffer& sig0_dqnprob	= GetBuffer(0);
	Buffer& sig1_dqnprob	= GetBuffer(1);
	VectorBool& label		= GetOutput(0).label;
	
	double max_epsilon = 0.20;
	double min_epsilon = 0.01;
	double epsilon = (max_epsilon - min_epsilon) * (dqn_max_rounds - dqn_round) / dqn_max_rounds + min_epsilon;
	dqn_trainer.SetEpsilon(epsilon);
	
	for(int i = 0; i < 10; i++) {
		int pos = dqn_round % (data.GetCount() - 1);
		
		
		double curr		= open_buf->GetUnsafe(pos);
		double next		= open_buf->GetUnsafe(pos + 1);
		
		
		DQN::DQItem& before = data[pos];
		before.action = dqn_trainer.Act(before);
		if (!before.action)		before.reward = next / (curr + spread_point) - 1.0;
		else					before.reward = curr / (next + spread_point) - 1.0;
		before.reward *= 10000;
		
		double p0 = dqn_trainer.data.add2.output.Get(0) * 0.2;
		double p1 = dqn_trainer.data.add2.output.Get(1) * 0.2;
		sig0_dqnprob.Set(pos, p0);
		sig1_dqnprob.Set(pos, p1);
		
		label.Set(pos, before.action);
		
		LOG(GetSymbol() << ": Act " << dqn_round << " (" << pos << "): p0=" << p0 << " p1=" << p1 << ": " << next << " / " << curr << "   " << before.reward);
		
		if (dqn_round > 100) {
			for(int j = 0; j < 5; j++)
				dqn_trainer.LearnAny(dqn_round);
		}
		
		dqn_round++;
	}
	
	
	// This is only for progress drawer...
	RunMain();
	
	
	// Stop eventually
	if (dqn_round >= dqn_max_rounds) {
		SetJobFinished();
	}
	
	
	return true;
}

bool DqnAdvisor::TrainingDQNEnd() {
	ForceSetCounted(0);
	RefreshAll();
	return true;
}

bool DqnAdvisor::TrainingDQNInspect() {
	bool succ = dqntraining_pts.Top() > 0.0;
	
	INSPECT(succ, "warning: negative result");
	
	return true;
}








void DqnAdvisor::RunMain() {
	ConstVectorBool& label = GetOutput(0).label;
	
	int bars = GetBars() - 1;
	
	double change_total	= 0.0;
	
	for(int i = 0; i < bars; i++) {
		bool signal		= label.Get(i);
		double curr		= open_buf->GetUnsafe(i);
		double next		= open_buf->GetUnsafe(i + 1);
		double change	= next / curr - 1.0;
		ASSERT(curr > 0.0);
		
		if (signal) change *= -1.0;
		change_total	+= change;
		
		dqntraining_pts[i] = change_total;
	}
	dqntraining_pts[bars] = change_total;
}

void DqnAdvisor::SetTrainingArea() {
	int tf = GetTf();
	int data_count = GetSystem().GetCountTf(tf);
	area.FillArea(data_count);
}

void DqnAdvisor::SetRealArea() {
	int week				= 1*5*24*60 / MAIN_PERIOD_MINUTES;
	int data_count			= open_buf->GetCount();
	area.train_begin		= week;
	area.train_end			= data_count - 2*week;
	area.test0_begin		= data_count - 2*week;
	area.test0_end			= data_count - 1*week;
	area.test1_begin		= data_count - 1*week;
	area.test1_end			= data_count;
}

void DqnAdvisor::FillBufferSource(const AccuracyConf& conf, ConstBufferSource& bufs) {
	int depth = TRUEINDI_COUNT * (conf.fastinput + 1) + 1;
	bufs.SetDepth(depth);
	ASSERT(depth >= 2);
	
	
	int k = 0;
	int symbol = GetSymbol();
	int tf = conf.GetBaseTf(0);
	
	ASSERT(TRUEINDI_COUNT == indi_count);
	for(int i = 0; i < TRUEINDI_COUNT; i++) {
		for(int j = 0; j < conf.fastinput+1; j++) {
			int l = tf-j;
			ASSERT(l >= 0 && l < TF_COUNT);
			ConstBuffer& buf = GetInputBuffer(1 + i + l * (indi_count + label_count), 0);
			ASSERT(&buf != NULL);
			bufs.SetSource(k++, buf);
		}
	}
	bufs.SetSource(k++, GetInputBuffer(1 + TF_COUNT * (indi_count + label_count), 0));
	
	
	ASSERT(k == depth);
}

void DqnAdvisor::RefreshOutputBuffers() {
	int bars = GetBars();
	
	if (full_mask.GetCount() != bars)
		full_mask.SetCount(bars).One();
	
	SetSafetyLimit(bars);
	
	data.SetCount(bars);
	
	ConstBufferSource bufs;
	
	rf_trainer.forest.tree_count = rf_trainer.options.tree_count;
	
	for (int p = 0; p < 2; p++) {
		double mul = p == 0 ? +1.0 : -1.0;
		
		Array<RF>& rflist = p == 0 ? rflist_pos : rflist_neg;
		
		for(int i = 0; i < rflist.GetCount(); i++) {
			RF& rf = rflist[i];
			AccuracyConf& conf = rf.a;
			VectorBool& real_mask = rf.c;
			
			if (!conf.is_processed || conf.id == -1) continue;
			
			
			rf_trainer.forest.memory.Attach(&rf.b);
			FillBufferSource(conf, bufs);
			
			int cursor = prev_counted;
			ConstBufferSourceIter iter(bufs, &cursor);
			
			#if DEBUG_BUFFERS
			Buffer& buf = GetBuffer(main_graphs + p + i * 2);
			#endif
			
			for(; cursor < bars; cursor++) {
				SetSafetyLimit(cursor);
				double prob = rf_trainer.forest.PredictOne(iter);
				double d = mul * (prob * 2.0 - 1.0);
				if (d > +1.0) d = +1.0;
				if (d < -1.0) d = -1.0;
				
				DQN::DQItem& before = data[cursor];
				int pos = p + i * 4;
				before.state.Set(pos, prob);
				if (cursor > 0) {
					DQN::DQItem& before2 = data[cursor - 1];
					double prev_prob = before2.state.Get(pos);
					double prob_change = (prob - prev_prob) * 5.0;
					before.state.Set(2 + p + i * 4, prob_change);
				}
				
				#if DEBUG_BUFFERS
				buf.Set(cursor, d);
				#endif
			}
			rf_trainer.forest.memory.Detach();
		}
	}
	
}

void DqnAdvisor::RefreshMain() {
	int bars = GetBars();
	int cursor = prev_counted;
	Buffer& sig0_dqnprob = GetBuffer(0);
	Buffer& sig1_dqnprob = GetBuffer(1);
	VectorBool& label = GetOutput(0).label;
	
	data.SetCount(bars);
	
	for(; cursor < bars; cursor++) {
		SetSafetyLimit(cursor);
		DQN::DQItem& before = data[cursor];
		before.action = dqn_trainer.Act(before);
		label.Set(cursor, before.action);
		sig0_dqnprob.Set(cursor, dqn_trainer.data.add2.output.Get(0) * 0.2);
		sig1_dqnprob.Set(cursor, dqn_trainer.data.add2.output.Get(1) * 0.2);
	}
}


void DqnAdvisor::SourceSearchCtrl::Paint(Draw& w) {
	Size sz = GetSize();
	ImageDraw id(sz);
	id.DrawRect(sz, White());
	
	DqnAdvisor* rfa = dynamic_cast<DqnAdvisor*>(&*job->core);
	ASSERT(rfa);
	DrawVectorPoints(id, sz, rfa->search_pts);
	
	w.DrawImage(0, 0, id);
}

void DqnAdvisor::TrainingRFCtrl::Paint(Draw& w) {
	Size sz = GetSize();
	ImageDraw id(sz);
	id.DrawRect(sz, White());
	
	DqnAdvisor* rfa = dynamic_cast<DqnAdvisor*>(&*job->core);
	ASSERT(rfa);
	DrawVectorPolyline(id, sz, rfa->training_pts, polyline);
	
	w.DrawImage(0, 0, id);
}

void DqnAdvisor::TrainingDQNCtrl::Paint(Draw& w) {
	Size sz = GetSize();
	ImageDraw id(sz);
	id.DrawRect(sz, White());
	
	DqnAdvisor* rfa = dynamic_cast<DqnAdvisor*>(&*job->core);
	ASSERT(rfa);
	DrawVectorPolyline(id, sz, rfa->dqntraining_pts, polyline);
	
	w.DrawImage(0, 0, id);
}


}
