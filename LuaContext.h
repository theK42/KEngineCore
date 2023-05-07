#pragma once
#include "LuaLibrary.h"
#include "Lua.hpp"
#include <vector>
#include <string>


namespace KEngineCore {

	class LuaScheduler;
	class ScheduledLuaThread;

	class LuaContext
	{
	public:
		LuaContext();
		~LuaContext();

		void Init(LuaScheduler *scheduler, LuaContext * parent);
		void Deinit();

		void RunScript(const std::string_view&, ScheduledLuaThread * thread);
		void AddContextualObject(const char* name, void* thingy);
		void PushToLua(lua_State* luaState);
		LuaScheduler* GetLuaScheduler() const;

		static LuaContext* GetFromState(lua_State* luaState, int stackIndex); 

		static const char MetaName[];

	private:
		LuaScheduler*						mScheduler{ nullptr };
		std::vector<ScheduledLuaThread* >	mRunningThreads;
	};

	class ContextLibrary : protected LuaLibraryTwo<LuaContext> {
	public:
		ContextLibrary();
		~ContextLibrary();
		void Init(lua_State* luaState);
		void Deinit();
	};


	/*void Pseudocode()
	{
		ScriptRunner app;

		LuaScheduler scheduler;
		Timer time;
		
		app.Init(&scheduler, nullptr);
		app.AddContextualObject("time", &time);

		app.RunScript("KRBSG.lua");

	}*/

/*  

General idiom:  start a cancelable process, wait for an event that might cancel the process.  If process could have multiple ends (player death, player success) wiat for an outcome of the process.

+++++++++
KRBSG.lua
+++++++++
	
	local input = require "input"
	local scheduler = require "scheduler"
	local krbsg = require "krbsg"



	local attractor = krbsg.createAttractMode(...);

	while (true) do

		attractor:start("attract.lua");

		input.waitForAnyButton(...);

		attractor:stop();

		local gameSession = krbsg.createGameSession("game.lua", ...);
			
		local paused = gameSession:run("game.lua"); 
		
		while (paused == true) do
			local resume = krbsg.showPauseMenu(...);
			if (resume)
				paused = gameSession:resume();
			else
				paused = false;
				gameSession.stop();
				gameSession = nil;
			end
		end	
	end


+++++++++++
attract.lua
+++++++++++
	local time = require "time"
	local krbsg = require "krbsg"

	while (true) do
		time.wait(5, ...);
		local gameSession = krbsg.createGameSession("attractGame.lua", ...);
		gameSession.run();
	end


++++++++
game.lua
++++++++

	local time = require "time"
	local krbsg = require "krbsg"

	local width = 800; -- TODO read as parameter?  Change entirely?
	local height = 600;
	local starfield = krbsg.spawnStarfield(width, height, ...);
	local position = {x = 400, y = 300};
	local playerShip = krbsg.spawnPlayerShip("basic", position, ...);

	position.y = 0;

	while true do
		timer.waits(1, ...);
		position.x = math.random(0, width);
		local enemyShip = krbsg.spawnEnemyShip("basic", position, ...);
	end

+++++++++++++++
attractgame.lua
+++++++++++++++

	local time = require "time"
	local krbsg = require "krbsg"

	local width = 800; -- TODO read as parameter?  Change entirely?
	local height = 600;
	local starfield = krbsg.spawnStarfield(width, height, ...);
	local position = {x = 400, y = 300};
	local playerShip = krbsg.spawnAttractPlayerShip("basic", position, ...);

	position.y = 0;

	while true do
		timer.waits(1);
		position.x = math.random(0, width);
		local enemyShip = krbsg.spawnEnemyShip("basic", position);
	end


	
*/


}
