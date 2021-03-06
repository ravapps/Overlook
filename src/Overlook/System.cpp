#include "Overlook.h"

namespace Overlook {







System::System() {
	allowed_symbols.Add("AUDCAD");
	allowed_symbols.Add("AUDJPY");
	allowed_symbols.Add("AUDNZD");
	allowed_symbols.Add("AUDUSD");
	allowed_symbols.Add("CADJPY");
	allowed_symbols.Add("CHFJPY");
	allowed_symbols.Add("EURCAD");
	allowed_symbols.Add("EURCHF");
	allowed_symbols.Add("EURGBP");
	allowed_symbols.Add("EURJPY");
	allowed_symbols.Add("EURUSD");
	allowed_symbols.Add("EURAUD");
	allowed_symbols.Add("GBPCHF");
	allowed_symbols.Add("GBPUSD");
	allowed_symbols.Add("GBPJPY");
	allowed_symbols.Add("NZDUSD");
	allowed_symbols.Add("USDCAD");
	allowed_symbols.Add("USDCHF");
	allowed_symbols.Add("USDJPY");
	allowed_symbols.Add("USDMXN");
	allowed_symbols.Add("USDTRY");
	
	/*allowed_symbols.Add("#CAT");
	allowed_symbols.Add("#BA");
	allowed_symbols.Add("#MMM");
	allowed_symbols.Add("#HD");*/

}

System::~System() {
	StopJobs();
	data.Clear();
	nndata.Clear();
}

void System::Init() {
	
	MetaTrader& mt = GetMetaTrader();
	try {
		bool connected = mt.Init(Config::arg_addr, Config::arg_port);
		ASSERTUSER_(!connected, "Can't connect to MT4. Is MT4Connection script activated in MT4?");
	}
	catch (UserExc e) {
		throw e;
	}
	catch (Exc e) {
		throw e;
	}
	catch (...) {
		ASSERTUSER_(false, "Unknown error with MT4 connection.");
	}
	
	
	if (symbols.IsEmpty())
		FirstStart();
	else {
		if (mt.GetSymbolCount() != symbols.GetCount())
			throw UserExc("MT4 symbols changed. Remove cached data.");
		for(int i = 0; i < mt.GetSymbolCount(); i++) {
			const Symbol& s = mt.GetSymbol(i);
			if (s.name != symbols[i])
				throw UserExc("MT4 symbols changed. Remove cached data.");
		}
	}
	InitRegistry();
	
	
	for(int i = 0; i < CommonFactories().GetCount(); i++) {
		CommonFactories()[i].b()->Init();
	}
	
	
	#ifdef flagGUITASK
	jobs_tc.Set(10, THISBACK(PostProcessJobs));
	#else
	jobs_running = true;
	jobs_stopped = false;
	Thread::Start(THISBACK(ProcessJobs));
	#endif
	
}

void System::FirstStart() {
	MetaTrader& mt = GetMetaTrader();
	String pair1[4], pair2[4];
	/*
	Index<String> avoid_currencies;
	avoid_currencies.Add("SEK");
	avoid_currencies.Add("HKD");
	avoid_currencies.Add("SGD");
	avoid_currencies.Add("NOK");
	avoid_currencies.Add("MXN");
	avoid_currencies.Add("RUB");
	*/
	try {
		time_offset = mt.GetTimeOffset();
		
		
		// Add symbols and currencies
		for(int i = 0; i < mt.GetSymbolCount(); i++) {
			const Symbol& s = mt.GetSymbol(i);
			if (s.name.Left(6) == "EURUSD") {
				postfix = s.name.Mid(6);
			}
			AddSymbol(s.name);
			
			if (s.IsForex()) {
				String a = s.name.Left(3);
				String b = s.name.Mid(3,3);
				//if (avoid_currencies.Find(a) == -1 && avoid_currencies.Find(b) == -1)
				{
					sym_currencies[i].Add(currencies.FindAdd(a));
					sym_currencies[i].Add(currencies.FindAdd(b));
					currency_syms.GetAdd(a).Add(i);
					currency_syms.GetAdd(b).Add(i);
					currency_sym_dirs.GetAdd(a).Add(i);
					currency_sym_dirs.GetAdd(b).Add(-i-1);
				}
			}
			//ASSERTUSER_(allowed_symbols.Find(s.name) != -1, "Symbol " + s.name + " does not have long M1 data. Please hide all short data symbols in MT4. Read Readme.txt for usable symbols.");
		}
		normal_symbol_count = symbols.GetCount();
		
		for(int i = 0; i < currency_syms.GetCount(); i++) {
			const Index<int>& syms = currency_syms[i];
			if (syms.GetCount() >= 4) {
				major_currency_syms.Add(currency_syms.GetKey(i)) <<= syms;
				major_currencies.Add(i);
			}
		}
		
		AddNet("USD1").Set("EURUSD" + postfix, -1).Set("GBPUSD" + postfix, -1).Set("USDCHF" + postfix, +1);
		AddNet("USD2").Set("EURUSD" + postfix, -1).Set("GBPUSD" + postfix, -1).Set("USDCHF" + postfix, +1).Set("USDJPY" + postfix, +1).Set("USDCAD" + postfix, +1).Set("AUDUSD" + postfix, -1).Set("NZDUSD" + postfix, -1);
		AddNet("EUR1").Set("EURUSD" + postfix, +1).Set("EURGBP" + postfix, +1).Set("EURCHF" + postfix, +1);
		AddNet("EUR2").Set("EURUSD" + postfix, +1).Set("EURGBP" + postfix, +1).Set("EURCHF" + postfix, +1).Set("EURJPY" + postfix, +1).Set("EURCAD" + postfix, +1).Set("EURAUD" + postfix, +1).Set("EURNZD" + postfix, +1);
		AddNet("GBP1").Set("EURGBP" + postfix, -1).Set("GBPUSD" + postfix, +1).Set("GBPCHF" + postfix, +1);
		AddNet("GBP2").Set("EURGBP" + postfix, -1).Set("GBPUSD" + postfix, +1).Set("GBPCHF" + postfix, +1).Set("GBPJPY" + postfix, +1).Set("GBPCAD" + postfix, +1).Set("GBPAUD" + postfix, +1).Set("GBPNZD" + postfix, +1);
		AddNet("JPY1").Set("EURJPY" + postfix, -1).Set("USDJPY" + postfix, -1).Set("GBPJPY" + postfix, -1);
		AddNet("JPY2").Set("EURJPY" + postfix, -1).Set("USDJPY" + postfix, -1).Set("GBPJPY" + postfix, -1).Set("CHFJPY" + postfix, -1).Set("AUDJPY" + postfix, -1).Set("NZDJPY" + postfix, -1).Set("CADJPY" + postfix, -1);
		AddNet("CAD1").Set("USDCAD" + postfix, -1).Set("EURCAD" + postfix, -1).Set("GBPCAD" + postfix, -1);
		AddNet("CAD2").Set("USDCAD" + postfix, -1).Set("EURCAD" + postfix, -1).Set("GBPCAD" + postfix, -1).Set("CADCHF" + postfix, +1).Set("CADJPY" + postfix, +1).Set("NZDCAD" + postfix, -1).Set("AUDCAD" + postfix, -1);
		AddNet("AUD1").Set("EURAUD" + postfix, -1).Set("AUDUSD" + postfix, +1).Set("GBPAUD" + postfix, -1);
		AddNet("AUD2").Set("EURAUD" + postfix, -1).Set("AUDUSD" + postfix, +1).Set("GBPAUD" + postfix, -1).Set("AUDCHF" + postfix, +1).Set("AUDJPY" + postfix, +1).Set("AUDNZD" + postfix, +1).Set("AUDCAD" + postfix, +1);
		AddNet("NZD1").Set("NZDUSD" + postfix, +1).Set("EURNZD" + postfix, -1).Set("GBPNZD" + postfix, -1);
		AddNet("NZD2").Set("NZDUSD" + postfix, +1).Set("EURNZD" + postfix, -1).Set("GBPNZD" + postfix, -1).Set("NZDJPY" + postfix, +1).Set("NZDCAD" + postfix, +1).Set("AUDNZD" + postfix, -1);
		AddNet("CHF1").Set("USDCHF" + postfix, -1).Set("EURCHF" + postfix, -1).Set("GBPCHF" + postfix, -1);
		AddNet("CHF2").Set("USDCHF" + postfix, -1).Set("EURCHF" + postfix, -1).Set("GBPCHF" + postfix, -1).Set("CHFJPY" + postfix, +1).Set("CADCHF" + postfix, -1).Set("AUDCHF" + postfix, -1);

		for(int i = 0; i < nets.GetCount(); i++) {
			NetSetting& net = nets[i];
			for(int j = 0; j < net.symbols.GetCount(); j++)
				net.symbol_ids.Add(FindSymbol(net.symbols.GetKey(j)), net.symbols[j]);
		}
		
		
		// Find variants
		variants.SetCount(symbols.GetCount());
		for(int i = 0; i < mt.GetSymbolCount(); i++) {
			const Symbol& s = mt.GetSymbol(i);
			if (!s.IsForex()) continue;
			
			VariantList& vl = variants[i];
			
			String a = s.name.Left(3);
			String b = s.name.Mid(3,3);
			for(int j = 0; j < currencies.GetCount(); j++) {
				const String& cur = currencies[j];
				if (cur == a || cur == b) continue;
				
				pair1[0] = a + cur;
				pair1[1] = a + cur;
				pair1[2] = cur + a;
				pair1[3] = cur + a;
				
				pair2[0] = b + cur;
				pair2[1] = cur + b;
				pair2[2] = b + cur;
				pair2[3] = cur + b;
				
				for(int k = 0; k < 4; k++) {
					int p1 = symbols.Find(pair1[k]);
					int p2 = symbols.Find(pair2[k]);
					if (p1 != -1 && p2 != -1) {
						VariantSymbol& vs = vl.symbols.Add();
						vs.math = k;
						vs.pair1 = pair1[k];
						vs.pair2 = pair2[k];
						vs.p1 = p1;
						vs.p2 = p2;
						
						vl.dependencies.FindAdd(p1);
						vl.dependencies.FindAdd(p2);
					}
				}
			}
		}
		
		
		
		// Add periods
		ASSERT(mt.GetTimeframe(0) == 1);
		#ifdef flagSECONDS
		AddPeriod("S1", 1);
		AddPeriod("S15", 15);
		for(int i = 0; i < mt.GetTimeframeCount(); i++)
			AddPeriod(mt.GetTimeframeString(i), mt.GetTimeframe(i) * 60);
		#else
		for(int i = 0; i < mt.GetTimeframeCount(); i++)
			AddPeriod(mt.GetTimeframeString(i), mt.GetTimeframe(i));
		AddPeriod("H12", 12*60);
		for(int i = 0; i < 5; i++)
			AddPeriod("D1." + IntStr(i+2), 1);
		#endif
		
		
		
		int sym_count = symbols.GetCount();
		int tf_count = periods.GetCount();
	
		if (sym_count == 0) throw DataExc();
		if (tf_count == 0)  throw DataExc();
	}
	catch (UserExc e) {
		throw e;
	}
	catch (Exc e) {
		throw e;
	}
	catch (...) {
		ASSERTUSER_(false, "Unknown error with MT4 connection.");
	}
	
	spread_points.SetCount(symbols.GetCount(), 0);
		
	for(int i = 0; i < symbols.GetCount(); i++) {
		if (i < mt.GetSymbolCount())
			spread_points[i] = mt.GetSymbol(i).point * 3;
		else
			spread_points[i] = 0.0003;
	}
	
}

void System::Deinit() {
	StopJobs();
	
	for(int i = 0; i < CommonFactories().GetCount(); i++) {
		RLOG("Overlook::RefreshCommon start " << i);
		CommonFactories()[i].b()->Deinit();
	}
}

void System::AddPeriod(String nice_str, int period) {
	int count = periods.GetCount();
	
	if (count == 0 && period != 1)
		throw DataExc();
	
	period_strings.Add(nice_str);
	periods.Add(period);
}

void System::AddSymbol(String sym) {
	ASSERT(symbols.Find(sym) == -1); // no duplicates
	symbols.Add(sym);
	signals.Add(0);
	sym_currencies.Add();
}

void System::AddCustomCore(const String& name, CoreFactoryPtr f, CoreFactoryPtr singlef) {
	CoreFactories().Add(CoreSystem(name, f, singlef));
}

void System::AddCustomScriptCore(const String& name, ScriptCoreFactoryPtr f, ScriptCoreFactoryPtr singlef) {
	ScriptCoreFactories().Add(ScriptCoreSystem(name, f, singlef));
}

}
