#include "McRave.h"

namespace McRave
{
	void InterfaceManager::onFrame()
	{
		drawInformation();
		drawAllyInfo();
		drawEnemyInfo();
		dingEffect();
		return;
	}

	void InterfaceManager::performanceTest(string function)
	{
		double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
		myTest[function] = myTest[function] * 0.99 + dur * 0.01;
		if (myTest[function] > 0.2) {
			Broodwar->drawTextScreen(180, screenOffset, "%c%s", Text::White, function);
			Broodwar->drawTextScreen(372, screenOffset, "%c%.2f ms", Text::White, myTest[function]);
			screenOffset += 10;
		}
		return;
	}

	void InterfaceManager::startClock()
	{
		start = chrono::high_resolution_clock::now();
		return;
	}

	void InterfaceManager::drawInformation()
	{
		// Reset the screenOffset for the performance tests
		screenOffset = 0;

		//WalkPosition mouse(Broodwar->getMousePosition() + Broodwar->getScreenPosition());
		//Broodwar->drawTextScreen(Broodwar->getMousePosition() - Position(0, 16), "%d", mapBWEM.GetMiniTile(mouse).Altitude());

		if (builds) {
			int offset = 0;
			int time = Broodwar->getFrameCount() / 24;

			if (Broodwar->self()->getColor() == 156)
				color = 156, textColor = 17;
			else {
				color = Broodwar->self()->getColor();
				textColor = Broodwar->self()->getTextColor();
			}

			int seconds = time % 60;
			int minute = time / 60;

			// Display what build is being used
			if (BuildOrder().getCurrentOpener() != "")
				Broodwar->drawTextScreen(432, 16, "%c%s: %c%s %s", Text::White, BuildOrder().getCurrentBuild().c_str(), Text::Grey, BuildOrder().getCurrentOpener().c_str(), BuildOrder().getCurrentTransition().c_str());
			else
				Broodwar->drawTextScreen(432, 16, "%c%s vs %s", Text::White, BuildOrder().getCurrentBuild().c_str(), Strategy().getEnemyBuild().c_str());

			Broodwar->drawTextScreen(432, 28, "%c%d", Text::Grey, Broodwar->getFrameCount());
			Broodwar->drawTextScreen(482, 28, "%c%d:%02d", Text::Grey, minute, seconds);

			// Display unit scoring
			for (auto &unit : Strategy().getUnitScores()) {
				if (unit.first.isValid() && unit.second > 0.0) {
					Broodwar->drawTextScreen(0, offset, "%c%s: %c%.2f", textColor, unit.first.c_str(), Text::White, unit.second);
					offset += 10;
				}
			}

			// Display remaining minerals on each mineral
			for (auto &r : Resources().getMyMinerals()) {
				Broodwar->drawTextMap(r.second.getPosition() + Position(-8, 8), "%c%d", Text::White, r.second.getRemainingResources());
			}

			// Display remaining gas on each geyser
			for (auto &r : Resources().getMyGas()) {
				Broodwar->drawTextMap(r.second.getPosition() + Position(-8, 32), "%c%d", Text::Green, r.second.getRemainingResources());
			}
		}

		if (bweb) {
			mapBWEB.draw();
		}
	}

	void InterfaceManager::drawAllyInfo()
	{
		if (targets) {
			for (auto &u : Units().getMyUnits()) {
				UnitInfo &unit = u.second;

				if (unit.hasTarget())
					Broodwar->drawLineMap(unit.getTarget().getPosition(), unit.getPosition(), unit.getPlayer()->getColor());

				if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0) {
					//Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f", Text::Brown, unit.getVisibleGroundStrength());
					//Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f", Text::Blue, unit.getVisibleAirStrength());
				}
			}
		}
		return;
	}

	void InterfaceManager::drawEnemyInfo()
	{
		if (targets) {
			for (auto &u : Units().getEnemyUnits()) {
				UnitInfo &unit = u.second;

				if (unit.hasTarget())
					Broodwar->drawLineMap(unit.getTarget().getPosition(), unit.getPosition(), unit.getPlayer()->getColor());

				if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0) {
					Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f", Text::Brown, unit.getVisibleGroundStrength());
					Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f", Text::Blue, unit.getVisibleAirStrength());
				}
			}
		}
	}

	void InterfaceManager::onSendText(string text)
	{
		if (text == "/targets")
			targets = !targets;
		else if (text == "/builds")
			builds = !builds;
		else if (text == "/bweb")
			bweb = !bweb;
		else if (text == "/paths")
			paths = !paths;
		else 
			Broodwar->sendText("%s", text.c_str());
		return;
	}

	void InterfaceManager::dingEffect()
	{
		for (auto &u : dings) {
			Unit unit = u.second;

			if (!unit || !unit->exists())
				continue;

			if (Broodwar->getFrameCount() - u.first > 100)
				continue;

			int diff = (100 - (Broodwar->getFrameCount() - u.first)) / 50;
			if (diff == 0)
				continue;

			int width = unit->getType().width() / diff;
			int height = unit->getType().height() / diff;

			Broodwar->drawEllipseMap(unit->getPosition(), width, height, color);
			Broodwar->drawTextMap(unit->getPosition() + Position(-11, -24), "%cDING!", textColor);
		}
	}

	void InterfaceManager::displayPath(UnitInfo& unit, vector<TilePosition> path)
	{
		if (path.empty())
			return;

		TilePosition next = TilePositions::Invalid;
		for (auto &tile : path) {

			if (tile == *(path.begin()))
				continue;

			if (next.isValid() && tile.isValid())
				Broodwar->drawLineMap(Position(next) + Position(16, 16), Position(tile) + Position(16, 16), color);

			next = tile;
		}
	}

	void InterfaceManager::displaySim(UnitInfo& unit, double simValue)
	{
		if (!sim || simValue == DBL_MAX)
			return;

		if (unit.getLocalState() == LocalState::Engaging)
			Broodwar->drawTextMap(unit.getPosition() + Position(-9, 16), "%c%.2f", textColor, simValue);
		else
			Broodwar->drawTextMap(unit.getPosition() + Position(-9, 16), "%c%.2f", Text::Grey, simValue);
	}
}