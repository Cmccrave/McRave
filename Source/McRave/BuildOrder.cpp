#include "McRave.h"
#include "BuildOrder.h"
#include <fstream>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::BuildOrder
{
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

        const auto shouldIncrement = [&](string name) {

            // Check if we're looking at the correct build to prevent incrementing an opener for a different build
            if (correctBuild) {

                // Check if we're looking at the correct opener to prevent incrementing a transition for a different build
                if (correctOpener) {
                    if (name == currentTransition) {
                        correctBuild = false;
                        correctOpener = false;
                        return true;
                    }
                }
                if (name == currentOpener) {
                    correctOpener = true;
                    return true;
                }
            }
            if (name == currentBuild) {
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
            // 1GateCore
            myBuilds["P1GateCore"].openers ={ "0Zealot", "1Zealot", "2Zealot" };
            myBuilds["P1GateCore"].transitions ={ "3GateRobo", "Reaver", "Corsair", "4Gate", "DT" };

            // 2Gate
            myBuilds["P2Gate"].openers ={ "Proxy", "Natural", "Main" };
            myBuilds["P2Gate"].transitions ={ "ZealotRush", "DT", "Reaver", "Expand", "DoubleExpand" };

            // FFE
            myBuilds["PFFE"].openers ={ "Gate", "Nexus", "Forge" };
            myBuilds["PFFE"].transitions ={ "NeoBisu", "2Stargate", "StormRush" };

            // 12 Nexus
            myBuilds["P12Nexus"].openers ={ "Dragoon", "Zealot" };
            myBuilds["P12Nexus"].transitions ={ "Standard", "DoubleExpand", "ReaverCarrier" };

            // 21 Nexus
            myBuilds["P21Nexus"].openers ={ "1Gate", "2Gate" };
            myBuilds["P21Nexus"].transitions ={ "Standard", "DoubleExpand", "Carrier" };
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

    bool isBuildPossible(string build, string opener)
    {
        vector<UnitType> buildings, defenses;

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (build == "P2Gate" && opener == "Natural") {
                buildings ={ Protoss_Gateway, Protoss_Gateway, Protoss_Pylon };
                defenses.insert(defenses.end(), 6, Protoss_Photon_Cannon);
            }
            else if (build == "PFFE" || build.find("Meme") != string::npos) {
                buildings ={ Protoss_Gateway, Protoss_Forge, Protoss_Pylon };
                defenses.insert(defenses.end(), 8, Protoss_Photon_Cannon);
            }
            else if (build == "P21Nexus" || build == "P12Nexus") {
                int count = Util::chokeWidth(BWEB::Map::getNaturalChoke()) / 64;
                buildings.insert(buildings.end(), count, Protoss_Pylon);
            }
            else
                return true;
        }

        if (Broodwar->self()->getRace() == Races::Terran)
            buildings ={ Terran_Barracks, Terran_Supply_Depot, Terran_Supply_Depot };

        if (Broodwar->self()->getRace() == Races::Zerg) {
            buildings ={ Zerg_Hatchery, Zerg_Evolution_Chamber, Zerg_Evolution_Chamber };
            defenses.insert(defenses.end(), 3, Zerg_Sunken_Colony);
            return true; // no walls for now
        }

        if (build == "T2Fact" || build == "TSparks") {
            if (Terrain().findMainWall(buildings, defenses))
                return true;
        }
        else {
            if (Terrain().findNaturalWall(buildings, defenses))
                return true;
        }
        return false;
    }

    bool isBuildAllowed(Race enemy, string build)
    {
        auto p = enemy == Races::Protoss;
        auto z = enemy == Races::Zerg;
        auto t = enemy == Races::Terran;
        auto r = enemy == Races::Unknown || Races::Random;

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (build == "P1GateCore")
                return true;
            if (build == "P2Gate")
                return true;
            if (build == "P12Nexus" || build == "P21Nexus")
                return t;
            if (build == "PFFE")
                return z;
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            if (build == "T2PortWraith")
                return true;
            else
                return false;
        }

        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players().vZ())
                return build == "Z9PoolSpire";
            else
                return build == "Z2HatchMuta";
        }
        return false;
    }

    bool isOpenerAllowed(Race enemy, string build, string opener)
    {
        // Ban certain openers from certain race matchups
        auto p = enemy == Races::Protoss;
        auto z = enemy == Races::Zerg;
        auto t = enemy == Races::Terran;
        auto r = enemy == Races::Unknown || Races::Random;

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (build == "P1GateCore") {
                if (opener == "0Zealot")
                    return t;
                if (opener == "1Zealot")
                    return true;
                if (opener == "2Zealot")
                    return p || z || r;
            }

            if (build == "P2Gate") {
                if (opener == "Proxy")
                    return false;
                if (opener == "Natural")
                    return z || p;
                if (opener == "Main")
                    return true;
            }

            if (build == "PFFE") {
                if (opener == "Gate" || opener == "Nexus" || opener == "Forge")
                    return z;
            }

            if (build == "P12Nexus") {
                if (opener == "Dragoon" || opener == "Zealot")
                    return /*p ||*/ t;
            }
            if (build == "P21Nexus") {
                if (opener == "1Gate" || opener == "2Gate")
                    return t;
            }
        }
        return false;
    }

    bool isTransitionAllowed(Race enemy, string build, string transition)
    {
        // Ban certain transitions from certain race matchups
        auto p = enemy == Races::Protoss;
        auto z = enemy == Races::Zerg;
        auto t = enemy == Races::Terran;
        auto r = enemy == Races::Unknown || Races::Random;

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (build == "P1GateCore") {
                if (transition == "DT")
                    return t || z;
                if (transition == "3GateRobo")
                    return p || r;
                if (transition == "Corsair")
                    return z;
                if (transition == "Reaver")
                    return p /*|| t*/ || r;
                if (transition == "4Gate")
                    return true;
            }

            if (build == "P2Gate") {
                if (transition == "DT")
                    return p || t;
                if (transition == "Reaver")
                    return p /*|| t*/ || r;
                if (transition == "Expand")
                    return p || z;
                if (transition == "DoubleExpand")
                    return t;
                //if (transition == "ZealotRush")
                //	return true;
            }

            if (build == "PFFE") {
                if (transition == "NeoBisu" || transition == "2Stargate" || transition == "StormRush")
                    return z;
            }

            if (build == "P12Nexus") {
                if (transition == "DoubleExpand"/* || transition == "ReaverCarrier"*/)
                    return t;
                if (transition == "Standard")
                    return t || p;
            }

            if (build == "P21Nexus") {
                if (transition == "DoubleExpand" || transition == "Carrier" || transition == "Standard")
                    return t;
            }
        }
        return false;
    }

    bool techComplete()
    {
        if (techUnit == UnitTypes::Protoss_Scout || techUnit == UnitTypes::Protoss_Corsair || techUnit == UnitTypes::Protoss_Observer || techUnit == UnitTypes::Terran_Science_Vessel)
            return (Broodwar->self()->completedUnitCount(techUnit) > 0);
        if (techUnit == UnitTypes::Protoss_High_Templar)
            return (Broodwar->self()->hasResearched(TechTypes::Psionic_Storm));
        if (techUnit == UnitTypes::Protoss_Arbiter)
            return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
        if (techUnit == UnitTypes::Protoss_Dark_Templar)
            return (Broodwar->self()->completedUnitCount(techUnit) >= 2);
        if (techUnit == UnitTypes::Protoss_Reaver)
            return Broodwar->self()->completedUnitCount(techUnit) > 0; //return (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Shuttle) >= 1) || (Broodwar->self()->completedUnitCount(techUnit) > 0 && Terrain().isNarrowNatural());
        return (Broodwar->self()->visibleUnitCount(techUnit) > 0);
    }

    void getDefaultBuild()
    {
        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players().getNumberProtoss() > 0) {
                currentBuild = "P1GateCore";
                currentOpener = "1Zealot";
                currentTransition = "Reaver";
            }
            else if (Players().getNumberZerg() > 0) {
                currentBuild = "P1GateCore";
                currentOpener = "2Zealot";
                currentTransition = "DT";
            }
            else if (Players().getNumberTerran() > 0) {
                currentBuild = "P21Nexus";
                currentOpener = "1Gate";
                currentTransition = "Standard";
                isBuildPossible(currentBuild, currentOpener);
            }
            else if (Players().getNumberRandom() > 0) {
                currentBuild = "P2Gate";
                currentOpener = "Main";
                currentTransition = "Reaver";
            }
        }
        if (Broodwar->self()->getRace() == Races::Terran)
            currentBuild = "TSparks";
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Players().getNumberZerg() > 0)
                currentBuild = "Z9PoolSpire";
            else
                currentBuild = "Z2HatchMuta";
        }
        return;
    }

    void onFrame()
    {
        Display().startClock();
        updateBuild();
        Display().performanceTest(__FUNCTION__);
    }

    void updateBuild()
    {
        if (Broodwar->self()->getRace() == Races::Protoss) {
            Protoss::opener();
            Protoss::tech();
            Protoss::situational();
            Protoss::unlocks();
        }
        //if (Broodwar->self()->getRace() == Races::Terran) {
        //	terranOpener();
        //	terranTech();
        //	terranSituational();
        //}
        //if (Broodwar->self()->getRace() == Races::Zerg) {
        //	zergOpener();
        //	zergTech();
        //	zergSituational();
        //	zergUnlocks();
        //}
    }

    bool shouldExpand()
    {
        UnitType baseType = Broodwar->self()->getRace().getResourceDepot();

        if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Broodwar->self()->minerals() > 500 + (100 * Broodwar->self()->completedUnitCount(baseType)))
                return true;
            else if (techUnit == UnitTypes::None && !Production().hasIdleProduction() && Resources().isMinSaturated() && techSat && productionSat)
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Terran) {
            if (Broodwar->self()->minerals() > 400 + (100 * Broodwar->self()->completedUnitCount(baseType)))
                return true;
        }
        else if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings::getQueuedMineral() >= 300 && !getOpening)
                return true;
        }
        return false;
    }

    bool shouldAddProduction()
    {
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Broodwar->self()->visibleUnitCount(UnitTypes::Zerg_Larva) <= 1 && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings::getQueuedMineral() >= 200 && !getOpening)
                return true;
        }
        else {
            if ((Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings::getQueuedMineral() > 150) || (!productionSat && Resources().isMinSaturated()))
                return true;
        }
        return false;
    }

    bool shouldAddGas()
    {
        auto workerCount = Broodwar->self()->completedUnitCount(Broodwar->self()->getRace().getWorker());
        if (Broodwar->self()->getRace() == Races::Zerg) {
            if (Resources().isGasSaturated() && Broodwar->self()->minerals() - Production().getReservedMineral() - Buildings::getQueuedMineral() > 500)
                return true;
        }

        else if (Broodwar->self()->getRace() == Races::Protoss) {
            if (Players().vP())
                return Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Assimilator) != 1 || workerCount >= 32 || Broodwar->self()->minerals() > 600;
            else if (Players().vT() || Players().vZ())
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

        // Otherwise, choose a tech based on highest unit score
        double highest = 0.0;
        for (auto &tech : Strategy().getUnitScores()) {
            if (tech.first == UnitTypes::Protoss_Dragoon
                || tech.first == UnitTypes::Protoss_Zealot
                || tech.first == UnitTypes::Protoss_Shuttle)
                continue;

            if (tech.second > highest) {
                highest = tech.second;
                techUnit = tech.first;
            }
        }
    }

    void checkNewTech()
    {
        // No longer need to choose a tech
        if (techUnit != UnitTypes::None) {
            getTech = false;
            techList.insert(techUnit);
            unlockedType.insert(techUnit);
        }
        if (firstUnit != UnitTypes::None) {
            techList.insert(firstUnit);
            unlockedType.insert(firstUnit);
        }

        // Multi-unlock
        if (techUnit == UnitTypes::Protoss_Arbiter || techUnit == UnitTypes::Protoss_High_Templar) {
            unlockedType.insert(UnitTypes::Protoss_Dark_Templar);
            techList.insert(UnitTypes::Protoss_Dark_Templar);
        }
        else if (techUnit == UnitTypes::Protoss_Reaver) {
            unlockedType.insert(UnitTypes::Protoss_Shuttle);
            techList.insert(UnitTypes::Protoss_Shuttle);
        }
        else if (techUnit == UnitTypes::Zerg_Mutalisk && Units().getGlobalEnemyAirStrength() > 0.0) {
            techList.insert(UnitTypes::Zerg_Scourge);
            unlockedType.insert(UnitTypes::Zerg_Scourge);
        }
        else if (techUnit == UnitTypes::Zerg_Lurker) {
            techList.insert(UnitTypes::Zerg_Hydralisk);
            unlockedType.insert(UnitTypes::Zerg_Hydralisk);
        }

        // HACK: If we have a Reaver add Obs to the tech
        if (Broodwar->self()->completedUnitCount(UnitTypes::Protoss_Reaver) >= 3) {
            techList.insert(UnitTypes::Protoss_Observer);
            unlockedType.insert(UnitTypes::Protoss_Observer);
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
            if (check == UnitTypes::Zerg_Lurker)
                itemQueue[UnitTypes::Zerg_Lair] = Item(1);

            // Add extra production - TODO: move to shouldAddProduction
            int s = Units().getSupply();
            if (canAdd && buildCount(check) <= 1) {
                if (check == UnitTypes::Protoss_Stargate) {
                    if ((s >= 150 && techList.find(UnitTypes::Protoss_Corsair) != techList.end())
                        || (s >= 300 && techList.find(UnitTypes::Protoss_Arbiter) != techList.end()))
                        itemQueue[check] = Item(2);
                    else
                        itemQueue[check] = Item(1);
                }
                else if (check != UnitTypes::Protoss_Gateway)
                    itemQueue[check] = Item(1);

            }
        }
    }

    void checkExoticTech()
    {
        // Corsair/Scout upgrades
        if ((techList.find(UnitTypes::Protoss_Scout) != techList.end() && currentBuild != "PDTExpand") || (techList.find(UnitTypes::Protoss_Corsair) != techList.end() && Units().getSupply() >= 300))
            itemQueue[UnitTypes::Protoss_Fleet_Beacon] = Item(1);

        // HACK: Bluestorm carrier hack
        if (Broodwar->mapFileName().find("BlueStorm") != string::npos && techList.find(UnitTypes::Protoss_Carrier) != techList.end())
            itemQueue[UnitTypes::Protoss_Stargate] = Item(Broodwar->self()->visibleUnitCount(UnitTypes::Protoss_Nexus));

        // Hive upgrades
        if (Broodwar->self()->getRace() == Races::Zerg && Units().getSupply() >= 200) {
            itemQueue[UnitTypes::Zerg_Queens_Nest] = Item(1);
            itemQueue[UnitTypes::Zerg_Hive] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) >= 1);
            itemQueue[UnitTypes::Zerg_Lair] = Item(Broodwar->self()->completedUnitCount(UnitTypes::Zerg_Queens_Nest) < 1);
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
    string getCurrentBuild() { return currentBuild; }
    string getCurrentOpener() { return currentOpener; }
    string getCurrentTransition() { return currentTransition; }
}