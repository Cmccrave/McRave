#pragma once
#include <BWAPI.h>
#include "Singleton.h"

using namespace BWAPI;
using namespace std;

#pragma warning(disable : 4351)

namespace McRave
{
	class BaseInfo;
	class GridManager
	{
		bool resetGrid[1024][1024] ={};
		int timeGrid[1024][1024] ={};
		int visibleGrid[256][256] ={};
		vector<WalkPosition> resetVector;

		int currentFrame = 0;

		// Ally Grid
		double parentDistance[1024][1024];
		double aGroundCluster[1024][1024] ={};
		double aAirCluster[1024][1024] ={};
		int defense[256][256] ={};

		// Enemy Grid
		double eGroundThreat[1024][1024] ={};
		double eAirThreat[1024][1024] ={};
		double eGroundCluster[1024][1024] ={};
		double eAirCluster[1024][1024] ={};
		int eSplash[1024][1024] ={};

		// Mobility Grid
		int mobility[1024][1024] ={};
		int collision[1024][1024] ={};
		double distanceHome[1024][1024] ={};

		Position armyCenter;
		void saveReset(WalkPosition);
		void updateDistance(), updateMobility(), updateAlly(), updateEnemy(), updateNeutral(), updateVisibility(), reset(), draw();

		void addThreat(UnitInfo&);
		void addCluster(UnitInfo&);
		void addSplash(UnitInfo&);
	public:

		// Update functions
		void onFrame(), onStart();

		void updateAllyMovement(Unit, WalkPosition);
		Position getArmyCenter() { return armyCenter; }

		// Defense grid
		void updateDefense(UnitInfo&);
		int getDefense(TilePosition here) { return defense[here.x][here.y]; }

		//double getAGroundCluster(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? aGroundCluster[here.x][here.y] : 0.0); }
		//double getAAirCluster(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? aAirCluster[here.x][here.y] : 0.0); }
		//double getEGroundThreat(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? eGroundThreat[here.x][here.y] : 0.0); }
		//double getEAirThreat(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? eAirThreat[here.x][here.y] : 0.0); }
		//double getEGroundCluster(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? eGroundCluster[here.x][here.y] : 0.0); }
		//double getEAirCluster(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? eAirCluster[here.x][here.y] : 0.0); }
		//int getCollision(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? collision[here.x][here.y] : 0); }
		//int getESplash(WalkPosition here) { return (Broodwar->getFrameCount() == timeGrid[here.x][here.y] ? eSplash[here.x][here.y] : 0); }

		double getAGroundCluster(WalkPosition here) { return aGroundCluster[here.x][here.y]; }
		double getAAirCluster(WalkPosition here) { return aAirCluster[here.x][here.y]; }
		double getEGroundThreat(WalkPosition here) { return eGroundThreat[here.x][here.y]; }
		double getEAirThreat(WalkPosition here) { return eAirThreat[here.x][here.y]; }
		double getEGroundCluster(WalkPosition here) { return eGroundCluster[here.x][here.y]; }
		double getEAirCluster(WalkPosition here) { return eAirCluster[here.x][here.y]; }
		int getCollision(WalkPosition here) { return collision[here.x][here.y]; }
		int getESplash(WalkPosition here) { return eSplash[here.x][here.y]; }

		int getMobility(WalkPosition here) { return mobility[here.x][here.y]; }
		double getDistanceHome(WalkPosition here) { return distanceHome[here.x][here.y]; }

		int lastVisibleFrame(TilePosition t) { return visibleGrid[t.x][t.y]; }



	private:

		template<class T>
		void addCollision(T& unit) {

			if (unit.getType().isFlyer())
				return;

			// Setup parameters
			int walkWidth = unit.getType().isBuilding() ? unit.getType().tileWidth() * 4 : (int)ceil(unit.getType().width() / 8.0) + 1;
			int walkHeight = unit.getType().isBuilding() ? unit.getType().tileHeight() * 4 : (int)ceil(unit.getType().height() / 8.0) + 1;
			//int frame = Broodwar->getFrameCount();

			// Iterate tiles and add to grid
			WalkPosition start(Util().getWalkPosition(unit.unit()));
			for (int x = start.x; x < start.x + walkWidth; x++) {
				for (int y = start.y; y < start.y + walkHeight; y++) {
					WalkPosition w(x, y);
					if (!w.isValid())
						continue;
					
					collision[x][y] = 1;
					saveReset(w);
				}
			}
		}
	};

}

typedef Singleton<McRave::GridManager> GridSingleton;