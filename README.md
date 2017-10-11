# McRave - A Broodwar Bot
## For any questions, email christianmccrave@gmail.com
## Bot started 01/03/2017, latest readme update 10/11/2017

A Broodwar AI Developed in C++ using Visual Studio Express 2013, BWAPI and BWEM.

**New Features:**
- Shuttle usage with Reavers and Templars.
- Opponent modeling for build orders was improved to include multiple builds and default builds for each matchup.

**BWEM Differences:**
- Removed all map drawings
- Applied map instance fix (thanks jaj22)
- Renamed MiniTiles to WalkPosition for consistency

**Information:**
- Stores all useful unit information of enemy, neutral and ally units.
- Stores all Terrain information gathered by BWEM.
- Grids for efficient access of information such as ground distances.

**Production:**
- Uses opponent modeling to learn the best opening build order.
- Tech is decided based on enemy composition.

**Micromanagement:**
- Ranged units will kite melee units and other ranged units with lower range or approach closer to units with higher range.
- Melee units will kite melee units if their health is low to try and stay alive.

**Strategy:**
- Every unit has a strength metric based on dps and range.
- A local combat simulation is run for every units theoretical engagement position to see what units will be in range for all or some of the duration of the simulated time.
- If the local combat simulation determines that the engagement is a win, units will engage with the highest priority targets first. Otherwise, units will retreat.
