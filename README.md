# McRave - A Broodwar Bot
## For any questions, email christianmccrave@gmail.com
## Bot started 01/03/2017, latest readme update 07/26/2017

A Broodwar AI Developed in C++ using Visual Studio Express 2013, BWAPI and BWEM.

**BWEM Differences:**
- Removed all map drawings
- Applied map instance fix (found by jaj22 on SSCAIT)
- Renamed MiniTiles to WalkPosition for consistency

**Information:**
- Stores all useful unit information of enemy, neutral and ally units.
- Stores all Terrain information gathered by BWEM.
- Grids for efficient access of information such as ground distances.

**Production:**
- Follows a general build order for the start of the game.
- After opening build, there are decisions based tech it currently has and enemy composition to decide which tech to get next.

**Micromanagement:**
- Ranged units will kite melee units and other ranged units with lower range or approach closer to units with higher range.
- Melee units will kite melee units if their health is low to try and stay alive.
- Transports can pickup and drop off any units to their targets position.

**Strategy:**
- Every unit has a strength metric based on dps and range.
- A simulation is run for every units theoretical engagement position to see what units will be in range for all or some of the duration of the simulated time.
- If the local simulation determines that the engagement is a win, units will engage with the highest priority targets first.
- If the local simulation determines that the engagement is a loss, units will retreat but contain the enemy outside of any threat area.

**Future Implementations:**
- Moving fogged units instead of removing their known position.
- Improved Shuttle/Arbiter usage.
- Zerg support.
