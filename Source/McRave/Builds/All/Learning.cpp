#include "Learning.h"

#include <fstream>
#include <iomanip>

#include "BuildOrder.h"
#include "Info/Player/Players.h"
#include "Map/Terrain/Terrain.h"
#include "Map/Walls/Walls.h"
#include "Strategy/Spy/Spy.h"

using namespace std;
using namespace BWAPI;
using namespace UnitTypes;
using namespace McRave::BuildOrder::All;

namespace McRave::Learning {
    namespace {

        bool mapLearning;
        vector<Build> myBuilds;
        stringstream ss;
        string noStats;
        string myRaceChar, enemyRaceChar;
        string version;
        string learningExtension, gameInfoExtension;

        bool isComponentPossible(string component)
        {
            // Protoss wall requirements
            if (Broodwar->self()->getRace() == Races::Protoss) {
                if (Players::PvZ() && component == P_FFE) {
                    if (Terrain::isPocketNatural())
                        return Walls::getMainWall();
                    return Walls::getNaturalWall();
                }
                return true;
            }

            // Zerg wall requirements
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::ZvP() && component == Z_2HatchMuta) {
                    return !Terrain::isPocketNatural();
                }
                return true;
            }

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
                    BuildOrder::setLearnedBuild(P_1GateCore, P_ZCore, P_Robo);
                else if (Players::PvZ())
                    BuildOrder::setLearnedBuild(P_2Gate, P_10_12, P_4Gate);
                else if (Players::PvT())
                    BuildOrder::setLearnedBuild(P_2Base, P_21Nexus, P_Obs);
                else if (Players::PvFFA() || Players::PvTVB())
                    BuildOrder::setLearnedBuild(P_1GateCore, P_ZCore, P_3Gate);
                else
                    BuildOrder::setLearnedBuild(P_2Gate, P_10_12, P_Robo);
            }

            // Zerg
            if (Broodwar->self()->getRace() == Races::Zerg) {
                if (Players::ZvZ())
                    BuildOrder::setLearnedBuild(Z_PoolLair, Z_9Pool, Z_1HatchMuta);
                else if (Players::ZvT())
                    BuildOrder::setLearnedBuild(Z_HatchPool, Z_12Hatch, Z_2HatchMuta);
                else if (Players::ZvP())
                    BuildOrder::setLearnedBuild(Z_PoolHatch, Z_Overpool, Z_2HatchMuta);
                else if (Players::ZvFFA() || Players::ZvTVB())
                    BuildOrder::setLearnedBuild(Z_HatchPool, Z_10Hatch, Z_3HatchMuta);
                else
                    BuildOrder::setLearnedBuild(Z_PoolHatch, Z_Overpool, Z_2HatchMuta);
            }

            // Terran
            if (Broodwar->self()->getRace() == Races::Terran) {
                if (Players::TvZ())
                    BuildOrder::setLearnedBuild(T_2Rax, T_11_13, T_Academy);
                else
                    BuildOrder::setLearnedBuild(T_RaxFact, T_2FactFE, T_5Fact);
            }
        }

        void getBestBuild()
        {
            int totalWins  = 0;
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

                Build *currentBuild = nullptr;
                while (!sscopy.eof()) {
                    sscopy >> token;

                    if (token == "Total") {
                        sscopy >> totalWins >> totalLoses;
                        totalGames = totalWins + totalLoses;
                        continue;
                    }

                    // Build winrate
                    auto itr = find_if(myBuilds.begin(), myBuilds.end(), [&](const auto &u) { return u.name == token; });
                    if (itr != myBuilds.end()) {
                        currentBuild = &*itr;
                        sscopy >> itr->w >> itr->l;
                    }
                    else if (currentBuild && currentBuild->getComponent(token))
                        sscopy >> currentBuild->getComponent(token)->w >> currentBuild->getComponent(token)->l;
                }
            };
            auto randomness = max(1, 10 - totalGames);

            const auto calculateUCB = [&](std::string name, int w, int l, bool logCalc = false) {
                auto UCB       = (w + l) > 0 ? (double(w) / double(w + l)) + pow(2.0 * log((double)totalGames) / double(w + l), 0.1) : 2.0;
                auto randomVal = double(rand() % randomness);
                auto finalVal  = (UCB * 25.0) + randomVal + 0.5;
                if (logCalc) {
                    LOG(name.c_str(), " learning value: ", std::fixed, std::setprecision(2), finalVal);
                    LOG("Win/Loss: ", w, "/", l);
                    LOG("UCB1 value: ", std::fixed, std::setprecision(2), UCB);
                    LOG("rand value: ", std::fixed, std::setprecision(2), randomVal);
                }
                return finalVal;
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
                build.ucb1 = calculateUCB(build.name, build.w, build.l);
                for (auto &opener : build.openers)
                    opener.ucb1 = calculateUCB(opener.name, opener.w, opener.l);
                for (auto &transition : build.transitions) {
                    transition.ucb1 = calculateUCB(transition.name, transition.w, transition.l, true);                    
                }

                sort(build.openers.begin(), build.openers.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });
                sort(build.transitions.begin(), build.transitions.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });
            }
            sort(myBuilds.begin(), myBuilds.end(), [&](const auto &left, const auto &right) { return left.ucb1 < right.ucb1; });

            // Pick the best build that is allowed
            auto bestBuildUCB1      = 0.0;
            auto bestOpenerUCB1     = 0.0;
            auto bestTransitionUCB1 = 0.0;
            for (auto &build : myBuilds) {
                if (build.ucb1 < bestBuildUCB1)
                    continue;
                if (!isComponentPossible(build.name))
                    continue;

                bestBuildUCB1      = build.ucb1;
                bestOpenerUCB1     = 0.0;
                bestTransitionUCB1 = 0.0;

                for (auto &opener : build.openers) {
                    if (opener.ucb1 < bestOpenerUCB1)
                        continue;
                    if (!isComponentPossible(opener.name))
                        continue;
                    bestOpenerUCB1     = opener.ucb1;
                    bestTransitionUCB1 = 0.0;

                    for (auto &transition : build.transitions) {
                        if (transition.ucb1 < bestTransitionUCB1)
                            continue;
                        if (!isComponentPossible(transition.name))
                            continue;
                        bestTransitionUCB1 = transition.ucb1;
                        BuildOrder::setLearnedBuild(build.name, opener.name, transition.name);
                    }
                }
            }

            LOG("Book selected: ", BuildOrder::getCurrentBuild(), BuildOrder::getCurrentOpener(), BuildOrder::getCurrentTransition());
        }

        void getPermanentBuild()
        {
            // Testing builds if needed
            if (false) {
                if (Players::PvZ()) {
                    BuildOrder::setLearnedBuild(P_2Gate, P_10_12, P_4Gate);
                    return;
                }
                if (Players::PvP()) {
                    BuildOrder::setLearnedBuild(P_1GateCore, P_ZCore, P_DT);
                    return;
                }
                if (Players::PvT()) {
                    BuildOrder::setLearnedBuild(P_2Base, P_21Nexus, P_Obs);
                    return;
                }
                if (Players::ZvZ()) {
                    BuildOrder::setLearnedBuild(Z_PoolHatch, Z_Overpool, Z_2HatchMuta);
                    return;
                }
                if (Players::ZvT()) {
                    BuildOrder::setLearnedBuild(Z_HatchPool, Z_12Hatch, Z_2HatchMuta);
                    return;
                }
                if (Players::ZvP()) {
                    BuildOrder::setLearnedBuild(Z_PoolHatch, Z_Overpool, Z_4HatchHydra);
                    return;
                }
            }
        }

        void protossBuildMaps()
        {
            Build OneGateCore(P_1GateCore);
            Build TwoGate(P_2Gate);
            Build TwoBase(P_2Base);
            Build FFE(P_FFE);

            // PvT
            if (Players::PvT()) {
                OneGateCore.setOpeners({P_NZCore, P_ZCore});
                OneGateCore.setTransitions({P_DT});

                TwoGate.setOpeners({P_10_12});
                TwoGate.setTransitions({P_DT});

                TwoBase.setOpeners({P_12Nexus, P_20Nexus, P_21Nexus});
                TwoBase.setTransitions({P_Obs, P_Carrier, P_ReaverCarrier});

                myBuilds = {OneGateCore, TwoGate, TwoBase};
            }

            // PvP
            if (Players::PvP()) {
                OneGateCore.setOpeners({P_ZCore});
                OneGateCore.setTransitions({P_DT, P_Robo, P_4Gate, P_3Gate});

                TwoGate.setOpeners({P_10_12});
                TwoGate.setTransitions({P_DT, P_Robo});

                myBuilds = {OneGateCore, TwoGate};
            }

            // PvZ
            if (Players::PvZ()) {
                TwoGate.setOpeners({P_10_12});
                TwoGate.setTransitions({P_4Gate});

                FFE.setOpeners({P_Forge});
                FFE.setTransitions({P_NeoBisu, P_2Stargate, P_5GateGoon});

                myBuilds = {TwoGate, FFE};
            }

            // PvR
            if (Players::PvR()) {
                OneGateCore.setOpeners({P_ZZCore});
                OneGateCore.setTransitions({P_Robo, P_3Gate});

                TwoGate.setOpeners({P_10_12});
                TwoGate.setTransitions({P_Robo});

                myBuilds = {OneGateCore, TwoGate};
            }
        }

        void zergBuildMaps()
        {
            Build PoolHatch(Z_PoolHatch);
            Build HatchPool(Z_HatchPool);
            Build PoolLair(Z_PoolLair);

            if (Players::ZvP()) {
                PoolHatch.setOpeners({Z_Overpool});
                PoolHatch.setTransitions({Z_2HatchMuta, Z_3HatchMuta, Z_3HatchHydra, Z_4HatchHydra, Z_6HatchHydra});

                HatchPool.setOpeners({/*Z_10Hatch,*/ Z_12Hatch});
                HatchPool.setTransitions({Z_2HatchMuta, Z_3HatchMuta, Z_3HatchHydra, Z_4HatchHydra, Z_6HatchHydra});

                myBuilds = {PoolHatch, HatchPool};
            }

            if (Players::ZvT()) {
                PoolHatch.setOpeners({Z_Overpool, Z_12Pool});
                PoolHatch.setTransitions({Z_2HatchMuta, Z_3HatchMuta});

                HatchPool.setOpeners({Z_12Hatch});
                HatchPool.setTransitions({Z_2HatchMuta, Z_3HatchMuta});

                PoolLair.setOpeners({Z_4Pool});
                PoolLair.setTransitions({Z_1HatchLurker});

                myBuilds = {PoolHatch, HatchPool /*, PoolLair*/};
            }

            if (Players::ZvZ()) {
                PoolHatch.setOpeners({Z_12Pool, Z_Overpool});
                PoolHatch.setTransitions({Z_2HatchMuta /*, Z_2HatchHydra*/});

                PoolLair.setOpeners({Z_9Pool, Z_Overpool, Z_Gaspool});
                PoolLair.setTransitions({Z_1HatchMuta});

                myBuilds = {PoolHatch, PoolLair};
            }

            if (Players::ZvR()) {
                PoolHatch.setOpeners({Z_Overpool});
                PoolHatch.setTransitions({Z_2HatchMuta});

                myBuilds = {PoolHatch};
            }
        }

        void terranBuildMaps()
        {
            Build TwoRax(T_2Rax);
            Build RaxFact(T_RaxFact);

            if (Players::TvP()) {
                RaxFact.setOpeners({T_1FactFE, T_2FactFE});
                RaxFact.setTransitions({T_5Fact});

                myBuilds = {RaxFact};
            }

            if (Players::TvT()) {
                RaxFact.setOpeners({T_1FactFE});
                RaxFact.setTransitions({T_5Fact});

                myBuilds = {RaxFact};
            }

            if (Players::TvZ()) {
                TwoRax.setOpeners({T_11_13});
                TwoRax.setTransitions({T_Academy});

                myBuilds = {TwoRax};
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
    } // namespace

    vector<Build> &getBuilds() { return myBuilds; }

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
        gameLog << (isWinner ? "Won" : "Lost") << "," << Util::getMapName() << "," << std::setfill('0') << Util::getTime().minutes << ":" << std::setw(2) << Util::getTime().seconds << ",";

        // What strategies were used/detected
        gameLog << Spy::getEnemyBuild() << "," << Spy::getEnemyOpener() << "," << Spy::getEnemyTransition() << "," << currentBuild << "," << currentOpener << "," << currentTransition << ",";

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

        // File extension including our race initial;
        mapLearning       = false;
        myRaceChar        = {*Broodwar->self()->getRace().c_str()};
        enemyRaceChar     = {*Broodwar->enemy()->getRace().c_str()};
        version           = "BASIL_2026_1";
        noStats           = " 0 0 ";
        learningExtension = myRaceChar + "v" + enemyRaceChar + "_" + Broodwar->enemy()->getName() + "_" + version + " Learning.txt";
        gameInfoExtension = myRaceChar + "v" + enemyRaceChar + "_" + Broodwar->enemy()->getName() + "_" + version + " Info.txt";

        createBuildMaps();
        getDefaultBuild();
        getBestBuild();
        getPermanentBuild();
    }
} // namespace McRave::Learning