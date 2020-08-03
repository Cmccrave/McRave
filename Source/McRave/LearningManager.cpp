#include "McRave.h"
#include "BuildOrder.h"
#include <fstream>
#include <iomanip>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::Learning {
    namespace {
        map <string, Build> myBuilds;
        stringstream ss;
        vector <string> buildNames;
        string enemyRaceChar, mapName;

        bool isBuildPossible(string build, string opener)
        {
            vector<UnitType> buildings, defenses;
            auto wallOptional = false;
            auto tight = false;

            // Protoss wall requirements
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (BWEB::Map::getNaturalArea()->ChokePoints().size() == 1)
                    return Walls::getMainWall();
                if (build == "FFE")
                    return Walls::getNaturalWall();
                return true;
            }

            // Zerg wall requirements
            if (Broodwar->self()->getRace() == Races::Zerg)
                return true;

            // Terran wall requirements
            if (Broodwar->self()->getRace() == Races::Terran)
                return Walls::getMainWall();
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
                    return t || p;
                if (build == "2Gate")
                    return true;
                if (build == "NexusGate" || build == "GateNexus")
                    return t;
                if (build == "FFE")
                    return z && !Terrain::isShitMap() && !Terrain::isIslandMap();
            }

            if (Broodwar->self()->getRace() == Races::Terran && build != "") {
                return true; // For now, test all builds to make sure they work!
            }

            if (Broodwar->self()->getRace() == Races::Zerg && build != "") {
                if (build == "PoolLair")
                    return z;
                if (build == "HatchPool")
                    return t;
                if (build == "PoolHatch")
                    return p || t;
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
                        return t || p;
                    if (opener == "1Zealot")
                        return true;
                    if (opener == "2Zealot")
                        return z || r;
                }

                if (build == "2Gate") {
                    if (opener == "Proxy")
                        return false;// !Terrain::isIslandMap() && (p /*|| z*/);
                    if (opener == "Natural")
                        return false;
                    if (opener == "Main")
                        return true;
                }

                if (build == "FFE") {
                    if (/*opener == "Gate" || opener == "Nexus" || */opener == "Forge")
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
                if (build == "PoolLair") {
                    if (opener == "9Pool")
                        return z;
                }
                if (build == "HatchPool") {
                    if (opener == "12Hatch")
                        return t;
                }
                if (build == "PoolHatch") {
                    if (opener == "Overpool")
                        return p;
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
            auto r = enemy == Races::Unknown || enemy == Races::Random;

            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (build == "1GateCore") {
                    if (transition == "DT")
                        return !Terrain::isShitMap() && (p || t /*|| z*/);
                    if (transition == "3Gate")
                        return p || r;
                    if (transition == "Robo")
                        return !Terrain::isShitMap() && (p /*|| t*/ || r);
                    if (transition == "4Gate")
                        return p;
                }

                if (build == "2Gate") {
                    if (transition == "DT")
                        return p || t;
                    if (transition == "Robo")
                        return p /*|| t*/ || r;
                    if (transition == "Expand")
                        return false;
                    if (transition == "DoubleExpand")
                        return t;
                    if (transition == "4Gate")
                        return z;
                }

                if (build == "FFE") {
                    if (transition == "NeoBisu" || transition == "2Stargate" || transition == "StormRush" || transition == "5GateGoon")
                        return z;
                }

                if (build == "NexusGate") {
                    if (transition == "DoubleExpand" || transition == "ReaverCarrier")
                        return t;
                    if (transition == "Standard")
                        return t;
                }

                if (build == "GateNexus") {
                    if (transition == "DoubleExpand" || transition == "Carrier" || transition == "Standard")
                        return t;
                }
            }

            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (build == "PoolLair") {
                    if (transition == "1HatchMuta")
                        return z;
                }
                if (build == "HatchPool") {
                    if (transition == "2HatchMuta")
                        return !Terrain::isShitMap() && t;
                }
                if (build == "PoolHatch") {
                    if (transition == "2HatchMuta")
                        return !Terrain::isShitMap() && p;
                    if (transition == "2HatchSpeedling")
                        return Terrain::isShitMap() && p;
                    if (transition == "3HatchSpeedling")
                        return Terrain::isShitMap() && t;
                }
            }
            return false;
        }

        void getDefaultBuild()
        {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::getPlayers().size() > 3) {
                    if (Players::getRaceCount(Races::Zerg, PlayerState::Enemy) > Players::getRaceCount(Races::Protoss, PlayerState::Enemy) + Players::getRaceCount(Races::Terran, PlayerState::Enemy))
                        BuildOrder::setLearnedBuild("FFE", "Forge", "5GateGoon");
                    else if (Players::getRaceCount(Races::Protoss, PlayerState::Enemy) > Players::getRaceCount(Races::Zerg, PlayerState::Enemy) + Players::getRaceCount(Races::Terran, PlayerState::Enemy))
                        BuildOrder::setLearnedBuild("1GateCore", "2Zealot", "Robo");
                    else if (Players::getRaceCount(Races::Terran, PlayerState::Enemy) > Players::getRaceCount(Races::Zerg, PlayerState::Enemy) + Players::getRaceCount(Races::Protoss, PlayerState::Enemy))
                        BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "Robo");
                }
                else if (Players::vP())
                    BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "Robo");
                else if (Players::vZ())
                    BuildOrder::setLearnedBuild("2Gate", "Main", "4Gate");
                else if (Players::vT())
                    BuildOrder::setLearnedBuild("GateNexus", "1Gate", "Standard");
                else
                    BuildOrder::setLearnedBuild("2Gate", "Main", "Robo");
            }
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::vZ())
                    BuildOrder::setLearnedBuild("PoolLair", "9Pool", "1HatchMuta");
                else if (Players::vT())
                    BuildOrder::setLearnedBuild("HatchPool", "12Hatch", "2HatchMuta");
                else if (Players::vP())
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "2HatchMuta");
                else
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "Unknown");
            }

            // Add walls
            isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
        }
    }

    void onEnd(bool isWinner)
    {
        if (!Broodwar->enemy() || Players::getPlayers().size() > 3)
            return;

        // HACK: Don't touch records if we play Plasma
        if (Terrain::isIslandMap())
            return;

        // File extension including our race initial;
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string learningExtension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + Broodwar->mapFileName() + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";
        const string gameInfoExtension = myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " Info.txt";

        // Write into the write directory 3 tokens at a time (4 if we detect a dash)
        ofstream config("bwapi-data/write/" + learningExtension);
        string token;
        LearningToken currentToken = LearningToken::Build;

        const auto shouldIncrement = [&](string name) {

            // Depending on token, increment the correct data
            if (currentToken == LearningToken::Transition && name == BuildOrder::getCurrentTransition()) {
                currentToken = LearningToken::None;
                return true;
            }
            if (currentToken == LearningToken::Opener && name == BuildOrder::getCurrentOpener()) {
                currentToken = LearningToken::Transition;
                return true;
            }
            if (currentToken == LearningToken::Build && name == BuildOrder::getCurrentBuild()) {
                currentToken = LearningToken::Opener;
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

        // Write into the write directory information about what we saw the enemy do
        ofstream gameLog("bwapi-data/write/" + gameInfoExtension, std::ios_base::app);

        // Who won on what map in how long
        gameLog << (isWinner ? "Won" : "Lost") << ","
            << mapName << ","
            << std::setfill('0') << Util::getTime().minutes << ":" << std::setw(2) << Util::getTime().seconds << ",";

        // What strategies were used/detected
        gameLog
            << Strategy::getEnemyBuild() << ","
            << Strategy::getEnemyOpener() << ","
            << Strategy::getEnemyTransition() << ","
            << currentBuild << "," << currentOpener << "," << currentTransition << ",";

        // When did we detect the enemy strategy
        gameLog << std::setfill('0') << Strategy::getEnemyBuildTime().minutes << ":" << std::setw(2) << Strategy::getEnemyOpenerTime().seconds << ",";
        gameLog << std::setfill('0') << Strategy::getEnemyOpenerTime().minutes << ":" << std::setw(2) << Strategy::getEnemyBuildTime().seconds << ",";
        gameLog << std::setfill('0') << Strategy::getEnemyTransitionTime().minutes << ":" << std::setw(2) << Strategy::getEnemyTransitionTime().seconds;

        // Store a list of total units everyone made
        for (auto &type : UnitTypes::allUnitTypes()) {
            if (!type.isBuilding()) {
                auto cnt = Players::getTotalCount(PlayerState::Self, type);
                if (cnt > 0)
                    gameLog << "," << cnt << "," << type.c_str();
            }
        }
        for (auto &type : UnitTypes::allUnitTypes()) {
            if (!type.isBuilding()) {
                auto cnt = Players::getTotalCount(PlayerState::Enemy, type);
                if (cnt > 0)
                    gameLog << "," << cnt << "," << type.c_str();
            }
        }

        gameLog << endl;
    }

    void onStart()
    {
        if (!Broodwar->enemy() || Players::getPlayers().size() > 3) {
            getDefaultBuild();
            return;
        }

        // Grab only the alpha characters from the map name to remove version numbers
        for (auto &c : Broodwar->mapFileName()) {
            if (isalpha(c))
                mapName.push_back(c);
            if (c == '.')
                break;
        }

        // File extension including our race initial;
        enemyRaceChar ={ *Broodwar->enemy()->getRace().c_str() };
        const auto mapLearning = false;
        const string dash = "-";
        const string noStats = " 0 0 ";
        const string myRaceChar{ *Broodwar->self()->getRace().c_str() };
        const string extension = mapLearning ? myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + mapName + ".txt" : myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + ".txt";

        // Tokens
        string buffer, token;

        // Protoss builds, openers and transitions
        if (Broodwar->self()->getRace() == Races::Protoss) {

            myBuilds["1GateCore"].openers ={ "0Zealot", "1Zealot", "2Zealot" };
            myBuilds["1GateCore"].transitions ={ "DT", "Robo", "4Gate", "3Gate" };

            myBuilds["2Gate"].openers ={ "Proxy", "Natural", "Main" };
            myBuilds["2Gate"].transitions ={ "DT", "Robo", "4Gate", "Expand", "DoubleExpand" };

            myBuilds["FFE"].openers ={ "Forge", "Gate", "Nexus" };
            myBuilds["FFE"].transitions ={ "NeoBisu", "2Stargate", "StormRush", "4GateArchon", "CorsairReaver", "5GateGoon" };

            myBuilds["NexusGate"].openers ={ "Dragoon", "Zealot" };
            myBuilds["NexusGate"].transitions ={ "Standard", "DoubleExpand", "ReaverCarrier" };

            myBuilds["GateNexus"].openers ={ "1Gate", "2Gate" };
            myBuilds["GateNexus"].transitions ={ "Standard", "DoubleExpand", "Carrier" };
        }

        if (Broodwar->self()->getRace() == Races::Zerg) {

            myBuilds["PoolHatch"].openers ={ "Overpool" };
            myBuilds["PoolHatch"].transitions={ "2HatchMuta", "2HatchSpeedling", "3HatchSpeedling" };

            myBuilds["HatchPool"].openers ={ "12Hatch" };
            myBuilds["HatchPool"].transitions={ "2HatchMuta", "2HatchSpeedling" };

            myBuilds["PoolLair"].openers ={ "9Pool" };
            myBuilds["PoolLair"].transitions={ "1HatchMuta" };
        }

        if (Broodwar->self()->getRace() == Races::Terran) {
            BuildOrder::setLearnedBuild("RaxFact", "10Rax", "2Fact");
            isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
            return;// Don't know what to do yet
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
                double ucbVal = numGames > 0 ? cbrt(2.0 * log((double)totalGamesPlayed) / double(numGames)) : DBL_MAX;
                double val = winRate + ucbVal;

                if (w > 0 && l == 0)
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

        if (bestBuild != "" && bestOpener != "" && bestTransition != "" && isBuildPossible(bestBuild, bestOpener) && isBuildAllowed(Broodwar->enemy()->getRace(), bestBuild))
            BuildOrder::setLearnedBuild(bestBuild, bestOpener, bestTransition);
        else
            getDefaultBuild();

        // Hardcoded stuff
        if (false) {
            if (Players::PvZ()) {
                BuildOrder::setLearnedBuild("FFE", "Forge", "NeoBisu");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Players::PvP()) {
                BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "DT");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Players::PvT()) {
                BuildOrder::setLearnedBuild("NexusGate", "Dragoon", "Standard");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Players::ZvZ()) {
                BuildOrder::setLearnedBuild("PoolLair", "9Pool", "1HatchMuta");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Players::ZvT()) {
                BuildOrder::setLearnedBuild("HatchPool", "12Hatch", "2HatchMuta");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
            if (Players::ZvP()) {
                BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "2HatchMuta");
                isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                return;
            }
        }

        if (Terrain::isIslandMap()) {
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::vT()) {
                    BuildOrder::setLearnedBuild("NexusGate", "Dragoon", "ReaverCarrier");
                    isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                    return;
                }
                else {
                    BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "4Gate");
                    isBuildPossible(BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener());
                    return;
                }
            }
        }
    }
}