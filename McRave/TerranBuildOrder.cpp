#include "McRave.h"

void BuildOrderTrackerClass::terranOpener()
{
	if (getOpening)
	{
		if (currentBuild == "T2Fact") T2Fact();
		if (currentBuild == "TSparks") TSparks();
	}
	return;
}

void BuildOrderTrackerClass::terranTech()
{
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && getOpening)
		getOpening = false;
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= 2 && getOpening)
		getOpening = false;
}

void BuildOrderTrackerClass::terranSituational()
{
	productionSat = (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= min(12, (2 * Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Command_Center))));
	techSat = true;

	if (!bioBuild && getOpening && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Marine) >= 4)	
		unlockedType.erase(UnitTypes::Terran_Marine);
	else unlockedType.insert(UnitTypes::Terran_Marine);

	if (!BuildOrder().isBioBuild())
	{
		unlockedType.erase(UnitTypes::Terran_Medic);
		unlockedType.erase(UnitTypes::Terran_Firebat);
	}
	else
	{
		unlockedType.insert(UnitTypes::Terran_Medic);
		unlockedType.insert(UnitTypes::Terran_Firebat);
	}

	unlockedType.insert(UnitTypes::Terran_Vulture);
	unlockedType.insert(UnitTypes::Terran_Siege_Tank_Tank_Mode);
	unlockedType.insert(UnitTypes::Terran_Goliath);

	// Supply Depot logic
	buildingDesired[UnitTypes::Terran_Supply_Depot] = min(22, (int)floor((Units().getSupply() / max(14, (16 - Broodwar->self()->allUnitCount(UnitTypes::Terran_Supply_Depot))))));

	// Expansion logic
	if (shouldExpand())
	{
		buildingDesired[UnitTypes::Terran_Command_Center] = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) + 1;
	}

	// Bunker logic
	if (Strategy().isRush())
	{
		buildingDesired[UnitTypes::Terran_Bunker] = 1;
	}

	// Refinery logic
	if (!Strategy().isPlayPassive() && Resources().isGasSaturated() && Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) == buildingDesired[UnitTypes::Terran_Command_Center] && Broodwar->self()->gas() < Broodwar->self()->minerals() * 5 && Broodwar->self()->minerals() > 100)
	{
		buildingDesired[UnitTypes::Terran_Refinery] = Resources().getTempGasCount();
	}

	// Armory logic - TODO find a better solution to this garbage
	if (Strategy().getUnitScore()[UnitTypes::Terran_Goliath] > 1.0)
	{
		buildingDesired[UnitTypes::Terran_Armory] = 1;
	}

	// Academy logic
	if (Strategy().needDetection())
	{
		buildingDesired[UnitTypes::Terran_Academy] = 1;
	}



	// Barracks logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Barracks) >= 3 && (Production().getIdleProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200) || (!productionSat && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Barracks] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 3, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Barracks) + 1);
	}

	// Factory logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 2 && (Production().getIdleProduction().size() == 0 && ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings().getQueuedMineral() > 200 && Broodwar->self()->gas() - Production().getReservedGas() - Buildings().getQueuedGas() > 100) || (!productionSat && Resources().isMinSaturated()))))
	{
		buildingDesired[UnitTypes::Terran_Factory] = min(Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center) * 2, Broodwar->self()->visibleUnitCount(UnitTypes::Terran_Factory) + 1);
	}

	// Machine Shop logic
	if (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) >= 1)
	{
		buildingDesired[UnitTypes::Terran_Machine_Shop] = Broodwar->self()->completedUnitCount(UnitTypes::Terran_Factory) - (Broodwar->self()->completedUnitCount(UnitTypes::Terran_Command_Center));
	}
}