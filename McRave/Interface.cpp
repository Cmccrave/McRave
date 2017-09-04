#include "McRave.h"

void InterfaceTrackerClass::update()
{
	drawInformation();
	drawAllyInfo();
	drawEnemyInfo();
	return;
}

void InterfaceTrackerClass::performanceTest(string function)
{
	double dur = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
	myTest[function] = myTest[function] * 0.99 + dur*0.01;
	if (myTest[function] > 1.00)
	{
		Broodwar->drawTextScreen(200, screenOffset, "%c%s", Text::White, function);
		Broodwar->drawTextScreen(350, screenOffset, "%c%.2f ms", Text::White, myTest[function]);
		screenOffset += 10;
	}
	return;
}

void InterfaceTrackerClass::startClock()
{
	start = chrono::high_resolution_clock::now();
	return;
}

void InterfaceTrackerClass::drawInformation()
{
	int offset = 0;
	screenOffset = 0;
	
	// Display global strength calculations	
	Broodwar->drawTextScreen(575, 16, "%c%.2f/%.2f", Text::White, Units().getGlobalAllyStrength(), Units().getGlobalEnemyStrength());
	Broodwar->drawTextScreen(452, 16, "%c%d", Text::White, Production().getReservedMineral() + Buildings().getQueuedMineral());
	Broodwar->drawTextScreen(520, 16, "%c%d", Text::White, Production().getReservedGas() + Buildings().getQueuedGas());
	Broodwar->drawTextScreen(520, 32, "Detection: %d", Strategy().needDetection());

	// Display unit scoring	
	for (auto &unit : Strategy().getUnitScore())
	{
		if (unit.second > 0.0)
		{
			Broodwar->drawTextScreen(0, offset, "%c%s: %c%.2f", Text::Blue, unit.first.c_str(), Text::White, unit.second);
			offset += 10;
		}
	}

	// Display remaining minerals on each mineral patch that is near our Nexus
	for (auto &r : Resources().getMyMinerals())
	{
		Broodwar->drawTextMap(r.second.getPosition() + Position(-8, 8), "%c%d", Text::White, r.second.getRemainingResources());
	}

	// Display remaining gas on each geyser that is near our Nexus
	for (auto &r : Resources().getMyGas())
	{
		Broodwar->drawTextMap(r.second.getPosition() + Position(-8, 32), "%c%d", Text::Green, r.second.getRemainingResources());
		Broodwar->drawTextMap(r.second.getPosition(), "%d", r.second.getGathererCount());
	}
	return;
}

void InterfaceTrackerClass::drawAllyInfo()
{
	if (debugging)
	{
		for (auto &u : Units().getAllyUnits())
		{
			UnitInfo unit = u.second;
			if (unit.getDeadFrame() == 0)
			{
				Broodwar->drawLineMap(unit.getTargetPosition(), unit.getPosition(), Broodwar->self()->getColor());
				if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0)
				{
					Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f, %.2f", Text::Brown, unit.getVisibleGroundStrength(), unit.getGroundLocal());
					Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f, %.2f", Text::Blue, unit.getVisibleAirStrength(), unit.getAirLocal());
				}
			}
		}
	}
	return;
}

void InterfaceTrackerClass::drawEnemyInfo()
{
	if (debugging)
	{
		for (auto &u : Units().getEnemyUnits())
		{
			UnitInfo unit = u.second;
			if (unit.getDeadFrame() == 0)
			{
				if (unit.getVisibleGroundStrength() > 0.0 || unit.getVisibleAirStrength() > 0.0)
				{
					Broodwar->drawTextMap(unit.getPosition() + Position(5, -10), "Grd: %c %.2f", Text::Brown, unit.getVisibleGroundStrength());
					Broodwar->drawTextMap(unit.getPosition() + Position(5, 2), "Air: %c %.2f", Text::Blue, unit.getVisibleAirStrength());
				}
			}
		}
	}
}

void InterfaceTrackerClass::sendText(string text)
{
	if (text == "/debug")
	{
		debugging = !debugging;
	}
	else
	{
		Broodwar->sendText("%s", text.c_str());
	}
	return;
}
