#include "McRave.h"
#include "BuildOrder.h"
#include <fstream>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder
{
    namespace {
        void updateBuild()
        {
            // TODO: Check if we own a <race> unit - have a build order allowed PER race for FFA weirdness and maybe mind control shenanigans
            if (Broodwar->self()->getRace() == Races::Protoss) {
                Protoss::opener();
                Protoss::tech();
                Protoss::situational();
                Protoss::unlocks();
            }
            if (Broodwar->self()->getRace() == Races::Terran) {
            	Terran::opener();
            	Terran::tech();
            	Terran::situational();
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                Zerg::opener();
                Zerg::tech();
                Zerg::situational();
                Zerg::unlocks();
            }
        }

        bool isBuildPossible(string build, string opener)
        {
            vector<UnitType> buildings, defenses;
            auto wallOptional = false;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "2Gate" && opener == "Natural") {
                    buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Pylon };
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                }
                else if (build == "GateNexus" || build == "NexusGate") {
                    int count = Util::chokeWidth(BWEB::Map::getNaturalChoke()) / 64;
                    buildings.insert(buildings.end(), count, Protoss_Pylon);
                }
                else {
                    buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                    defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
                    wallOptional = true;
                }
            }

            if (Broodwar->self()->getRace() == Races::Terran)
                buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };

            if (Broodwar->self()->getRace() == Races::Zerg) {
                buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
                defenses.insert(defenses.end(), 6, Zerg_Sunken_Colony);
            }

            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Terrain::findMainWall(buildings, defenses) || wallOptional)
                    return true;
            }
            else {
                if (Terrain::findNaturalWall(buildings, defenses) || wallOptional)
                    return true;
            }
            return false;
        }

        bool isBuildAllowed(Race enemy, string build)
        {
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore")
                    return true;
                if (build == "2Gate")
                    return true;
                if (build == "NexusGate" || build == "GateNexus")
                    return t;
                if (build == "FFE")
                    return z;
            }

            if (Broodwar->self()->getRace() == Races::Terran && build != "") {
                return true; // For now, test all builds to make sure they work!
            }

            if (Broodwar->self()->getRace() == Races::Zerg && build != "") {
                if (build == "PoolLair")
                    return z;
                if (build == "HatchPool")
                    return t || p;
            }
            return false;
        }

        bool isOpenerAllowed(Race enemy, string build, string opener)
        {
            // Ban certain openers from certain race matchups
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore") {
                    if (opener == "0Zealot")
                        return t;
                    if (opener == "1Zealot")
                        return true;
                    if (opener == "2Zealot")
                        return p || z || r;
                }

                if (build == "2Gate") {
                    if (opener == "Proxy")
                        return false;
                    if (opener == "Natural")
                        return z || p;
                    if (opener == "Main")
                        return true;
                }

                if (build == "FFE") {
                    if (opener == "Gate" || opener == "Nexus" || opener == "Forge")
                        return z;
                }

                if (build == "NexusGate") {
                    if (opener == "Dragoon" || opener == "Zealot")
                        return /*p ||*/ t;
                }
                if (build == "GateNexus") {
                    if (opener == "1Gate" || opener == "2Gate")
                        return t;
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (build == "PoolLair")
                    if (opener == "9Pool")
                        return z;
                if (build == "HatchPool")
                    if (opener == "12Hatch")
                        return t || p;
            }
            return false;
        }

        bool isTransitionAllowed(Race enemy, string build, string transition)
        {
            // Ban certain transitions from certain race matchups
            auto p = enemy == Races::Protoss;
            auto z = enemy == Races::Zerg;
            auto t = enemy == Races::Terran;
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore") {
                    if (transition == "DT")
                        return p || t || z;
                    if (transition == "3GateRobo")
                        return p || r;
                    if (transition == "Corsair")
                        return z;
                    if (transition == "Reaver")
                        return p /*|| t*/ || r;
                    if (transition == "4Gate")
                        return p || t;
                }

                if (build == "2Gate") {
                    if (transition == "DT")
                        return /*p ||*/t;
                    if (transition == "Reaver")
                        return p /*|| t*/ || r;
                    if (transition == "Expand")
                        return p || z;
                    if (transition == "DoubleExpand")
                        return t;
                    if (transition == "4Gate")
                        return z;
                    //if (transition == "ZealotRush")
                    //	return true;
                }

                if (build == "FFE") {
                    if (transition == "NeoBisu" || transition == "2Stargate" || transition == "StormRush")
                        return z;
                }

                if (build == "NexusGate") {
                    if (transition == "DoubleExpand"/* || transition == "ReaverCarrier"*/)
                        return t;
                    if (transition == "Standard")
                        return t || p;
                }

                if (build == "GateNexus") {
                    if (transition == "DoubleExpand" || transition == "Carrier" || transition == "Standard")
                        return t;
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (build == "PoolLair")
                    if (transition == "1HatchMuta")
                        return z;
				if (build == "HatchPool") {
					if (transition == "2HatchMuta")
						return t || p;
					if (transition == "2HatchHydra")
						return p;
					if (transition == "3HatchLing")
						return t;
				}
            }
            return false;
        }
    }

    void onEnd(bool isWinner)
    {
        // File extension including our race initial;
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string enemyRaceChar{ *Broodwar->enemy()->getRace().c_str() };
        const string extension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + Broodwar->mapFileName() + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";

        // Write into the write directory 3 tokens at a time (4 if we detect a dash)
        ofstream config("bwapi-data/write/" + extension);
        string token;
        bool correctBuild = false;
        bool correctOpener = false;
        bool correctTransition = false;

        const auto shouldIncrement = [&](string name) {

            // Check if we're looking at the correct build to prevent incrementing an opener for a different build
            if (correctBuild) {

                // Check if we're looking at the correct opener to prevent incrementing a transition for a different build
                if (correctOpener) {

                    // Check if we're looking for a transition string
                    if (!correctTransition && name == currentTransition)
                        return true;
                }
                if (!correctOpener && name == currentOpener) {
                    correctOpener = true;
                    return true;
                }
            }
            if (!correctBuild && name == currentBuild) {
                correctBuild = true;
                return true;
            }
            return false;
        };

        int totalWins, totalLoses;
        ss >> totalWins >> totalLoses;
        isWinner ? totalWins++ : totalLoses++;
        config << totalWins << " " << totalLoses << endl;

        // For each token, check if we should increment the wins or losses then shove it into the config file
        while (ss >> token) {
            if (token == dash) {
                int w, l;

                ss >> token >> w >> l;
                shouldIncrement(token) ? (isWinner ? w++ : l++) : 0;
                config << dash << endl;
                config << token << " " << w << " " << l << endl;
            }
            else {
                int w, l;

                ss >> w >> l;
                shouldIncrement(token) ? (isWinner ? w++ : l++) : 0;
                config << token << " " << w << " " << l << endl;
            }
        }
    }

    void onStart()
    {
        if (!Broodwar->enemy()) {
            getDefaultBuild();
            return;
        }

        bool testing = false;
        if (testing) {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                currentBuild = "2Gate";
                currentOpener = "Natural";
                currentTransition = "DT";
                isBuildPossible(currentBuild, currentOpener);
                return;
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                currentBuild = "HatchPool";
                currentOpener = "12Hatch";
                currentTransition = "3HatchLing";
                isBuildPossible(currentBuild, currentOpener);
                return;
            }
        }

        // File extension including our race initial;
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string enemyRaceChar{ *Broodwar->enemy()->getRace().c_str() };
        const string extension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + Broodwar->mapFileName() + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";

        // Tokens
        string buffer, token;

        // Protoss builds, openers and transitions
        if (Broodwar->self()->getRace() == Races::Protoss) {

            myBuilds["1GateCore"].openers ={ "0Zealot", "1Zealot", "2Zealot" };
            myBuilds["1GateCore"].transitions ={ "3GateRobo", "Reaver", "Corsair", "4Gate", "DT" };

            myBuilds["2Gate"].openers ={ "Proxy", "Natural", "Main" };
            myBuilds["2Gate"].transitions ={ "ZealotRush", "DT", "Reaver", "Expand", "DoubleExpand", "4Gate" };

            myBuilds["FFE"].openers ={ "Gate", "Nexus", "Forge" };
            myBuilds["FFE"].transitions ={ "NeoBisu", "2Stargate", "StormRush" };

            myBuilds["NexusGate"].openers ={ "Dragoon", "Zealot" };
            myBuilds["NexusGate"].transitions ={ "Standard", "DoubleExpand", "ReaverCarrier" };

            myBuilds["GateNexus"].openers ={ "1Gate", "2Gate" };
            myBuilds["GateNexus"].transitions ={ "Standard", "DoubleExpand", "Carrier" };
        }

        if (Broodwar->self()->getRace() == Races::Zerg) {

            myBuilds["PoolHatch"].openers ={ "9Pool", "12Pool", "13Pool" };
            myBuilds["PoolHatch"].transitions={ "2HatchMuta", "2HatchHydra" };

            myBuilds["HatchPool"].openers ={ "9Hatch" , "10Hatch", "12Hatch" };
            myBuilds["HatchPool"].transitions={ "3HatchLing", "2HatchMuta", "2HatchHydra" };

            myBuilds["PoolLair"].openers ={ "9Pool" };
            myBuilds["PoolLair"].transitions={ "1HatchMuta", "1HatchLurker" };
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            currentBuild = "RaxFact";
            currentOpener = "10Rax";
            currentTransition = "2FactVulture";
            isBuildPossible(currentBuild, currentOpener);
            return;// Don't know what to do yet

/*
            myBuilds["RaxFact"].openers ={ "8Rax", "10Rax", "12Rax" };
            myBuilds["RaxFact"].transitions ={ "Joyo", "2FactVulture", "1FactExpand", "2PortWraith" };

            myBuilds["2Rax"].openers = { "}*/
        }

        // Winrate tracking
        auto bestBuildWR = -0.01;
        string bestBuild = "";

        auto bestOpenerWR = -0.01;
        string bestOpener = "";

        auto bestTransitionWR = -0.01;
        string bestTransition = "";

        const auto parseLearningFile = [&](ifstream &file) {
            int dashCnt = -1; // Start at -1 so we can index at 0
            while (file >> buffer)
                ss << buffer << " ";

            // Create a copy so we aren't dumping out the information
            stringstream sscopy;
            sscopy << ss.str();

            // Initialize			
            string thisBuild = "";
            string thisOpener = "";
            Race enemyRace = Broodwar->enemy()->getRace();
            int totalWins, totalLoses;

            // Store total games played to calculate UCB values
            sscopy >> totalWins >> totalLoses;
            int totalGamesPlayed = totalWins + totalLoses;

            while (!sscopy.eof()) {
                sscopy >> token;

                if (token == "-") {
                    dashCnt++;
                    dashCnt %= 3;
                    continue;
                }

                int w, l, numGames;
                sscopy >> w >> l;
                numGames = w + l;

                double winRate = numGames > 0 ? double(w) / double(numGames) : 0;
                double ucbVal = sqrt(2.0 * log((double)totalGamesPlayed) / numGames);
                double val = winRate + ucbVal;

                if (l == 0)
                    val = DBL_MAX;

                switch (dashCnt) {

                    // Builds
                case 0:
                    if (val > bestBuildWR && isBuildAllowed(enemyRace, token)) {
                        bestBuild = token;
                        bestBuildWR = val;

                        bestOpenerWR = -0.01;
                        bestOpener = "";

                        bestTransitionWR = -0.01;
                        bestTransition = "";
                    }
                    break;

                    // Openers
                case 1:
                    if (val > bestOpenerWR && isOpenerAllowed(enemyRace, bestBuild, token)) {
                        bestOpener = token;
                        bestOpenerWR = val;

                        bestTransitionWR = -0.01;
                        bestTransition = "";
                    }
                    break;

                    // Transitions
                case 2:
                    if (val > bestTransitionWR && isTransitionAllowed(enemyRace, bestBuild, token)) {
                        bestTransition = token;
                        bestTransitionWR = val;
                    }
                    break;
                }
            }
            return;
        };

        // Attempt to read a file from the read directory
        ifstream readFile("bwapi-data/read/" + extension);
        ifstream writeFile("bwapi-data/write/" + extension);
        if (readFile)
            parseLearningFile(readFile);

        // Try to read a file from the write directory
        else if (writeFile)
            parseLearningFile(writeFile);

        // If no file, create one
        else {
            if (!writeFile.good()) {
                ss << noStats;
                for (auto &b : myBuilds) {
                    auto &build = b.second;
                    ss << dash << " ";
                    ss << b.first << noStats;

                    // Load the openers for this build
                    ss << dash << " ";
                    for (auto &opener : build.openers)
                        ss << opener << noStats;


                    // Load the transitions for this build
                    ss << dash << " ";
                    for (auto &transition : build.transitions)
                        ss << transition << noStats;
                }
            }
        }

        // TODO: If the build is possible based on this map (we have wall/island requirements)
        if (isBuildPossible(bestBuild, bestOpener) && isBuildAllowed(Broodwar->enemy()->getRace(), bestBuild)) {
            currentBuild = bestBuild;
            currentOpener = bestOpener;
            currentTransition = bestTransition;
        }
        else
            getDefaultBuild();

    }

    bool techComplete()
    {
        if (techUnit == Protoss_Scout || techUnit == Protoss_Corsair || techUnit == Protoss_Observer || techUnit == Terran_Science_Vessel)
            return (Broodwar->self()->completedUnitCount(techUnit) > 0);
        if (techUnit == Protoss_High_Templar)
            return (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
        if (techUnit == Protoss_Arbiter)
            return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
        if (techUnit == Protoss_Dark_Templar)
            return (Broodwar->self()->completedUnitCount(techUnit) >= 2);
        if (techUnit == Protoss_Reaver)
            return Broodwar->self()->completedUnitCount(techUnit) > 0; //return (Broodwar->self()->completedUnitCount(Protoss_Shuttle) >= 1) || (Broodwar->self()->completedUnitCount(techUnit) > 0 && Terrain::isNarrowNatural());
        return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
    }

    void getDefaultBuild()
    {
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players::vP()) {
                currentBuild = "1GateCore";
                currentOpener = "1Zealot";
                currentTransition = "Reaver";
            }
            else if (Players::vZ()) {
                currentBuild = "1GateCore";
                currentOpener = "2Zealot";
                currentTransition = "Corsair";
            }
            else if (Players::vT()) {
                currentBuild = "GateNexus";
                currentOpener = "1Gate";
                currentTransition = "Standard";
                isBuildPossible(currentBuild, currentOpener);
            }
            else {
                currentBuild = "2Gate";
                currentOpener = "Main";
                currentTransition = "Reaver";
            }
        }
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players::vZ()) {
                currentBuild = "PoolLair";
                currentOpener = "9Pool";
                currentTransition = "1HatchMuta";
            }
            else {
                currentBuild = "HatchPool";
                currentOpener = "12Hatch";
                currentTransition = "2HatchMuta";
            }
            isBuildPossible(currentBuild, currentOpener);
        }
        return;
    }

    void onFrame()
    {
        Visuals::startPerfTest();
        updateBuild();
        Visuals::endPerfTest("BuildOrder");
    }

    bool shouldExpand()
    {
        UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Broodwar->self()->minerals() > 400 + (50 * com(baseType)))
                return true;
            else if (techUnit == None && !Production::hasIdleProduction() && Resources::isMinSaturated() && techSat && productionSat)
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Terran) {
            if (Broodwar->self()->minerals() > 400 + (100 * com(baseType)))
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->minerals() - Production::getReservedMineral() - Buildings::getQueuedMineral() >= 300)
                return true;
        }
        return false;
    }

    bool shouldAddProduction()
    {
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->minerals() >= 300 && vis(Zerg_Larva) < 3)
                return true;
        }
        else {
            if (!productionSat && Resources::isMinSaturated() && (!Production::hasIdleProduction() || Units::getSupply() >= 300 || Broodwar->self()->minerals() > 600))
                return true;
        }
        return false;
    }

    bool shouldAddGas()
    {
        auto workerCount = Broodwar->self()->completedUnitCount(Broodwar->self()->getRace().getWorker());
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Resources::isGasSaturated() && Broodwar->self()->minerals() - Production::getReservedMineral() - Buildings::getQueuedMineral() > 300)
                return true;
            if (vis(Zerg_Extractor) == 0)
                return true;
        }

        else if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players::vP())
                return Broodwar->self()->visibleUnitCount(Protoss_Assimilator) != 1 || workerCount >= 32 || Broodwar->self()->minerals() > 600;
            else if (Players::vT() || Players::vZ())
                return true;
        }

        else if (Broodwar->self()->getRace() == Races::Terran)
            return true;
        return false;
    }

    int buildCount(UnitType unit)
    {
        if (itemQueue.find(unit) != itemQueue.end())
            return itemQueue[unit].getActualCount();
        return 0;
    }

    bool firstReady()
    {
        if (firstTech != TechTypes::None && Broodwar->self()->hasResearched(firstTech))
            return true;
        else if (firstUpgrade != UpgradeTypes::None && Broodwar->self()->getUpgradeLevel(firstUpgrade) > 0)
            return true;
        else if (firstTech == TechTypes::None && firstUpgrade == UpgradeTypes::None)
            return true;
        return false;
    }

    void getNewTech()
    {
        if (!getTech)
            return;

        // Choose a tech based on highest unit score
        double highest = 0.0;
        for (auto &tech : Strategy::getUnitScores()) {
            if (tech.first == Protoss_Dragoon
                || tech.first == Protoss_Zealot
                || tech.first == Protoss_Shuttle)
                continue;

            if (tech.second > highest) {
                highest = tech.second;
                techUnit = tech.first;
            }
        }
    }

    void checkNewTech()
    {
        auto canGetTech = (Broodwar->self()->getRace() == Races::Protoss && com(Protoss_Cybernetics_Core) > 0)
            || (Broodwar->self()->getRace() == Races::Zerg && com(Zerg_Spawning_Pool) > 0);

        const auto alreadyUnlocked = [&](UnitType type) {
            if (unlockedType.find(type) != unlockedType.end() && techList.find(type) != techList.end())
                return true;
            return false;
        };

        // No longer need to choose a tech
        if (techUnit != None) {
            getTech = false;
            techList.insert(techUnit);
            unlockedType.insert(techUnit);
        }

        // Multi-unlock
        if (techUnit == Protoss_Arbiter || techUnit == Protoss_High_Templar) {
            unlockedType.insert(Protoss_Dark_Templar);
            techList.insert(Protoss_Dark_Templar);
        }
        else if (techUnit == Protoss_Reaver) {
            unlockedType.insert(Protoss_Shuttle);
            techList.insert(Protoss_Shuttle);
        }
        else if (techUnit == Zerg_Mutalisk && Players::getStrength(PlayerState::Enemy).airToAir > 0.0) {
            techList.insert(Zerg_Scourge);
            unlockedType.insert(Zerg_Scourge);
        }
        else if (techUnit == Zerg_Lurker) {
            techList.insert(Zerg_Hydralisk);
            unlockedType.insert(Zerg_Hydralisk);
        }

        // HACK: If we have a Reaver add Obs to the tech
        if (Broodwar->self()->completedUnitCount(Protoss_Reaver) >= 3) {
            techList.insert(Protoss_Observer);
            unlockedType.insert(Protoss_Observer);
        }
    }

    void checkAllTech()
    {
        bool moreToAdd;
        set<UnitType> toCheck;

        // For every unit in our tech list, ensure we are building the required buildings
        for (auto &type : techList) {
            toCheck.insert(type);
            toCheck.insert(type.whatBuilds().first);
        }

        // Iterate all required branches of buildings that are required for this tech unit
        do {
            moreToAdd = false;
            for (auto &check : toCheck) {
                for (auto &pair : check.requiredUnits()) {
                    UnitType type(pair.first);
                    if (Broodwar->self()->completedUnitCount(type) == 0 && toCheck.find(type) == toCheck.end()) {
                        toCheck.insert(type);
                        moreToAdd = true;
                    }
                }
            }
        } while (moreToAdd);

        // For each building we need to check, add to our queue whatever is possible to build based on its required branch
        int i = 0;
        for (auto &check : toCheck) {

            if (!check.isBuilding())
                continue;

            i += 10;

            bool canAdd = true;
            for (auto &pair : check.requiredUnits()) {
                UnitType type(pair.first);
                if (type.isBuilding() && Broodwar->self()->completedUnitCount(type) == 0) {
                    canAdd = false;
                }
            }

            // HACK: Our check doesn't look for required buildings for tech needed for Lurkers
            if (check == Zerg_Lurker)
                itemQueue[Zerg_Lair] = Item(1);

            // Add extra production - TODO: move to shouldAddProduction
            int s = Units::getSupply();
            if (canAdd && buildCount(check) <= 1) {
                if (check == Protoss_Stargate) {
                    if ((s >= 150 && techList.find(Protoss_Corsair) != techList.end())
                        || (s >= 300 && techList.find(Protoss_Arbiter) != techList.end())
                        || (s >= 100 && techList.find(Protoss_Carrier) != techList.end()))
                        itemQueue[check] = Item(2);
                    else
                        itemQueue[check] = Item(1);
                }
                else if (check != Protoss_Gateway)
                    itemQueue[check] = Item(1);

            }
        }
    }

    void checkExoticTech()
    {
        // Corsair/Scout upgrades
        if ((techList.find(Protoss_Scout) != techList.end() || techList.find(Protoss_Corsair) != techList.end()) && Units::getSupply() >= 300)
            itemQueue[Protoss_Fleet_Beacon] = Item(1);

        // HACK: Bluestorm carrier hack
        if (Broodwar->mapFileName().find("BlueStorm") != string::npos && techList.find(Protoss_Carrier) != techList.end())
            itemQueue[Protoss_Stargate] = Item(Broodwar->self()->visibleUnitCount(Protoss_Nexus));

        // Hive upgrades
        if (Broodwar->self()->getRace() == Races::Zerg && Units::getSupply() >= 200) {
            itemQueue[Zerg_Queens_Nest] = Item(1);
            itemQueue[Zerg_Hive] = Item(Broodwar->self()->completedUnitCount(Zerg_Queens_Nest) >= 1);
            itemQueue[Zerg_Lair] = Item(Broodwar->self()->completedUnitCount(Zerg_Queens_Nest) < 1);
        }
    }

    map<BWAPI::UnitType, Item>& getItemQueue() { return itemQueue; }
    UnitType getTechUnit() { return techUnit; }
    UnitType getFirstUnit() { return firstUnit; }
    UpgradeType getFirstUpgrade() { return firstUpgrade; }
    TechType getFirstTech() { return firstTech; }
    set <UnitType>& getTechList() { return  techList; }
    set <UnitType>& getUnlockedList() { return  unlockedType; }
    int gasWorkerLimit() { return gasLimit; }
    bool isWorkerCut() { return cutWorkers; }
    bool isUnitUnlocked(UnitType unit) { return unlockedType.find(unit) != unlockedType.end(); }
    bool isTechUnit(UnitType unit) { return techList.find(unit) != techList.end(); }
    bool isOpener() { return getOpening; }
    bool isFastExpand() { return fastExpand; }
    bool shouldScout() { return scout; }
    bool isWallNat() { return wallNat; }
    bool isWallMain() { return wallMain; }
    bool isProxy() { return proxy; }
    bool isHideTech() { return hideTech; }
    bool isPlayPassive() { return playPassive; }
    bool isRush() { return rush; }
    bool isGasTrick() { return gasTrick; }
    string getCurrentBuild() { return currentBuild; }
    string getCurrentOpener() { return currentOpener; }
    string getCurrentTransition() { return currentTransition; }
}