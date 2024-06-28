#include "Main/McRave.h"
#include "BuildOrder.h"
#include <fstream>
#include <iomanip>

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::Learning {
    namespace {

        bool mapLearning;
        vector<Build> myBuilds;
        stringstream ss;
        string mapName;
        string noStats;
        string myRaceChar, enemyRaceChar;
        string version;
        string learningExtension, gameInfoExtension;

        bool isBuildPossible(string build, string opener)
        {
            vector<UnitType> buildings, defenses;
            auto wallOptional = false;
            auto tight = false;

            // Protoss wall requirements
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Terrain::getNaturalArea()->ChokePoints().size() == 1)
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
                return true;
            return false;
        }

        void getDefaultBuild()
        {
            // Protoss
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::PvP())
                    BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "Robo");
                else if (Players::PvZ())
                    BuildOrder::setLearnedBuild("2Gate", "Main", "4Gate");
                else if (Players::PvT())
                    BuildOrder::setLearnedBuild("2Base", "21Nexus", "Obs");
                else if (Players::PvFFA() || Players::PvTVB())
                    BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "3Gate");
                else
                    BuildOrder::setLearnedBuild("2Gate", "Main", "Robo");
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::ZvZ())
                    BuildOrder::setLearnedBuild("PoolLair", "9Pool", "1HatchMuta");
                else if (Players::ZvT())
                    BuildOrder::setLearnedBuild("HatchPool", "12Hatch", "2HatchMuta");
                else if (Players::ZvP())
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "2HatchMuta");
                else if (Players::ZvFFA() || Players::ZvTVB())
                    BuildOrder::setLearnedBuild("HatchPool", "10Hatch", "3HatchMuta");
                else
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "2HatchSpeedling");
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Players::TvZ())
                    BuildOrder::setLearnedBuild("2Rax", "11/13", "Academy");
                else
                    BuildOrder::setLearnedBuild("RaxFact", "2FactFE", "5Fact");
            }
        }

        void getBestBuild()
        {
            int totalWins = 0;
            int totalLoses = 0;
            int totalGames = 0;
            auto enemyRace = Broodwar->enemy()->getRace();

            const auto parseLearningFile = [&](ifstream &file) {
                string buffer, token;
                while (file >> buffer)
                    ss << buffer << " ";

                // Create a copy so we aren't dumping out the information
                stringstream sscopy;
                sscopy << ss.str();

                Build * currentBuild = nullptr;
                while (!sscopy.eof()) {
                    sscopy >> token;

                    if (token == "Total") {
                        sscopy >> totalWins >> totalLoses;
                        totalGames = totalWins + totalLoses;
                        continue;
                    }

                    // Build winrate
                    auto itr = find_if(myBuilds.begin(), myBuilds.end(), [&](const auto& u) { return u.name == token; });
                    if (itr != myBuilds.end()) {
                        currentBuild = &*itr;
                        sscopy >> itr->w >> itr->l;
                    }
                    else if (currentBuild && currentBuild->getComponent(token))
                        sscopy >> currentBuild->getComponent(token)->w >> currentBuild->getComponent(token)->l;

                }
            };
            auto randomness = max(1, 10 - totalGames);

            const auto calculateUCB = [&](int w, int l) {
                auto UCB = (w + l) > 0 ? (double(w) / double(w + l)) + pow(2.0 * log((double)totalGames) / double(w + l), 0.1) : 1.0;
                return (UCB * 25.0) + double(rand() % randomness) + 1.0;
            };

            // Attempt to read a file from the read directory first, then write directory
            ifstream readFile("bwapi-data/read/" + learningExtension);
            ifstream writeFile("bwapi-data/write/" + learningExtension);
            if (readFile)
                parseLearningFile(readFile);
            else if (writeFile)
                parseLearningFile(writeFile);

            // No file found, create a new one
            else {
                if (!writeFile.good()) {

                    ss << "Total " << noStats;
                    for (auto &build : myBuilds) {
                        ss << build.name << noStats;

                        for (auto &opener : build.openers)
                            ss << opener.name << noStats;
                        for (auto &transition : build.transitions)
                            ss << transition.name << noStats;
                    }
                }
            }

            // Calculate UCB1 values - sort by descending value
            for (auto &build : myBuilds) {
                build.ucb1 = calculateUCB(build.w, build.l);
                for (auto &opener : build.openers)
                    opener.ucb1 = calculateUCB(opener.w, opener.l);
                for (auto &transition : build.transitions) {
                    transition.ucb1 = calculateUCB(transition.w, transition.l);
                    Util::debug("[Learning]: " + transition.name + " UCB1 value is " + to_string(transition.ucb1) + "(" + to_string(transition.w) + ", " + to_string(transition.l) + ")");
                }                

                sort(build.openers.begin(), build.openers.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });
                sort(build.transitions.begin(), build.transitions.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });
            }
            sort(myBuilds.begin(), myBuilds.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });

            // Pick the best build that is allowed
            auto bestBuildUCB1 = 0.0;
            auto bestOpenerUCB1 = 0.0;
            auto bestTransitionUCB1 = 0.0;
            for (auto &build : myBuilds) {
                if (build.ucb1 < bestBuildUCB1)
                    continue;

                bestBuildUCB1 = build.ucb1;
                bestOpenerUCB1 = 0.0;
                bestTransitionUCB1 = 0.0;

                for (auto &opener : build.openers) {
                    if (opener.ucb1 < bestOpenerUCB1)
                        continue;
                    bestOpenerUCB1 = opener.ucb1;
                    bestTransitionUCB1 = 0.0;

                    for (auto &transition : build.transitions) {
                        if (transition.ucb1 < bestTransitionUCB1)
                            continue;
                        bestTransitionUCB1 = transition.ucb1;
                        BuildOrder::setLearnedBuild(build.name, opener.name, transition.name);
                    }
                }
            }
        }

        void getPermanentBuild()
        {
            // Testing builds if needed
            if (Players::ZvZ()) {
                if (Players::PvZ()) {
                    BuildOrder::setLearnedBuild("FFE", "Forge", "NeoBisu");
                    return;
                }
                if (Players::PvP()) {
                    BuildOrder::setLearnedBuild("1GateCore", "1Zealot", "DT");
                    return;
                }
                if (Players::PvT()) {
                    BuildOrder::setLearnedBuild("2Base", "21Nexus", "Obs");
                    return;
                }
                if (Players::ZvZ()) {
                    BuildOrder::setLearnedBuild("PoolHatch", "12Pool", "2HatchMuta");
                    return;
                }
                if (Players::ZvT()) {
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "2HatchMuta");
                    return;
                }
                if (Players::ZvP()) {
                    BuildOrder::setLearnedBuild("PoolHatch", "Overpool", "3HatchMuta");
                    return;
                }
            }
        }

        void protossBuildMaps()
        {
            Build OneGateCore("1GateCore");
            Build TwoGate("2Gate");
            Build TwoBase("2Base");
            Build FFE("FFE");

            // PvT
            if (Players::PvT()) {
                OneGateCore.setOpeners({ "0Zealot", "1Zealot" });
                OneGateCore.setTransitions({ "DT" });

                TwoGate.setOpeners({ "Main" });
                TwoGate.setTransitions({ "DT" });

                TwoBase.setOpeners({ "12Nexus", "20Nexus", "21Nexus" });
                TwoBase.setTransitions({ "Obs", "Carrier", "ReaverCarrier" });

                myBuilds ={ OneGateCore, TwoGate, TwoBase };
            }

            // PvP
            if (Players::PvP()) {
                OneGateCore.setOpeners({ "1Zealot" });
                OneGateCore.setTransitions({ "DT", "Robo", "4Gate", "3Gate" });

                TwoGate.setOpeners({ "Main" });
                TwoGate.setTransitions({ "DT", "Robo" });

                myBuilds ={ OneGateCore, TwoGate };
            }

            // PvZ
            if (Players::PvZ()) {
                TwoGate.setOpeners({ "Main" });
                TwoGate.setTransitions({ "4Gate" });

                FFE.setOpeners({ "Forge" });
                FFE.setTransitions({ "NeoBisu", "2Stargate", "StormRush", "5GateGoon" });

                myBuilds ={ TwoGate, FFE };
            }

            // PvR
            if (Players::PvR()) {
                OneGateCore.setOpeners({ "2Zealot" });
                OneGateCore.setTransitions({ "Robo", "3Gate" });

                TwoGate.setOpeners({ "Main" });
                TwoGate.setTransitions({ "Robo" });

                myBuilds ={ OneGateCore, TwoGate };
            }
        }

        void zergBuildMaps()
        {
            Build PoolHatch("PoolHatch");
            Build HatchPool("HatchPool");
            Build PoolLair("PoolLair");

            if (Players::ZvP()) {
                PoolHatch.setOpeners({ "Overpool" });
                PoolHatch.setTransitions({ "2HatchMuta", "3HatchMuta", "3HatchHydra", "4HatchHydra", "6HatchHydra" });

                HatchPool.setOpeners({ "10Hatch", "12Hatch" });
                HatchPool.setTransitions({ "2HatchMuta", "3HatchMuta", "3HatchHydra", "4HatchHydra", "6HatchHydra" });

                myBuilds ={ PoolHatch, HatchPool };
            }

            if (Players::ZvT()) {
                PoolHatch.setOpeners({ "4Pool", "Overpool", "12Pool" });
                PoolHatch.setTransitions({ "2HatchMuta", "3HatchMuta" });

                HatchPool.setOpeners({ "12Hatch" });
                HatchPool.setTransitions({ "2HatchMuta", "3HatchMuta" });

                myBuilds ={ PoolHatch, HatchPool };
            }

            if (Players::ZvZ()) {
                PoolHatch.setOpeners({ "12Pool" });
                PoolHatch.setTransitions({ "2HatchMuta"/*, "2HatchHydra"*/ });

                PoolLair.setOpeners({ "9Pool" });
                PoolLair.setTransitions({ "1HatchMuta" });

                myBuilds ={ PoolHatch, PoolLair };
            }

            if (Players::ZvR()) {
                PoolHatch.setOpeners({ "Overpool" });
                PoolHatch.setTransitions({ "2HatchMuta" });

                myBuilds ={ PoolHatch };
            }
        }

        void terranBuildMaps()
        {
            Build TwoRax("2Rax");
            Build RaxFact("RaxFact");

            if (Players::TvP()) {
                RaxFact.setOpeners({ "1FactFE", "2FactFE" });
                RaxFact.setTransitions({ "5Fact" });

                myBuilds ={ RaxFact };
            }

            if (Players::TvT()) {
                RaxFact.setOpeners({ "1FactFE" });
                RaxFact.setTransitions({ "5Fact" });

                myBuilds ={ RaxFact };
            }

            if (Players::TvZ()) {
                TwoRax.setOpeners({ "11/13" });
                TwoRax.setTransitions({ "Academy" });

                myBuilds ={ TwoRax };
            }
        }

        void createBuildMaps()
        {
            if (Broodwar->self()->getRace() == Races::Protoss)
                protossBuildMaps();
            if (Broodwar->self()->getRace() == Races::Zerg)
                zergBuildMaps();
            if (Broodwar->self()->getRace() == Races::Terran)
                terranBuildMaps();
        }
    }

    vector<Build>& getBuilds() { return myBuilds; }

    void onEnd(bool isWinner)
    {
        if (!Broodwar->enemy() || Players::ZvFFA() || Players::ZvTVB() || !Broodwar->isMultiplayer())
            return;

        // HACK: Don't touch records if we play islands, since islands aren't fully implemented yet
        if (Terrain::isIslandMap())
            return;

        // Write into the write directory 3 tokens at a time (4 if we detect a dash)
        ofstream config("bwapi-data/write/" + learningExtension);
        string token;
        bool foundLearning = false;

        // For each token, check if we should increment the wins or losses then shove it into the config file
        while (ss >> token) {
            int w, l;
            ss >> w >> l;
            if (BuildOrder::getCurrentBuild() == token)
                foundLearning = true;
            if (BuildOrder::getCurrentBuild() == token || token == "Total" || (foundLearning && (BuildOrder::getCurrentOpener() == token || BuildOrder::getCurrentTransition() == token)))
                isWinner ? w++ : l++;
            if (BuildOrder::getCurrentTransition() == token)
                foundLearning = false;
            config << token << " " << w << " " << l << endl;
        }

        const auto copyFile = [&](string source, string destination) {
            std::ifstream src(source, std::ios::binary);
            std::ofstream dest(destination, std::ios::binary);
            dest << src.rdbuf();
            return src && dest;
        };

        // Write into the write directory information about what we saw the enemy do
        ifstream readFile("bwapi-data/read/" + gameInfoExtension);
        if (readFile)
            copyFile("bwapi-data/read/" + gameInfoExtension, "bwapi-data/write/" + gameInfoExtension);
        ofstream gameLog("bwapi-data/write/" + gameInfoExtension, std::ios_base::app);

        // Who won on what map in how long
        gameLog << (isWinner ? "Won" : "Lost") << ","
            << mapName << ","
            << std::setfill('0') << Util::getTime().minutes << ":" << std::setw(2) << Util::getTime().seconds << ",";

        // What strategies were used/detected
        gameLog
            << Spy::getEnemyBuild() << ","
            << Spy::getEnemyOpener() << ","
            << Spy::getEnemyTransition() << ","
            << currentBuild << "," << currentOpener << "," << currentTransition << ",";

        // When did we detect the enemy strategy
        gameLog << std::setfill('0') << Spy::getEnemyBuildTime().minutes << ":" << std::setw(2) << Spy::getEnemyBuildTime().seconds << ",";
        gameLog << std::setfill('0') << Spy::getEnemyOpenerTime().minutes << ":" << std::setw(2) << Spy::getEnemyOpenerTime().seconds << ",";
        gameLog << std::setfill('0') << Spy::getEnemyTransitionTime().minutes << ":" << std::setw(2) << Spy::getEnemyTransitionTime().seconds;

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
        if (!Broodwar->enemy() || Players::vFFA() || Players::vTVB()) {
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
        Util::debug("[Learning]: New game on " + mapName);

        // File extension including our race initial;
        mapLearning         = false;
        myRaceChar          ={ *Broodwar->self()->getRace().c_str() };
        enemyRaceChar       ={ *Broodwar->enemy()->getRace().c_str() };
        version             = "Offseason2023";
        noStats             = " 0 0 ";
        learningExtension   = myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + version + " Learning.txt";
        gameInfoExtension   = myRaceChar + "v" + enemyRaceChar + " " + Broodwar->enemy()->getName() + " " + version + " Info.txt";

        createBuildMaps();
        getDefaultBuild();
        getBestBuild();
        getPermanentBuild();
    }
}