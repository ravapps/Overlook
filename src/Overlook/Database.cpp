#include "Overlook.h"

namespace Overlook {

UserDatabase& GetDatabase(int user_id) {
	static ArrayMap<int, UserDatabase> dbs;
	static SpinLock lock;
	lock.Enter();
	UserDatabase& db = dbs.GetAdd(user_id);
	if (!db.IsOpen())
		db.Init(user_id);
	lock.Leave();
	return db;
}






UserDatabase::UserDatabase() {
	
}

int UserDatabase::Init(int user_id) {
	lock.Enter();
	this->user_id = user_id;
	
	String user_dir = GetOverlookFile("users");
	RealizeDirectory(user_dir);
	
	user_file = AppendFileName(user_dir, IntStr(user_id) + ".bin");
	user_loc_file = AppendFileName(user_dir, IntStr(user_id) + "_loc.bin");
	
	if (FileExists(user_file))
		LoadFromFile(*this, user_file);
	
	if (!location.Open(user_loc_file)) {
		lock.Leave();
		return 1;
	}
	
	open = true;
	lock.Leave();
	return 0;
}

void UserDatabase::Deinit() {
	Flush();
	lock.Enter();
	location.Close();
	open = false;
	lock.Leave();
}

void UserDatabase::Flush() {
	lock.Enter();
	StoreToFile(*this, user_file);
	lock.Leave();
}








ServerDatabase::ServerDatabase() {
	
}

void ServerDatabase::Init() {
	srv_file = GetOverlookFile("server_db.bin");
	
	LoadFromFile(*this, srv_file);
}


void ServerDatabase::Flush() {
	StoreToFile(*this, srv_file);
}


}
