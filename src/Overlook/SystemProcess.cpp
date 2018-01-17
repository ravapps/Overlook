#include "Overlook.h"

namespace Overlook {

void System::StartMain() {
	StopMain();
	main_stopped = false;
	main_running = true;
	Thread::Start(THISBACK(MainLoop));
}

void System::StopMain() {
	main_running = false;
	while (!main_stopped || workers_started) Sleep(100);
}

void System::MainLoop() {
	main_mem.SetCount(MEM_COUNT, 0);
	logic0.SetCount(GetCommonCount());
	logic1.SetCount(GetCommonCount());
	logic2.SetCount(GetCommonCount());
	workers_started = 0;
	
	int workers = Upp::max(1, CPU_Cores() - 1);
	for(int i = 0; i < workers; i++) {
		Thread::Start(THISBACK1(Worker, i));
		workers_started++;
	}
	
	
	for(int i = 0; i < REG_COUNT; i++) main_reg[i] = 0;
	
	while (main_running && !Thread::IsShutdownThreads()) {
		dword& ins = main_reg[REG_INS];
		
		Time t = GetMetaTrader().GetTime();
		int step = t.minute / 15;
		
		switch (ins) {
			
		case INS_WAIT_NEXTSTEP:
			// Wait until new data position
			if (prev_step == step) {
				Sleep(100);
				break;
			}
			prev_step = step;
			ins++;
			break;
		
		case INS_REFRESHINDI:
			RealizeMainWorkQueue();
			ProcessMainWorkQueue(true);
			ins++;
			break;
		
		case INS_INDIBITS:
			FillIndicatorBits();
			ins++;
			break;
		
		case INS_TRAINABLE:
			FillTrainableBits();
			ins++;
			break;
			
		case INS_REALIZE_LOGICTRAINING:
			RealizeLogicTraining();
			ins++;
			break;
		
		case INS_WAIT_LOGICTRAINING:
			for (int l = 0; l < level_count; l++) {
				dword& is_training = main_reg[REG_LOGICTRAINING_L0_ISRUNNING + l];
				dword& is_trained = main_mem[MEM_TRAINED_L0 + l];
				if (is_training) {
					if (main_jobs.IsEmpty()) {
						is_training = false;
						is_trained = true;
					} else {
						Sleep(100);
						break;
					}
				}
			}
			ins++;
			break;
			
		case INS_LOGICBITS:
			FillLogicBits();
			
			// Require trained logic
			if (main_mem[MEM_COUNTED_L2] == 0)
				ins = 0;
			else
				ins++;
			break;
		
		case INS_REFRESH_REAL:
			RefreshReal();
			ins = 0;
			break;
			
		}
		
		
	}
	
	main_running = false;
	main_stopped = true;
}

void System::RealizeMainWorkQueue() {
	if (main_reg[REG_WORKQUEUE_INITED])
		return;
	
	ASSERT(main_tf_ids.IsEmpty());
	ASSERT(main_sym_ids.IsEmpty());
	ASSERT(main_indi_ids.IsEmpty());
	
	FactoryDeclaration decl;

	decl.factory = Find<DataBridge>();							main_indi_ids.Add(decl);
	decl.factory = Find<MovingAverage>();						main_indi_ids.Add(decl);
	decl.factory = Find<MovingAverageConvergenceDivergence>();	main_indi_ids.Add(decl);
	decl.factory = Find<BollingerBands>();						main_indi_ids.Add(decl);
	decl.factory = Find<ParabolicSAR>();						main_indi_ids.Add(decl);
	decl.factory = Find<StandardDeviation>();					main_indi_ids.Add(decl);
	decl.factory = Find<AverageTrueRange>();					main_indi_ids.Add(decl);
	decl.factory = Find<BearsPower>();							main_indi_ids.Add(decl);
	decl.factory = Find<BullsPower>();							main_indi_ids.Add(decl);
	decl.factory = Find<CommodityChannelIndex>();				main_indi_ids.Add(decl);
	decl.factory = Find<DeMarker>();							main_indi_ids.Add(decl);
	decl.factory = Find<Momentum>();							main_indi_ids.Add(decl);
	decl.factory = Find<RelativeStrengthIndex>();				main_indi_ids.Add(decl);
	decl.factory = Find<RelativeVigorIndex>();					main_indi_ids.Add(decl);
	decl.factory = Find<StochasticOscillator>();				main_indi_ids.Add(decl);
	decl.factory = Find<AcceleratorOscillator>();				main_indi_ids.Add(decl);
	decl.factory = Find<AwesomeOscillator>();					main_indi_ids.Add(decl);
	decl.factory = Find<PeriodicalChange>();					main_indi_ids.Add(decl);
	decl.factory = Find<VolatilityAverage>();					main_indi_ids.Add(decl);
	decl.factory = Find<VolatilitySlots>();						main_indi_ids.Add(decl);
	decl.factory = Find<VolumeSlots>();							main_indi_ids.Add(decl);
	decl.factory = Find<ChannelOscillator>();					main_indi_ids.Add(decl);
	decl.factory = Find<ScissorChannelOscillator>();			main_indi_ids.Add(decl);
	decl.factory = Find<CommonForce>();							main_indi_ids.Add(decl);
	decl.factory = Find<CorrelationOscillator>();				main_indi_ids.Add(decl);
	
	main_factory_ids.Clear();
	for(int i = 0; i < main_indi_ids.GetCount(); i++)
		main_factory_ids.Add(main_indi_ids[i].factory);
	
	main_tf_ids.Add(FindPeriod(5));
	main_tf_ids.Add(FindPeriod(15));
	main_tf_ids.Add(FindPeriod(60));
	//tf_ids.Add(FindPeriod(240));
	ASSERT(main_tf_ids.GetCount() == TF_COUNT);
	
	for(int i = 0; i < GetCommonCount(); i++) {
		for(int j = 0; j < GetCommonSymbolCount(); j++)
			main_sym_ids.Add(GetCommonSymbolId(i, j));
		main_sym_ids.Add(GetCommonSymbolId(i));
	}
	
	main_work_queue.Clear();
	GetCoreQueue(main_work_queue, main_sym_ids, main_tf_ids, main_indi_ids);
	
	
	ProcessMainWorkQueue(true);
	if (!main_running) return;
	
	
	// Save data count at the time of queue processing
	int bars = GetCountMain(main_tf_ids[main_tf_pos]);
	main_mem[MEM_INDIBARS] = bars;
	
	
	int ordered_cores_total = main_tf_ids.GetCount() * main_sym_ids.GetCount() * main_indi_ids.GetCount();
	ordered_cores.Clear();
	ordered_cores.SetCount(ordered_cores_total, NULL);
	
	for(int i = 0; i < main_work_queue.GetCount(); i++) {
		CoreItem& ci = *main_work_queue[i];
		ASSERT(&*ci.core != NULL);
		
		int sym_pos = main_sym_ids.Find(ci.sym);
		int tf_pos = main_tf_ids.Find(ci.tf);
		int factory_pos = main_factory_ids.Find(ci.factory);
		
		if (sym_pos == -1) Panic("Symbol not found");
		if (tf_pos == -1) Panic("Tf not found");
		if (factory_pos == -1) Panic("Factory not found");
		
		int core_pos = GetOrderedCorePos(sym_pos, tf_pos, factory_pos);
		ASSERT(ordered_cores[core_pos] == NULL);
		ordered_cores[core_pos] = &*ci.core;
	}
	
	for(int i = 0; i < ordered_cores.GetCount(); i++) {
		ASSERT(ordered_cores[i] != NULL);
	}
	
	
	// Find starting position
	main_begin.SetCount(GetCommonCount(), 0);
	for(int c = 0; c < GetCommonCount(); c++) {
		int common_id = GetCommonSymbolId(c);
		int main_tf  = main_tf_ids[main_tf_pos];
		int& begin = main_begin[c];
		for(int i = 0; i < main_tf_ids.GetCount(); i++) {
			int tf = main_tf_ids[i];
			
			for(int j = 0; j < GetCommonSymbolCount(); j++) {
				int sym_id = GetCommonSymbolId(c, j);
				int pos = GetShiftTf(sym_id, tf, common_id, main_tf, 1);
				if (pos > begin) begin = pos;
			}
		}
		
		int data_count = bars - begin;
		int test_count = 4*5*24*4;
		int training_count = data_count - test_count;
		if (test_count > training_count)
			throw DataExc("Not enough data for symbols in common group " + IntStr(c));
	}
	
	
	main_reg[REG_WORKQUEUE_INITED] = true;
}

void System::ProcessMainWorkQueue(bool store_cache) {
	EnterProcessing();
	dword& queue_cursor = main_reg[REG_WORKQUEUE_CURSOR];
	for (queue_cursor = 0; queue_cursor < main_work_queue.GetCount() && main_running; queue_cursor++)
		Process(*main_work_queue[queue_cursor], store_cache);
	LeaveProcessing();
}

void System::FillIndicatorBits() {
	ASSERT(!main_tf_ids.IsEmpty());
	ASSERT(!main_sym_ids.IsEmpty());
	ASSERT(!main_indi_ids.IsEmpty());
	
	dword& cursor = main_mem[MEM_COUNTED_INDI];
	
	if (cursor > 0)
		cursor--; // set previous BIT_REALSIGNAL
	
	VectorBool vec;
	vec.SetCount(ASSIST_COUNT);
	
	int bars = main_mem[MEM_INDIBARS];
	ASSERT(bars > 0);
	int main_data_count = GetMainDataPos(bars, 0, 0, 0);
	main_data.SetCount(main_data_count);
	
	for (; cursor < bars && main_running; cursor++) {
		
		for(int i = 0; i < main_tf_ids.GetCount(); i++) {
			int tf = main_tf_ids[i];
			
			for(int j = 0; j < main_sym_ids.GetCount(); j++) {
				int sym = main_sym_ids[j];
				int core_cursor = GetShiftFromMain(sym, tf, cursor);
				
				vec.Zero();
				
				for(int k = 0; k < main_indi_ids.GetCount(); k++) {
					int core_pos = GetOrderedCorePos(j, i, k);
					Core& core = *ordered_cores[core_pos];
					core.Assist(core_cursor, vec);
					
					if (k == 0 && cursor < bars - 1) {
						ConstBuffer& open_buf = core.GetBuffer(0);
						double next = open_buf.GetUnsafe(core_cursor + 1);
						double curr = open_buf.GetUnsafe(core_cursor);
						bool signal = next < curr;
						int pos = GetMainDataPos(cursor, j, i, BIT_REALSIGNAL);
						main_data.Set(pos, signal);
						pos = GetMainDataPos(cursor, j, i, BIT_WRITTEN_REAL);
						main_data.Set(pos, true);
					}
				}
				
				int main_pos = GetMainDataPos(cursor, j, i, BIT_L0BITS_BEGIN);
				for(int k = 0; k < ASSIST_COUNT; k++) {
					main_data.Set(main_pos++, vec.Get(k));
				}
			}
		}
	}
	
	main_reg[REG_INDIBITS_INITED] = true;
}

int System::GetOrderedCorePos(int sym_pos, int tf_pos, int factory_pos) {
	return (sym_pos * main_tf_ids.GetCount() + tf_pos) * main_factory_ids.GetCount() + factory_pos;
}

void System::FillTrainableBits() {
	
	// Set trainable bits only once to keep fixed training dataset
	if (main_mem[MEM_TRAINABLESET])
		return;
	
	dword bars = main_mem[MEM_INDIBARS];
	ASSERT(bars > 0);
	
	dword& train_bars		= main_mem[MEM_TRAINBARS];
	dword& train_midstep	= main_mem[MEM_TRAINMIDSTEP];
	dword& train_begin		= main_mem[MEM_TRAINBEGIN];
	
	train_bars = bars;
	train_midstep = train_bars - 4*5*24*4;
	train_begin = main_begin[main_tf_pos];
	ASSERT(train_begin < train_midstep && train_midstep < train_bars);
	
	
	main_mem[MEM_TRAINABLESET] = true;
}

void System::RealizeLogicTraining() {
	for(int l = 0; l < level_count; l++) {
		dword& is_trained = main_mem[MEM_TRAINED_L0 + l];
		
		if (!is_trained) {
			dword& is_training = main_reg[REG_LOGICTRAINING_L0_ISRUNNING + l];
			if (!is_training) {
				for(int i = 0; i < GetCommonCount(); i++) LearnLogic(l, i);
				is_training = true;
			}
			return;
		}
	}
}

void System::LearnLogic(int level, int common_pos) {
	main_lock.EnterWrite();
	MainJob& job		= main_jobs.Add();
	job.level			= level;
	job.common_pos		= common_pos;
	job.being_processed	= 0;
	main_lock.LeaveWrite();
}

void System::Worker(int id) {
	MainJob* rem_job = NULL;
	
	while (main_running && !Thread::IsShutdownThreads()) {
		bool processed_something = false;
		main_lock.EnterRead();
		for(int i = 0; i < main_jobs.GetCount(); i++) {
			MainJob& job = main_jobs[i];
			if (job.is_finished)
				continue;
			bool got_job = ++job.being_processed == 1;
			if (got_job) {
				job.is_finished = ProcessMainJob(job);
				processed_something = true;
			}
			job.being_processed--;
			if (job.is_finished) {
				rem_job = &job;
				break;
			}
		}
		main_lock.LeaveRead();
		
		if (rem_job) {
			main_lock.EnterWrite();
			for(int i = 0; i < main_jobs.GetCount(); i++) {
				MainJob& job = main_jobs[i];
				if (job.is_finished && job.being_processed == 0) {
					main_jobs.Remove(i);
					i--;
					continue;
				}
			}
			main_lock.LeaveWrite();
			rem_job = NULL;
		}
		
		if (!processed_something) Sleep(100);
	}
	
	workers_started--;
}

template <class T> int TrainLogic(System::MainJob& job, System& sys, T& logic) {
	job.actual = logic.dqn_round;
	job.total = logic.dqn_max_rounds;
	
	
	// Random event probability (not used anyway)
	logic.dqn_trainer.SetEpsilon(0);
	
	// Future reward discount factor.
	// 0 means, that the loss or reward event is completely separate event from future events.
	logic.dqn_trainer.SetGamma(0);
	
	// Learning rate. Decreases over training.
	double max_alpha = 0.01;
	double min_alpha = 0.0001;
	double alpha = (max_alpha - min_alpha) * (logic.dqn_max_rounds - logic.dqn_round) / (double)logic.dqn_max_rounds + min_alpha;
	logic.dqn_trainer.SetAlpha(alpha);
	
	const dword train_bars		= sys.main_mem[System::MEM_TRAINBARS];
	const dword train_midstep	= sys.main_mem[System::MEM_TRAINMIDSTEP];
	const dword train_begin		= sys.main_mem[System::MEM_TRAINBEGIN];
	
	int data_count;
	if      (job.level == 0)		data_count = train_midstep - train_begin;
	else if (job.level == 1)		data_count = train_bars - train_begin;
	else if (job.level == 2)		data_count = train_bars - train_midstep;
	else Panic("Invalid level");
	ASSERT(data_count > 0);
	
	double correct[logic.dqn_trainer.OUTPUT_SIZE];
	typename T::DQN::MatType tmp_before_state, tmp_after_state;
	
	double av_tderror = 0.0;
	for(int i = 0; i < 10; i++) {
		int cursor = train_begin + logic.dqn_round % (data_count - 1);
		
		for(int j = 0; j < 5; j++) {
			int count = Upp::min(data_count - 1, logic.dqn_round - 1);
			if (count < 1) break;
			int pos = train_begin + Random(count);
			if (pos < 0 || pos >= train_bars) continue;
			sys.LoadInput(job.level, job.common_pos, pos, tmp_before_state);
			sys.LoadInput(job.level, job.common_pos, pos+1, tmp_after_state);
			sys.LoadOutput(job.level, job.common_pos, pos, correct, logic.dqn_trainer.OUTPUT_SIZE);
			av_tderror = logic.dqn_trainer.Learn(tmp_before_state, correct, tmp_after_state);
		}
		
		logic.dqn_round++;
	}
	
	job.training_pts.Add(av_tderror);
	
	return logic.dqn_round >= logic.dqn_max_rounds;
}

int System::ProcessMainJob(MainJob& job) {
	int ret = 0;
	if      (job.level == 0)		ret = TrainLogic(job, *this, logic0[job.common_pos]);
	else if (job.level == 1)		ret = TrainLogic(job, *this, logic1[job.common_pos]);
	else if (job.level == 2)		ret = TrainLogic(job, *this, logic2[job.common_pos]);
	else Panic("Invalid level");
	return ret;
}

template <class T> void FillLogicBits(int level, int common_pos, System& sys, T& logic) {
	dword bars					= sys.main_mem[System::MEM_INDIBARS];
	dword& cursor				= sys.main_mem[System::MEM_COUNTED_L0 + level];
	const dword output_size		= sys.main_mem[System::MEM_OUTPUT_L0 + level];
	ASSERT(output_size > 0);
	
	int sym_count = sys.GetCommonSymbolCount() + 1;
	int sym_begin = common_pos * sym_count;
	
	double evaluated[output_size];
	typename T::DQN::MatType tmp_before_state, tmp_after_state;
	
	for (; cursor < bars; cursor++) {
		
		sys.LoadInput(level, common_pos, cursor, tmp_before_state);
		sys.LoadInput(level, common_pos, cursor+1, tmp_after_state);
		sys.LoadOutput(level, common_pos, cursor, evaluated, output_size);
		
		int action_begin = sys.main_mem[System::MEM_ACTIONBEGIN_L0 + level];
		int action_count = sys.main_mem[System::MEM_ACTIONCOUNT_L0 + level];
		ASSERT(action_begin > 0 && action_count > 0);
		
		int out_pos = 0;
		for(int i = 0; i < sym_count; i++) {
			int sym = sym_begin + i;
			for(int j = 0; j < action_count; j++) {
				bool bit_value = evaluated[out_pos++] < 0.5;
				int bit_pos = sys.GetMainDataPos(cursor, sym, sys.main_tf_pos, action_begin + j);
				sys.main_data.Set(bit_pos, bit_value);
			}
		}
		ASSERT(out_pos == output_size);
	}
}

void System::FillLogicBits() {
	for(int l = 0; l < level_count; l++) {
		bool is_trained				= main_mem[MEM_TRAINED_L0 + l];
		if (!is_trained) break;
		
		for(int common_pos = 0; common_pos < GetCommonCount(); common_pos++) {
			if      (l == 0)		::Overlook::FillLogicBits(l, common_pos, *this, logic0[common_pos]);
			else if (l == 1)		::Overlook::FillLogicBits(l, common_pos, *this, logic1[common_pos]);
			else if (l == 2)		::Overlook::FillLogicBits(l, common_pos, *this, logic2[common_pos]);
			else Panic("Invalid level");
		}
		
		Panic("Set something");
	}
}

void System::RefreshReal() {
	
	Time now = GetUtcTime();
	int64 step = periods[main_tf_pos] * 60;
	now -= now.Get() % step;
	int current_main_pos = main_time[main_tf_pos].Find(now);
	
	if (current_main_pos == -1) {
		LOG("error: current main pos not found");
		return;
	}
	
	
	int wday				= DayOfWeek(now);
	Time after_3hours		= now + 3 * 60 * 60;
	int wday_after_3hours	= DayOfWeek(after_3hours);
	now.second				= 0;
	MetaTrader& mt			= GetMetaTrader();
	
	
	// Skip weekends and first hours of monday
	if (wday == 0 || wday == 6 || (wday == 1 && now.hour < 1)) {
		LOG("Skipping weekend...");
		return;
	}
	
	
	// Inspect for market closing (weekend and holidays)
	else if (wday == 5 && wday_after_3hours == 6) {
		WhenInfo("Closing all orders before market break");

		for (int i = 0; i < mt.GetSymbolCount(); i++) {
			mt.SetSignal(i, 0);
			mt.SetSignalFreeze(i, false);
		}

		mt.SignalOrders(true);
		return;
	}
	
	
	
	WhenInfo("Updating MetaTrader");
	WhenPushTask("Putting latest signals");
	
	// Reset signals
	if (realtime_count == 0) {
		for (int i = 0; i < mt.GetSymbolCount(); i++)
			mt.SetSignal(i, 0);
	}
	realtime_count++;
	
	
	try {
		mt.Data();
		mt.RefreshLimits();
		int open_count = 0;
		Vector<int> signals;
		for(int i = 0; i < GetCommonCount(); i++) {
			for(int j = 0; j < GetCommonSymbolCount(); j++) {
				int sym_id = GetCommonSymbolId(i, j);
				int sym_pos = main_sym_ids.Find(sym_id);
				
				// Do some quality checks
				{
					int pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_WRITTEN_REAL);
					int not_written_real = main_data.Get(pos);
					pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_WRITTEN_L0);
					int not_written_L0 = main_data.Get(pos);
					pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_WRITTEN_L1);
					int not_written_L1 = main_data.Get(pos);
					pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_WRITTEN_L2);
					int not_written_L2 = main_data.Get(pos);
					int e = (not_written_real << 3) | (not_written_L0 << 2) | (not_written_L0 << 1) | (not_written_L0 << 0);
					if (e) throw UserExc("Real account function quality check failed: error code " + IntStr(e) + " sym=" + IntStr(sym_id));
				}
				
				int signal_pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_L1_SIGNAL);
				int enabled_pos = GetMainDataPos(current_main_pos, sym_pos, main_tf_pos, BIT_L2_ENABLED);
				bool signal_bit = main_data.Get(signal_pos);
				bool enabled_bit = main_data.Get(enabled_pos);
				int sig = enabled_bit ? (signal_bit ? -1 : +1) : 0;
				int prev_sig = mt.GetSignal(sym_id);
				signals.Add(sig);
				if (sig == prev_sig && sig != 0)
					open_count++;
			}
		}
		int sig_pos = 0;
		for(int i = 0; i < GetCommonCount(); i++) {
			for(int j = 0; j < GetCommonSymbolCount(); j++) {
				int sym_id = GetCommonSymbolId(i, j);
				int sym_pos = main_sym_ids.Find(sym_id);
				int sig = signals[sig_pos++];
				int prev_sig = mt.GetSignal(sym_id);
				
				if (sig == prev_sig && sig != 0)
					mt.SetSignalFreeze(sym_id, true);
				else {
					if (!prev_sig && sig) {
						if (open_count >= GetCommonSymbolCount())
							sig = 0;
						else
							open_count++;
					}
					mt.SetSignal(sym_id, sig);
					mt.SetSignalFreeze(sym_id, false);
				}
				LOG("Real symbol " << sym_id << " signal " << sig);
			}
		}
		
		mt.SetFreeMarginLevel(FMLEVEL);
		mt.SetFreeMarginScale(GetCommonSymbolCount());
		mt.SignalOrders(true);
	}
	catch (...) {
		
	}
	
	
	WhenRealtimeUpdate();
	WhenPopTask();
}

int System::GetCommonSymbolId(int common_pos, int symbol_pos) const {
	ASSERT(common_pos >= 0 && common_pos < COMMON_COUNT);
	ASSERT(symbol_pos >= 0 && symbol_pos < SYM_COUNT);
	return sym_priority[common_pos * SYM_COUNT + symbol_pos];
}

int System::FindCommonSymbolId(int sym_id) const {
	return common_symbol_id[sym_id];
}

int System::FindCommonSymbolPos(int sym_id) const {
	return common_symbol_pos[sym_id];
}

int System::FindCommonSymbolPos(int common_id, int sym_id) const {
	return common_symbol_group_pos[common_id][sym_id];
}

int System::GetMainDataPos(int cursor, int sym_pos, int tf_pos, int bit_pos) const {
	ASSERT(cursor >= 0 && cursor <= main_mem[MEM_INDIBARS]);
	ASSERT(sym_pos >= 0 && sym_pos < main_sym_ids.GetCount());
	ASSERT(tf_pos >= 0 && tf_pos < main_tf_ids.GetCount());
	ASSERT(bit_pos >= 0 && bit_pos < BIT_COUNT);
	return (((cursor * main_sym_ids.GetCount() + sym_pos) * main_tf_ids.GetCount() + tf_pos) * BIT_COUNT + bit_pos);
}

void System::LoadInput(int level, int common_pos, int cursor, double* buf, int bufsize) {
	int buf_pos = 0;
	
	// Time bits
	for(int i = 0; i < 5+24+4; i++)
		buf[buf_pos + i] = 1.0;
	
	Time t = GetTimeMain(main_tf_pos, cursor);
	
	int wday = Upp::max(0, Upp::min(5, DayOfWeek(t) - 1));
	buf[buf_pos + wday] =0.0;
	buf_pos += 5;
	
	buf[buf_pos + t.hour] = 0.0;
	buf_pos += 24;
	
	buf[buf_pos + t.minute / 15] = 0.0;
	buf_pos += 4;
	
	if (level == 0 || level == 1) {
		for(int i = 0; i <= GetCommonSymbolCount(); i++) {
			int sym_pos = common_pos * (GetCommonSymbolCount() + 1) + i;
			for(int j = 0; j < main_tf_ids.GetCount(); j++) {
				int pos = GetMainDataPos(cursor, sym_pos, j, BIT_L0BITS_BEGIN);
				for(int k = 0; k < ASSIST_COUNT; k++) {
					bool b = main_data.Get(pos);
					buf[buf_pos] = b ? 0.0 : 1.0;
					buf_pos++;
					pos++;
				}
			}
		}
		ASSERT(buf_pos == bufsize);
		ASSERT(buf_pos == LogicLearner0::INPUT_SIZE);
	}
	
	else if (level == 2) {
		for(int i = 0; i <= GetCommonSymbolCount(); i++) {
			int sym_pos = common_pos * (GetCommonSymbolCount() + 1) + i;
			for(int j = 0; j < main_tf_ids.GetCount(); j++) {
				int pos = GetMainDataPos(cursor, sym_pos, j, BIT_L2BITS_BEGIN);
				for(int k = 0; k < L2_INPUT; k++) {
					bool b = main_data.Get(pos);
					buf[buf_pos] = b ? 0.0 : 1.0;
					buf_pos++;
					pos++;
				}
			}
		}
		ASSERT(buf_pos == bufsize);
		ASSERT(buf_pos == LogicLearner2::INPUT_SIZE);
	}
	
	else Panic("Invalid level");
	
}

void System::LoadOutput(int level, int common_pos, int cursor, double* buf, int bufsize) {
	
	if (level == 0 || level == 1) {
		int buf_pos = 0;
		for(int i = 0; i <= GetCommonSymbolCount(); i++) {
			int sym_pos = common_pos * (GetCommonSymbolCount() + 1) + i;
			for(int j = 0; j < main_tf_ids.GetCount(); j++) {
				int pos = GetMainDataPos(cursor, sym_pos, j, BIT_REALSIGNAL);
				bool action = main_data.Get(pos);
				buf[buf_pos++] = !action ? 0.0 : 1.0;
				buf[buf_pos++] =  action ? 0.0 : 1.0;
			}
		}
		ASSERT(buf_pos == bufsize);
		ASSERT(buf_pos == LogicLearner0::OUTPUT_SIZE);
	}
	
	else if (level == 2) {
		int buf_pos = 0;
		for(int i = 0; i <= GetCommonSymbolCount(); i++) {
			int sym_pos = common_pos * (GetCommonSymbolCount() + 1) + i;
			for(int j = 0; j < main_tf_ids.GetCount(); j++) {
				int pos = GetMainDataPos(cursor, sym_pos, j, BIT_L2_REALENABLED);
				bool enabled = main_data.Get(pos);
				buf[buf_pos++] = enabled ? 0.0 : 1.0;
			}
		}
		ASSERT(buf_pos == bufsize);
		ASSERT(buf_pos == LogicLearner2::OUTPUT_SIZE);
	}
	
	else Panic("Invalid level");
	
}

String System::GetRegisterKey(int i) const {
	switch (i) {
		case REG_INS: return "Instruction";
		case REG_WORKQUEUE_CURSOR: return "Work queue cursor";
		case REG_WORKQUEUE_INITED: return "Is workqueue initialised";
		case REG_INDIBITS_INITED: return "Is indicator bits initialised";
		case REG_LOGICTRAINING_L0_ISRUNNING: return "Is level 0 training running";
		case REG_LOGICTRAINING_L1_ISRUNNING: return "Is level 1 training running";
		case REG_LOGICTRAINING_L2_ISRUNNING: return "Is level 2 training running";
		default: return IntStr(i);
	}
}

String System::GetRegisterValue(int i, int j) const {
	switch (i) {
		case REG_INS:
			switch (j) {
				case INS_WAIT_NEXTSTEP:				return "Waiting next timestep";
				case INS_REFRESHINDI:				return "Refresh indicator";
				case INS_INDIBITS:					return "Refresh indicator bits in main data";
				case INS_TRAINABLE:					return "Refresh training values";
				case INS_REALIZE_LOGICTRAINING:		return "Realize logic training";
				case INS_WAIT_LOGICTRAINING:		return "Wait logic training to finish";
				case INS_LOGICBITS:					return "Refresh logic bits in main data";
				case INS_REFRESH_REAL:				return "Refresh real account";
				default:							return "Unknown";
			};
		case REG_WORKQUEUE_CURSOR:
			return IntStr(j) + "/" + IntStr(main_work_queue.GetCount());
		case REG_WORKQUEUE_INITED:
		case REG_INDIBITS_INITED:
		case REG_LOGICTRAINING_L0_ISRUNNING:
		case REG_LOGICTRAINING_L1_ISRUNNING:
		case REG_LOGICTRAINING_L2_ISRUNNING:
			return j ? "Yes" : "No";
		default:
			return IntStr(j);
	}
}

String System::GetMemoryKey(int i) const {
	switch (i) {
		case MEM_TRAINABLESET:		return "Is trainable bits set";
		case MEM_INDIBARS:			return "Bars (for all)";
		case MEM_COUNTED_INDI:		return "Counted for indicators";
		case MEM_TRAINBARS:			return "Bars (for training)";
		case MEM_TRAINMIDSTEP:		return "Midstep (for levels)";
		case MEM_TRAINBEGIN:		return "Beginning of training";
		case MEM_COUNTED_L0:		return "Counted for level 0";
		case MEM_COUNTED_L1:		return "Counted for level 1";
		case MEM_COUNTED_L2:		return "Counted for level 2";
		case MEM_TRAINED_L0:		return "Is level 0 trained";
		case MEM_TRAINED_L1:		return "Is level 1 trained";
		case MEM_TRAINED_L2:		return "Is level 2 trained";
		case MEM_OUTPUT_L0:			return "Ouput size of level 0";
		case MEM_OUTPUT_L1:			return "Ouput size of level 1";
		case MEM_OUTPUT_L2:			return "Ouput size of level 2";
		case MEM_ACTIONBEGIN_L0:	return "Action begin of level 0";
		case MEM_ACTIONBEGIN_L1:	return "Action begin of level 1";
		case MEM_ACTIONBEGIN_L2:	return "Action begin of level 2";
		case MEM_ACTIONCOUNT_L0:	return "Action count of level 0";
		case MEM_ACTIONCOUNT_L1:	return "Action count of level 1";
		case MEM_ACTIONCOUNT_L2:	return "Action count of level 2";
		
		default: return IntStr(i);
	}
}

String System::GetMemoryValue(int i, int j) const {
	switch (i) {
		case MEM_TRAINABLESET:
		case MEM_TRAINED_L0:
		case MEM_TRAINED_L1:
		case MEM_TRAINED_L2:
			return j ? "Yes" : "No";
		case MEM_INDIBARS:
		case MEM_COUNTED_INDI:
		case MEM_TRAINBARS:
		case MEM_TRAINMIDSTEP:
		case MEM_TRAINBEGIN:
		case MEM_COUNTED_L0:
		case MEM_COUNTED_L1:
		case MEM_COUNTED_L2:
		case MEM_OUTPUT_L0:
		case MEM_OUTPUT_L1:
		case MEM_OUTPUT_L2:
		case MEM_ACTIONBEGIN_L0:
		case MEM_ACTIONBEGIN_L1:
		case MEM_ACTIONBEGIN_L2:
		case MEM_ACTIONCOUNT_L0:
		case MEM_ACTIONCOUNT_L1:
		case MEM_ACTIONCOUNT_L2:
		default:
			return IntStr(j);
	}
}


/*

void RunSimBroker() {
	System& sys = GetSystem();
	DataBridge* db = dynamic_cast<DataBridge*>(GetInputCore(0, GetSymbol(), GetTf()));
	
	Buffer& open_buf	= db->GetBuffer(0);
	Buffer& low_buf		= db->GetBuffer(1);
	Buffer& high_buf	= db->GetBuffer(2);
	Buffer& volume_buf	= db->GetBuffer(3);
	
	int sym = GetSymbol();
	int tf = GetTf();
	int bars = GetBars();
	db->ForceCount(bars);
	
	
	sb.Brokerage::operator=(GetMetaTrader());
	sb.SetInitialBalance(10000);
	sb.Init();
	sb.SetFreeMarginLevel(FMLEVEL);
	
	for(int i = 0; i < SYM_COUNT; i++) {
		check_sum1[i] = 0;
		check_sum2[i] = 0;
	}
	
	for(int i = main_begin; i < bars; i++) {
		DQN::DQVectorType& current = data[i];
		
		for(int j = 0; j < SYM_COUNT; j++) {
			int pos = sys.GetShiftTf(GetSymbol(), tf_ids[0], sys.GetCommonSymbol(j), tf_ids[0], i);
			ConstBuffer& open_buf = *this->open_buf[j];
			double curr = open_buf.GetUnsafe(pos);
			int symbol = sys.GetCommonSymbol(j);
			sb.SetPrice(symbol, curr);
		}
		sb.RefreshOrders();
		
		Time time = sys.GetTimeTf(sym, tf, i);
		int t = time.hour * 100 + time.minute;
		int open_count = 0;
		
		for(int j = 0; j < SYM_COUNT; j++) {
			int sym			= sys.GetCommonSymbol(j);
			int prev_sig	= sb.GetSignal(sym);
			bool prev_signal  = prev_sig == +1 ? false : true;
			bool prev_enabled = prev_sig !=  0;
			
			int sig = 0;
			
			if (is_enabled) {
				sig = signal ? -1 : +1;
				open_count++;
			}
			else if (is_priority)
				open_count++;
			
			
			if (sig == sb.GetSignal(sym) && sig != 0)
				sb.SetSignalFreeze(sym, true);
			else {
				sb.SetSignal(sym, sig);
				sb.SetSignalFreeze(sym, false);
			}
		}
		
		sb.SetFreeMarginScale(open_count ? open_count : 1);
		
		sb.SignalOrders(false);
		
		
		double value = sb.AccountEquity();
		if (value < 0.0) {
			sb.ZeroEquity();
			value = 0.0;
		}
		open_buf.Set(i, value);
		low_buf.Set(i, value);
		high_buf.Set(i, value);
	}
}
*/
	
}
