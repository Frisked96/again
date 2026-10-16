# World Generation Implementation Stages: 10,000 × 10,000 Medieval Grim Fantasy

> **Vision Alignment (`read.md`)**:  
> *"A single, vast ASCII world birthed whole before you enter... The entire world is generated once at the start of a new game. No chunk-loading, no runtime generation. It’s a living diorama, fully realized from moment zero."*

This document defines the definitive, step-by-step technical architecture for the procedural generation of the world map (10,000 × 10,000 tiles = 100,000,000 cells). Every stage is self-contained, deterministically seeded, and ordered by dependency so each component can be implemented, tested, and verified independently.

---

## Table of Contents

1. [Architectural Overview & Performance Strategy](#1-architectural-overview--performance-strategy)
2. [Stage 1: Continental Macro-Geography, Climate & Drainage](#stage-1-continental-macro-geography-climate--drainage)
3. [Stage 2: Geopolitical Provinces & Natural Demarcation](#stage-2-geopolitical-provinces--natural-demarcation)
4. [Stage 3: Settlement Siting, Hierarchy & Spatial Ranges](#stage-3-settlement-siting-hierarchy--spatial-ranges)
5. [Stage 4: Territorial Sovereignty & Settlement Control Zones](#stage-4-territorial-sovereignty--settlement-control-zones)
6. [Stage 5: Continental Transportation Network (Highways & Bridges)](#stage-5-continental-transportation-network-highways--bridges)
7. [Stage 6: Intra-Settlement Urban Layouts (City Streets & Plazas)](#stage-6-intra-settlement-urban-layouts-city-streets--plazas)
8. [Stage 7: Building Blueprint Randomization & Mutation](#stage-7-building-blueprint-randomization--mutation)
9. [Stage 8: Urban Zoning & Building Placement](#stage-8-urban-zoning--building-placement)
10. [Stage 9: Countryside & Hinterland Generation](#stage-9-countryside--hinterland-generation)
11. [Stage 10: Wildlife & Monster Ecological Simulation](#stage-10-wildlife--monster-ecological-simulation)
12. [Stage 11: World Validation, Integrity & Spawn Selection](#stage-11-world-validation-integrity--spawn-selection)
13. [Pipeline Execution Sequence & Memory Budgets](#13-pipeline-execution-sequence--memory-budgets)

---

## 1. Architectural Overview & Performance Strategy

### The 100-Million Cell Challenge
The world map spans 10,000 × 10,000 tiles. In naive implementations, graph pathfinding, per-tile Voronoi computations, or unbuffered allocations would lead to gigabytes of temporary memory and minutes of freeze time.

To preserve **sub-5-second total generation** across all 11 stages, the system uses a **Two-Tier Hierarchical Generation Model**:

```
+----------------------------------------------------------------------------+
| MACRO TIER (Downsampled Planning Grids: 500x500 to 1000x1000 & Graphs)     |
| - Biome control points & continental plates                               |
| - Settlement placement (Poisson-Disk / Blue Noise)                         |
| - Geopolitical borders & territorial Voronoi cost-distance                |
| - Continental road network graph (MST + Delaunay + A* path routing)       |
| - Wildlife carrying capacity & den locations                              |
+----------------------------------------------------------------------------+
                                     |
                                     v Rasterization & Local Synthesis
+----------------------------------------------------------------------------+
| MICRO TIER (Native 10,000x10,000 Grid: Direct Scanlines & Local Parcels)   |
| - Direct row-pointer memory writes (`tile_row`, `veg_row`, `region_row`)   |
| - Micro-noise terrain dithering & sub-biome substrate assignment           |
| - Street paving, gutter drainage, and plaza paving                        |
| - Building prefab stamping with procedural mutations                       |
| - Strip-field agricultural furrow generation & hedgerow placement         |
+----------------------------------------------------------------------------+
```

### Memory Footprint of the World Grids
- `tile_grid`: 100M × 1 byte (`Tile::ID`) = **100 MB**
- `region_grid`: 100M × 1 byte (`World::RegionID`) = **100 MB**
- `vegetation_grid`: 100M × 2 bytes (`Vegetation::Cell`) = **200 MB**
- `surface_structure_grid`: 100M × 2 bytes (`Structure::Cell`) = **200 MB**
- `SparseVerticalStorage` (Z ≠ 0): Chunks of 32 × 32 allocated only where upper stories or cellars exist = **~10–25 MB**
- **Total Persistent Generation Footprint**: **~625 MB RAM** (constant, preallocated).

---

## Stage 1: Continental Macro-Geography, Climate & Drainage

### Objective
Establish the physical foundation of the world: continental landmasses, mountain chains, temperature and moisture gradients, oceanic margins, and continuous river drainage basins.

### Inputs & Dependencies
- Input: Global 32-bit `seed`.
- Pre-allocated `Map` instance (10,000 × 10,000).

### Mathematical Model & Algorithms
1. **Continental Heightfield Synthesis**:
   - 32 × 32 macro-control points interpolated across the 10,000 × 10,000 grid using cubic smoothstep interpolation.
   - Continental landmass shaping: central highland ridge, mountain spine running diagonal, low coastal shelves on west/south.
2. **Whittaker Climate Space**:
   - Continuous latitude temperature gradient (colder north, hotter south) combined with elevation thermal lapse rate:
     $$T_{\text{effective}} = T_{\text{macro}} - (\text{elevation} \times \alpha_{\text{lapse}}) + \text{basin\_heat}$$
   - Moisture field influenced by oceanic proximity and orographic precipitation (rain shadow behind mountain spines).
3. **Hydraulic Drainage Network**:
   - Locate high-elevation precipitation accumulation nodes.
   - Trace flow vectors downhill using steepest descent downhill with local depression filling (A* breach filling for mountain passes).
   - Merge converging rivulets into main river arteries (width 2–4 tiles) carving valleys into coastlines.

### Concrete Data Structures
```cpp
struct RiverNode {
  int x;
  int y;
  float flow_volume;
  int width; // 1 = brook, 2 = stream, 3-4 = major river
};

struct RiverArtery {
  uint32_t id;
  std::vector<RiverNode> path;
  bool reaches_ocean{false};
};
```

### Outputs & State Updates
- Writes `Tile::ID` into `map.tile_row(y)` (DeepWater, CoastalWater, MountainPeak, Cliff, Foothills, RockyGround, etc.).
- Writes `World::RegionID` into `map.region_row(y)` across the 10 Whittaker macro-biomes.
- Carves `Tile::ID::River` and identifies river fords (`Tile::ID::RiverFord`).

### Unit Testing & Verification Criteria
- [ ] No unhandled sinkholes or rivers that terminate abruptly in mid-air.
- [ ] Minimum 60% walkable continental landmass; coastlines smooth and free of salt-and-pepper noise.
- [ ] Temperature decreases predictably with elevation and northern latitude.

---

## Stage 2: Geopolitical Provinces & Natural Demarcation

### Objective
Carve the vast 10,000 × 10,000 wilderness into 8–16 distinct, named **Provinces/Baronies** (e.g., *"The Velen Marches"*, *"Barony of Westervale"*, *"The Oldwood Weald"*, *"Redwater Fens"*). Each province has distinct cultural architecture, tax rates, military presence, and danger level.

### Inputs & Dependencies
- Stage 1 completed (`tile_grid`, `region_grid`, and elevation map).

### Mathematical Model & Algorithms
1. **Seed Provincial Capital Nodes**:
   - Choose 8–16 macro points in fertile river basins, coastal bays, or strategic plains.
2. **Cost-Distance Watershed Partitioning**:
   - Compute weighted Voronoi partitions using Dijkstra / Jump Flood on a 500 × 500 macro grid.
   - Traversal cost is multiplied by 10× for sheer cliffs/mountains and 5× for wide rivers, forcing provincial borders to naturally align with mountain ridges and river channels.
3. **Border Smoothing & Identity Tagging**:
   - Cellular automata pass (smoothing jagged cell edges).
   - Assign cultural identity, economic wealth, and lawlessness ratings.

```
+-------------------------------------------------------+
|  [Province 1: Frost Tundra]   |   [Province 2: Taiga]  |
|  Lawless, Clan-ruled          |   Barony of Ironhold   |
|~~~~~~~~ Mountain Spine ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
|  [Province 3: The Weald]     |   [Province 4: Lowlands]|
|  Ancient Forest Realm        |   Ducal Wheatland       |
+-------------------------------------------------------+
```

### Concrete Data Structures
```cpp
namespace World {

struct Province {
  uint8_t id;
  std::string name;
  std::string sovereign_title; // "Duchy", "Barony", "March", "Free League"
  float wealth_index;          // 0.2 (starving peasant fen) - 1.0 (wealthy ducal capital)
  float lawlessness;           // 0.0 (heavily patrolled) - 1.0 (bandit-infested)
  World::RegionID dominant_biome;
  int capital_x;
  int capital_y;
};

} // namespace World
```

### Outputs & State Updates
- Global registry of `std::vector<Province> provinces`.
- Macro province lookup texture (500 × 500) for fast spatial queries.

### Unit Testing & Verification Criteria
- [ ] Every province has at least 1 viable urban center location.
- [ ] Provincial borders do not unnaturally cross over unscalable mountain peaks without a pass.

---

## Stage 3: Settlement Siting, Hierarchy & Spatial Ranges

### Objective
Position all settlements across the continent according to realistic medieval socio-economic hierarchies and spatial range constraints.

### Settlement Tiers & Constraints
| Tier | Settlement Classification | Count per Continent | Population / Buildings | Min Distance to Same Tier | Min Distance to Any Tier | Placement Criteria |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Tier 1** | **Capital / Free City** | 2–4 | 120–250 | 2500 tiles | 600 tiles | Deep harbor, wide river confluence, fertile plain |
| **Tier 2** | **Major Town / Barony Seat** | 8–16 | 45–90 | 1200 tiles | 350 tiles | Highway crossroads, river crossing, foothill mineral |
| **Tier 3** | **Agrarian Village** | 40–80 | 15–35 | 400 tiles | 150 tiles | Arable farmland, forest edge, lake shore |
| **Tier 4** | **Hamlet / Outpost / Fort** | 100–200 | 3–10 | 150 tiles | 60 tiles | Natural resources (quarries, mills, watchtowers) |

### Mathematical Model & Algorithms
1. **Terrain Suitability Scoring**:
   For each candidate location $(x, y)$, calculate:
   $$S(x, y) = W_{\text{flat}} \cdot (1.0 - \text{slope}) + W_{\text{water}} \cdot \text{prox}(H_2O) + W_{\text{fert}} \cdot \text{soil} - W_{\text{hazard}} \cdot \text{danger}$$
   Disqualify any tile on `DeepWater`, `MountainPeak`, `Cliff`, or `GlacialCrown`.
2. **Hierarchical Blue Noise / Poisson-Disk Sampling**:
   - First pass: Place Tier 1 Capitals using candidate elimination within radius $R_1 = 2500$.
   - Second pass: Place Tier 2 Towns ensuring $R_2 \ge 1200$ from other towns and $R \ge 600$ from capitals.
   - Third pass: Place Tier 3 Villages clustered inside provincial fertile zones ($R_3 \ge 400$).
   - Fourth pass: Place Tier 4 Hamlets near resource nodes and frontier passes ($R_4 \ge 150$).

### Concrete Data Structures
```cpp
namespace World {

enum class SettlementTier : uint8_t {
  CapitalCity = 1,
  MajorTown = 2,
  Village = 3,
  Hamlet = 4
};

struct Settlement {
  uint32_t id;
  std::string name;
  SettlementTier tier;
  uint8_t province_id;
  int center_x;
  int center_y;
  int radius;                  // Urban footprint radius in tiles
  int control_zone_radius;     // Sovereign hinterland radius in tiles
  float wealth;
  std::vector<std::string> dominant_trades; // "farming", "fishing", "blacksmithing", "timber"
};

} // namespace World
```

### Outputs & State Updates
- `std::vector<Settlement> settlements` stored in engine world metadata.
- Settlement centers cataloged for subsequent road routing and territorial zoning.

### Unit Testing & Verification Criteria
- [ ] No two settlements violate minimum spatial distance rules.
- [ ] All Tier 1 and Tier 2 settlements possess access to fresh water or sea coast.
- [ ] Settlement centers are strictly on walkable, stable terrain.

---

## Stage 4: Territorial Sovereignty & Settlement Control Zones

### Objective
Mathematically answer: **"What area comes under which city?"**  
Define jurisdictional boundaries, taxation areas, patrol security zones, and lawless wilderness fringes.

### Territorial Zoning Hierarchy
Every settlement projects an influence sphere broken into 4 distinct concentric control zones:

```
+---------------------------------------------------------------------------------+
|  CONTESTED FRONTIER / WILD WILDERNESS (Beyond all city control)                 |
|  - Lawless, monsters hunt freely, bandit encampments, ancient barrows          |
|                                                                                 |
|      +-------------------------------------------------------------------+      |
|      |  AGRARIAN HINTERLAND & LORD'S FOREST (City Control Zone)         |      |
|      |  - Tenant strip fields, pastures, watermills, roadside shrines    |      |
|      |  - Poaching illegal, guarded by rural bailiffs                   |      |
|      |                                                                   |      |
|      |      +-----------------------------------------------------+      |      |
|      |      |  SUBURBAN / EXTRA-MURAL PERIPHERY                   |      |      |
|      |      |  - Tanneries, slaughterhouses, slums, coaching inns |      |      |
|      |      |  - Gallows hills at city gates                      |      |      |
|      |      |                                                     |      |      |
|      |      |      +-------------------------------+              |      |      |
|      |      |      |  INTRA-MURAL URBAN CORE       |              |      |      |
|      |      |      |  - Walled, paved cobblestone  |              |      |      |
|      |      |      |  - Heavily guarded, curfew    |              |      |      |
|      |      |      +-------------------------------+              |      |      |
|      |      +-----------------------------------------------------+      |      |
|      +-------------------------------------------------------------------+      |
+---------------------------------------------------------------------------------+
```

### Mathematical Model & Algorithms
1. **Weighted Geodesic Cost-Distance Transform**:
   - Executed on a 1000 × 1000 macro-grid (1 cell = 10 × 10 world tiles).
   - Seed each cell with its closest settlement, weighted by settlement tier power:
     $$\text{Reach}(S) = \text{BaseRadius}(\text{Tier}) \times (1.0 + \text{Wealth} \times 0.5)$$
   - Propagation cost through cells:
     - Along roads: $\text{Cost} = 0.2$
     - Flat grassland/meadow: $\text{Cost} = 1.0$
     - Forest/Fen: $\text{Cost} = 2.5$
     - River: $\text{Cost} = 5.0$ (unless bridged)
     - Cliff/Peak: $\text{Cost} = \infty$
2. **Jurisdiction Tagging**:
   - For any world tile $(x, y)$, query closest controlling settlement ID and distance:
     - $d < R_{\text{urban}}$: **Zone 1: Urban Core**
     - $R_{\text{urban}} \le d < R_{\text{suburb}}$: **Zone 2: Extra-Mural Periphery**
     - $R_{\text{suburb}} \le d < R_{\text{control}}$: **Zone 3: Agrarian Hinterland**
     - $d \ge R_{\text{control}}$: **Zone 4: Contested Frontier / Wild Wilderness**
3. **Physical Border Landmarks**:
   - At intersection points where roads cross from Zone 4 into Zone 3: place **Boundary Stones** or **Carved Mileposts**.
   - At intersections between two competing Tier 1/2 control zones: place **Fortified Border Guardposts** and **Toll Gates**.

### Concrete Data Structures
```cpp
namespace World {

enum class ControlZoneType : uint8_t {
  UrbanCore = 0,
  ExtraMuralPeriphery,
  AgrarianHinterland,
  ContestedFrontier
};

struct TileControlData {
  uint32_t settlement_id{0};
  uint8_t province_id{0};
  ControlZoneType zone_type{ControlZoneType::ContestedFrontier};
  uint16_t distance_to_center{0};
};

} // namespace World
```

### Outputs & State Updates
- Downsampled control zone lookup map.
- Fast evaluation API: `World::TileControlData get_control_zone(int x, int y)`.

### Unit Testing & Verification Criteria
- [ ] No disconnected "islands" of city sovereignty across uncrossable mountain barriers.
- [ ] Frontier lands exist between distant cities where lawlessness is high.

---

## Stage 5: Continental Transportation Network (Highways & Bridges)

### Objective
Weave the continent together with an organic, topographically natural road network connecting capitals, towns, and villages with paved highways, cart roads, and mountain passes.

### Road Hierarchy
1. **Imperial Highways (Paved Cobblestone)**:
   - Width: 3–4 tiles.
   - Pavement: `Tile::ID::Cobblestone`. Flanked by dirt ditches.
   - Connects: All Tier 1 Capitals and Tier 2 Major Towns.
2. **Merchant Cart Roads (Packed Dirt)**:
   - Width: 2 tiles.
   - Pavement: `Tile::ID::DirtRoad`.
   - Connects: Tier 2 Towns to Tier 3 Agrarian Villages.
3. **Rural Bridleways & Paths (Worn Trail)**:
   - Width: 1 tile.
   - Pavement: `Tile::ID::DirtRoad` or dithered path.
   - Connects: Villages to Tier 4 Hamlets, outlying mills, and quarries.

### Mathematical Model & Algorithms
1. **Graph Topology Generation**:
   - Compute **Delaunay Triangulation** on all settlement center vertices.
   - Extract the **Euclidean Minimum Spanning Tree (EMST)** to guarantee 100% connectivity with no isolated settlements.
   - Re-introduce 20–30% of the non-tree Delaunay edges to create realistic loops and alternative merchant trade routes.
2. **Topographic Cost-Surface A* Pathfinding**:
   - Route edges across the continent using hierarchical A* on a 1000 × 1000 cost grid:
     $$C_{\text{traversal}} = \Delta h \times 15.0 + C_{\text{substrate}} + C_{\text{water}}$$
   - Slopes $> 15^\circ$ penalized heavily; sheer cliffs impassable unless a natural col/saddle exists.
   - Following river valleys has negative cost bias (natural travel corridor).
3. **Bridges & Ford Construction**:
   - When a road path intersects a `Tile::ID::River`:
     - If width $\le 2$ tiles: stamp `Tile::ID::RiverFord` or wooden bridge.
     - If width $\ge 3$ tiles: stamp stone pier bridge (`Tile::ID::Cobblestone` flanked by `Structure::ID::StoneWall` parapets).
4. **Rasterization & Micro-Meander**:
   - Trace path vertices down to the native 10,000 × 10,000 grid using Bresenham lines with small sinusoidal Perlin meander to avoid unnaturally straight lines.
   - Clear existing trees (`Vegetation::ID::None`) within the road corridor.

```
       [Capital City A]
              ||
     ======== Bridge ======== (Major River)
              ||
       [Highway Fork] 
        //          \\
 [Town B]            [Town C]
    |                   |
 (Cart Road)         (Cart Road)
    |                   |
[Village D]         [Village E]
```

### Outputs & State Updates
- Modifies `tile_grid` rows with `Cobblestone`, `DirtRoad`, `RiverFord`.
- Clears conflicting vegetation from `vegetation_grid`.
- Catalogs all road coordinate corridors for coaching inn placement and urban road tying.

### Unit Testing & Verification Criteria
- [ ] Every settlement is reachable from every other settlement via the road graph.
- [ ] No road segment climbs sheer cliffs without a traversable grade.
- [ ] Bridges are generated over all river intersections along highway paths.

---

## Stage 6: Intra-Settlement Urban Layouts (City Streets & Plazas)

### Objective
Generate authentic, functional medieval street networks inside settlement boundaries rather than uniform square grids.

### Urban Street Typologies
- **Capitals & Major Towns (Radial-Concentric & Walled)**:
  - Central open **Market Plaza / Cathedral Square** (12 × 12 to 24 × 24).
  - Primary **Arterial Avenues** (width 3) connecting the central plaza directly to the city gatehouses where continental highways enter.
  - Secondary **Concentric Ring Alleys** (width 2) tracing along the inner curtain walls.
  - Narrow **Pedestrian Lanes & Cul-de-Sacs** (width 1) branching into residential blocks.
- **Villages & Hamlets (Ribbon & Green Settlements)**:
  - Linear road flanked by wide verges ("Village Green").
  - Central communal well / watering trough.
  - Loose branching paths leading out into peasant field lanes.

```
      +-------------+-------------+
      |      |   Arterial   |     |
      | Alleys   Avenue     Alleys|
      |      |      |       |     |
      |------+--[PLAZA]-----+-----|
      |      |  Market, |   |     |
      | Alleys  Well,   |  Alleys |
      |      |  Gallows |   |     |
      +-------------+-------------+
```

### Mathematical Model & Algorithms
1. **Gatehouse Anchor Points**:
   - Detect where external continental highways penetrate the settlement radius.
   - Place fortified Gatehouses (`Structure::ID::IronBars` portcullis + `StoneWall` towers).
2. **Town Center Carving**:
   - Clear an open plaza space in the geographic center of the urban radius.
   - Pave plaza with `Tile::ID::Cobblestone`.
   - Place central landmark: civic well, town pillory/gallows, or market stalls.
3. **Organic Arterial Tracing**:
   - Connect each Gatehouse to the central plaza using Bresenham lines with slight organic jitter.
4. **Lot Parceling (Subdivision along Street Frontage)**:
   - Identify unpaved blocks between streets.
   - Subdivide blocks into buildable rectangular lots (parcels) ranging from 6 × 6 (hovel) to 16 × 16 (manor/tavern).
   - Guarantee that every parcel has at least one edge bordering a street tile.

### Concrete Data Structures
```cpp
namespace Architecture {

struct UrbanLot {
  int x;
  int y;
  int width;
  int height;
  int street_facing_x;
  int street_facing_y;
  int door_facing_dir; // 0 = North, 1 = East, 2 = South, 3 = West
  std::string district_zone; // "commercial", "civic", "craft", "residential"
};

} // namespace Architecture
```

### Outputs & State Updates
- Paves internal streets and plazas on `tile_grid`.
- Produces `std::vector<UrbanLot>` for Stage 8 building stamping.

### Unit Testing & Verification Criteria
- [ ] All parcels possess clear pedestrian access to a street.
- [ ] Highway entrances connect seamlessly to the central plaza without dead ends.

---

## Stage 7: Building Blueprint Randomization & Mutation

### Objective
Ensure no two buildings in the world look identical. Take handcrafted base blueprints from [`buildings.json`](file:///home/arvind/Documents/again/data/prefabs/buildings.json) and procedurally generate infinite variations using a **Blueprint Perturbation Grammar**.

### Mutation Techniques

#### 1. Dimensional Jitter & Annex Attachments
- **Base Footprint Mutation**: Expand or shrink rooms by 1–2 tiles along minor axes.
- **Annex Extrusion**:
  - Attach an exterior lean-to shed (wood storage, pigpen, tool stall) along the exterior blank wall.
  - Transform a rectangular cottage into an L-shaped or T-shaped floorplan.

```
Base Cottage (8x8):       Mutated Cottage with Stables Annex (12x8):
  ########                  ##############
  #......#                  #......##....#  <- Attached stable
  #......#                  #......##....#     or firewood shed
  #...####                  #...#####....#
  #...+..#                  #...+..#######
  #......#                  #......#
  #......#                  #......#
  ###+####                  ###+####
```

#### 2. Biome & Wealth Material Palette Swapping
Dynamically translate structural IDs based on local settlement wealth and regional climate:

| Settlement Wealth / Biome | Wall Material | Floor Material | Roof / Feature Style |
| :--- | :--- | :--- | :--- |
| **High Wealth (City Core / Manor)** | `Structure::ID::StoneWall` | `Structure::ID::StoneFloor` | Glass windows, slate roofs, iron bars |
| **Moderate Wealth (Towns / Trade)** | Half-Timbered (Stone base, Wood upper) | `Structure::ID::WoodFloor` | Glass/shutter windows, wood doors |
| **Peasant / Rural (Villages)** | `Structure::ID::WoodWall` | `Structure::ID::WoodFloor` or dirt | Simple wood shutters, thatch |
| **Impoverished / Frontier (Hovels)** | `Structure::ID::Palisade` | Compacted dirt (`Tile::ID::HardenedClay`) | Open apertures, cloth partition |
| **Arid Waste / Steppes** | Sun-baked Clay Brick (`StoneWall`) | `Tile::ID::HardenedClay` | Flat roofs, narrow slits |
| **Boreal Taiga / Highlands** | Heavy Pine Log (`WoodWall`) | Split timber planks | Central open smoke hearth |

#### 3. Procedural Interior Prop & Furniture Placement
Using [`RoomDetector`](file:///home/arvind/Documents/again/include/building_prefab.hpp#L96), identify enclosed spaces and populate contextual props:
- **Tavern Taprooms**: Procedural bar counters (`Structure::ID::Counter`), beer barrels, benches.
- **Smithies**: Central anvil (`Structure::ID::Anvil`), stone hearth forge against back wall.
- **Living Quarters**: Straw pallets/beds in corners away from doorways, hearths on outside stone walls.
- **Cellars ($Z = -1$)**: Wine casks, storage crates, spiderwebs, stone pillars for load support.

#### 4. Dilapidation, Ruin & Grim Fantasy Decay
Based on province `lawlessness` or village status (e.g. abandoned, plague-struck, burnt):
- **Structural Collapse**: Replace 10–30% of wall segments with `Structure::ID::Rubble`.
- **Decay Scars**: Replace floor tiles with `Structure::ID::WoodScraps` or dirt patches.
- **Doorway States**: Leave doors broken off hinges (`DoorOpen` or missing entirely).
- **Grim Storytelling**: Splashes of dried blood, barricaded doorways (`Structure::ID::Palisade`), charred timbers.

### Concrete Blueprint Generator Interface
```cpp
namespace Architecture {

struct MutationParameters {
  uint32_t seed;
  float wealth{0.5f};
  float dilapidation{0.0f}; // 0.0 = pristine, 1.0 = ruined shell
  World::RegionID biome{World::RegionID::LowlandMeadow};
  bool allow_annex{true};
  int forced_door_side{-1};  // 0=N, 1=E, 2=S, 3=W
};

class BlueprintMutator {
public:
  static BuildingTemplate mutate(
      const BuildingTemplate &base_template,
      const MutationParameters &params);
};

} // namespace Architecture
```

### Unit Testing & Verification Criteria
- [ ] Mutated blueprints maintain valid outer enclosures with no accidental unsealed exterior gaps.
- [ ] Staircases between Z-levels align vertically with 100% precision ($Z=0$ stairs down directly aligns over $Z=-1$ stairs up).
- [ ] Dilapidation does not break pathfinding to critical narrative objects.

---

## Stage 8: Urban Zoning & Building Placement

### Objective
Populate urban lots generated in Stage 6 with appropriately mutated building blueprints from Stage 7, ensuring proper district zoning, setbacks, door orientations, and multi-level vertical stamping.

### District Zoning Rules
1. **Civic / Religious Core**:
   - Location: Facing the central Market Plaza.
   - Allowed Archetypes: Cathedral/Chapel, Town Hall, Guildhall, Manor House, Bank.
2. **Commercial Thoroughfare**:
   - Location: Along arterial avenues within 150 tiles of the plaza.
   - Allowed Archetypes: Coaching Inns, Taverns, Apothecaries, General Shops, Bakeries.
3. **Craftsmen & Industrial Perimeter**:
   - Location: Downwind or adjacent to river channels (for water power and waste disposal).
   - Allowed Archetypes: Blacksmithies, Tanneries, Breweries, Mills, Warehouses.
4. **Residential Wards**:
   - Location: Along secondary lanes and cul-de-sacs.
   - Allowed Archetypes: Multi-story Townhouses (wealthy quarters), Cottages, Peasant Hovels (slums).
5. **Fortifications & Military**:
   - Location: Along urban perimeter and gate crossings.
   - Allowed Archetypes: Barracks, Guardposts, Watchtowers, Palisade/Stone Walls.

### Placement & Stamping Algorithm
For each lot in `UrbanLot`:
1. Select appropriate template archetype based on the lot's `district_zone`.
2. Generate mutated variant using `BlueprintMutator::mutate` sized to fit lot bounds.
3. **Rotation Alignment**:
   - Rotate building template by 0°, 90°, 180°, or 270° so the primary exterior entrance door directly faces the lot's `street_facing_x, street_facing_y`.
4. **Clearance & Leveling**:
   - Level terrain under building footprint to flat solid substrate (`Tile::ID::Grassland`, `Tile::ID::Cobblestone`, or `Tile::ID::HardenedClay`).
   - Remove all natural vegetation.
5. **Multi-Level Stamping**:
   - Call [`Map::stamp_building`](file:///home/arvind/Documents/again/include/game_map.hpp#L144).
   - Ground floor written to `surface_structure_grid` ($Z = 0$).
   - Upper floors ($Z = 1, 2$) and Cellars ($Z = -1$) written to [`SparseVerticalStorage`](file:///home/arvind/Documents/again/include/game_map.hpp#L38).
   - Pave entrance threshold walk connecting door to the street.

### Outputs & State Updates
- Modifies `surface_structure_grid` and `vertical_structures`.
- Updates `tile_grid` for floor substrates and perimeter walkways.

### Unit Testing & Verification Criteria
- [ ] No building overlaps another building or intrudes onto a paved street tile.
- [ ] Every building's front door is completely unobstructed and opens directly onto an access path.
- [ ] Multi-story buildings possess adequate structural support (no floating stone walls on upper floors).

---

## Stage 9: Countryside & Hinterland Generation

### Objective
Transform the agricultural hinterland (Control Zone 3) surrounding settlements into a living, productive medieval countryside. In authentic medieval societies, 85–90% of land was dedicated to food and resource production.

### Components of the Countryside

#### 1. Open-Field Medieval Strip Farming
- **Structure**: Long, narrow rectangular plots (e.g., 40 × 8 to 100 × 16 tiles) arranged in parallel furlongs.
- **Substrate**: `Tile::ID::Farmland` furrowed rows.
- **Crop Variety**: Alternating strips of:
  - Ripe Grain (`Vegetation::ID::WildCrops` yield = 100%)
  - Young Green Spelt / Barley
  - Fallow Pasture (grazing clover)
- **Field Boundaries**: Narrow drainage swales, stone hedgerows, or split-rail wooden fences (`Structure::ID::Palisade`).

```
============================================ (Cart Track)
[ Furlong Strip A: Wheat Field             ] (Furrowed Farmland)
............................................ (Drainage Ditch)
[ Furlong Strip B: Barley Field            ] (Furrowed Farmland)
............................................ (Stone Hedgerow)
[ Furlong Strip C: Fallow Grazing Land     ] (Grassland + Clover)
============================================ (Field Lane)
```

#### 2. Livestock Pastures & Enclosures
- Irregular, fenced enclosures for cattle, sheep, and pigs.
- Wooden gate transitions (`Structure::ID::DoorClosed`).
- Scattered hayricks, water troughs, and rustic open-sided animal sheds.

#### 3. Outlying Agro-Industrial Nodes
- **Watermills**: Placed on riverbanks where slope provides fast current, connected by mill-race canals.
- **Windmills**: Positioned on exposed, breezy knolls and hilltops overlooking the fields.
- **Woodcutter Camps & Charcoal Burners**: Situated at the boundary where farmland meets dense deciduous/boreal forest.
- **Stone Quarries**: Situated at exposed foothill crags and cliff bases (`Tile::ID::RockyGround`).

#### 4. Grim Fantasy Wayside Features
- **Wayside Coaching Inns**: Placed at highway crossroads every 1000–1500 tiles (a full day's travel for a packhorse).
- **Gallows Hills & Gibbets**: Erected at settlement borders and high crossroads, displaying executed bandits as warnings.
- **Roadside Shrines & Wayside Crosses**: Small 3 × 3 stone shrines offering shelter or blessings to weary travelers.
- **Barrows & Megalithic Ruins**: Ancient pre-human burial mounds, ringed by standing stones, overgrown by brambles.
- **Desolated Outsteads**: Burnt peasant cabins, destroyed by monsters or raiders, with overgrown weed fields.

### Outputs & State Updates
- Writes farmland substrates and crop vegetation to `tile_grid` and `vegetation_grid`.
- Stamps outbuildings, mills, shrines, and coaching inns across the countryside.

### Unit Testing & Verification Criteria
- [ ] Farmland strips do not generate on steep mountain slopes or barren sand dunes.
- [ ] Watermills are strictly adjacent to flowing river tiles.
- [ ] Coaching inns are always generated within 5 tiles of an Imperial Highway.

---

## Stage 10: Wildlife & Monster Ecological Simulation

### Objective
Populate the world's wilderness and countryside with a living, self-balancing food web of animals and grim Witcher-inspired monsters that pose daily survival threats.

### Ecological Tiers & Fauna

#### Tier 1: Herbivores & Game Herds (Prey Base)
- **Species**: Red Deer, Roe Buck, Wild Boars, Mountain Hares, Highland Ibex.
- **Habitats**: Lowland Meadows, Deciduous Wealds, Steppes, Alpine Foothills.
- **Spawning Rules**: Spawn in family groups (herds of 4–12).
- **Behavior & Yields**: Grazing routines; flee on sight of player/predator; yield hides, venison, bone, sinew.

#### Tier 2: Wilderness Predators & Scavengers
- **Species**: Timber Wolf Packs, Cave Bears, Lynx, Red Foxes, Carrion Crows, Wild Dogs.
- **Habitats**: Deep Boreal Taiga, Oldwood Wealds, Rocky Crags, Peat Fens.
- **Dens & Lairs**: Do not spawn randomly in thin air; emanate from specific geographic nodes (Wolf Dens in hollows, Bear Caves in cliffs).
- **Settlement Avoidance**: Actively avoid Zone 1 (Urban Core) and Zone 2 (Suburbs); hunt in Zone 3 (Hinterlands) and Zone 4 (Frontiers).

#### Tier 3: Grim Fantasy Monsters (The Witcher Daily Threats)
Monsters are dangerous, predatory, and rooted in the grim folklore of the world:

| Monster Type | Native Biome & Terrain | Lair Anchor / Spawn Site | Threat Level & Ecological Role |
| :--- | :--- | :--- | :--- |
| **Leshen / Wood Wight** | Ancient Deciduous Oldwood | Ancient Hollow Oak / Totem Circle | Apex forest guardian; summons wolf packs and crows |
| **Drowner / Water Hag** | Peat Fens, Marsh Water, Riverbanks | Sunken river skiffs, quaking bogs | Ambush aquatic predators; feed on corpses and unwary travelers |
| **Griffin / Wyvern** | Highland Peaks, Alpine Crags | Crag eyrie nests built from bones | Aerial apex predator; hunts game herds across foothills |
| **Ghoul / Rotfiend** | Gallows Hills, Plague Ruins, Battlefields | Fresh grave mounds, charnel pits | Scavengers of human tragedy; attack in ravenous packs |
| **Ice Troll / Frost Revenant** | Glacial Crown, Frost Tundra | Glacier ice caves, blizzard crests | Territorial brutes; defend high mountain passes |

```
+-------------------------------------------------------------+
| ECOSYSTEM DENSITY HEATMAP                                    |
| [City Center]  -> Monster Density = 0.0 (Suppressed by Guards) |
| [Farmland]     -> Wildlife = Pests & Domestic (Boars, Crows)|
| [Forest Edge]  -> Game Herds & Wolves (Hunting Grounds)     |
| [Deep Fen/Crag]-> Apex Monsters & Ancient Terrors (Lairs)   |
+-------------------------------------------------------------+
```

### Spawning Algorithm & Data Structures
1. **Carrying Capacity Calculation**:
   Compute suitability score based on vegetation cover, water availability, and human settlement proximity.
2. **Den & Lair Anchor Placement**:
   Place physical den structures (`Tile::ID::SubterraneanRock` hollows or bone nests) in Zone 4 frontiers and deep biomes.
3. **Cohesive Pack Spawning**:
   Entities are placed in clusters around their den anchor with designated territory radii.

```cpp
namespace Ecology {

enum class CreatureType : uint16_t {
  // Herbivores
  Hare = 1,
  Deer,
  WildBoar,
  HighlandGoat,

  // Predators
  Wolf,
  Bear,
  Lynx,

  // Grim Monsters
  Drowner,
  WaterHag,
  Leshen,
  Griffin,
  Ghoul,
  IceTroll
};

struct LairNode {
  uint32_t id;
  CreatureType resident_type;
  int x;
  int y;
  int territory_radius;
  uint8_t pack_size;
  bool is_active;
};

} // namespace Ecology
```

### Outputs & State Updates
- Registry of active creature lairs and roaming wildlife packs.
- Initial entity placements ready for Engine turn simulation.

### Unit Testing & Verification Criteria
- [ ] No apex monsters spawn inside walled city cores (Zone 1).
- [ ] Every predator pack is tethered to a valid, reachable lair anchor.
- [ ] Prey animals outnumber apex predators by a minimum ratio of 6:1.

---

## Stage 11: World Validation, Integrity & Spawn Selection

### Objective
Run comprehensive topological and physical integrity audits over the fully generated 10,000 × 10,000 world, ensure zero game-breaking generation flaws, and select an evocative, immersive player starting spawn.

### Integrity Validation Suite
1. **Continental Highway Reachability**:
   - Flood-fill from Capital 1 along road corridors. Assert 100% of settlements have at least one traversable road path to the capital.
2. **Structural Load & Gravity Check**:
   - Scan all multi-level structures. Assert that no upper-story stone wall ($Z = 1$) rests on open air without a supporting ground-floor wall ($Z = 0$).
3. **Portal & Door Traversability**:
   - Verify every stamped building has at least one working door that opens into a walkable tile.
4. **Seed Determinism Verification**:
   - Re-running the generation pipeline with Seed $S$ must produce identical checksums across `tile_grid`, `structure_grid`, and `region_grid`.

### Atmospheric Player Spawn Selection
To honor the **Grim Freedom Sandbox** pillar:
- Avoid dropping the player into a blank grassy field or an overwhelming capital throne room.
- **Ideal Starting Spawn**: The common room threshold of a **Wayside Coaching Inn** along a temperate Imperial Highway, or a humble **Peasant Hamlet** bordering the Oldwood Weald.
- The player steps out immediately into an active, living diorama: horses tethered at the post, smoke rising from the chimney, distant farmlands on one side, and the brooding wilderness on the other.

---

## 13. Pipeline Execution Sequence & Memory Budgets

### Sequential Stage Pipeline

```
[Start Seed]
    │
    ▼ (1.3s)
Stage 1: Continental Geography, Climate & Drainage
    │
    ▼ (0.2s)
Stage 2: Geopolitical Provinces & Natural Demarcation
    │
    ▼ (0.3s)
Stage 3: Settlement Siting, Hierarchy & Spatial Ranges
    │
    ▼ (0.4s)
Stage 4: Territorial Sovereignty & Settlement Control Zones
    │
    ▼ (0.8s)
Stage 5: Continental Transportation Network (Highways & Bridges)
    │
    ▼ (0.5s)
Stage 6: Intra-Settlement Urban Layouts (City Streets & Plazas)
    │
    ▼ (0.3s)
Stage 7: Building Blueprint Randomization & Mutation
    │
    ▼ (0.4s)
Stage 8: Urban Zoning & Building Placement
    │
    ▼ (0.5s)
Stage 9: Countryside & Hinterland Generation
    │
    ▼ (0.3s)
Stage 10: Wildlife & Monster Ecological Simulation
    │
    ▼ (0.2s)
Stage 11: World Validation, Integrity & Spawn Selection
    │
    ▼
[World Ready: Total Target Execution Time < 5.5 Seconds]
```

### Memory & Performance Budgets

| Stage | Memory Allocation | Algorithm Time Complexity | Target Time Budget |
| :--- | :--- | :--- | :--- |
| **Stage 1: Geography & Climate** | 400 MB (preallocated grids) | $O(N)$ scanlines ($100\text{M cells}$) | **1.35 s** |
| **Stage 2: Provinces** | 1 MB (macro grid 500 × 500) | $O(M \log M)$ Voronoi | **0.20 s** |
| **Stage 3: Settlements** | < 1 MB (vector of ~250 sites) | $O(K \log K)$ Poisson-Disk | **0.30 s** |
| **Stage 4: Control Zones** | 4 MB (macro grid 1000 × 1000) | $O(M)$ Jump Flood Algorithm | **0.40 s** |
| **Stage 5: Road Network** | 8 MB (path nodes & A* heap) | $O(E \log V)$ Hierarchical A* | **0.80 s** |
| **Stage 6: Urban Layouts** | 2 MB (lot allocations) | $O(S \times B)$ localized Bresenham | **0.50 s** |
| **Stage 7: Blueprint Mutation** | < 1 MB (scratch templates) | $O(T)$ in-memory string parsing | **0.30 s** |
| **Stage 8: Building Placement** | 15 MB (`SparseVerticalStorage`) | $O(B \times \text{area})$ stamp operations | **0.40 s** |
| **Stage 9: Countryside** | Streamed directly into grids | $O(A)$ raster strip furrows | **0.50 s** |
| **Stage 10: Wildlife Simulation**| 4 MB (lair nodes & packs) | $O(L)$ Poisson density scatter | **0.30 s** |
| **Stage 11: Validation & Spawn** | 2 MB (BFS queue) | $O(V + E)$ graph connectivity | **0.20 s** |
| **TOTAL** | **~625 MB (Stable Preallocated)** | **Linear Scanline + Macro Graph** | **~5.25 s** |

---

## Conclusion & Next Development Milestones
With this stage specification completed, developers can implement each system in strict isolation:
1. **Next Immediate Coding Step**: Implement `World::SettlementPlanner` (Stages 2–4), establishing the province demarcations, settlement Poisson-disk sampling, and territorial control zone queries.
2. **Subsequent Step**: Build the `Architecture::BlueprintMutator` (Stage 7), expanding on the existing `BuildingGenerator` and `buildings.json` prefabs to support annex attachments, material swaps, and wear/tear dilapidation.
3. **Follow-on Step**: Implement the highway and intra-urban road network (Stages 5–6), followed by countryside strip farming and wildlife den scattering (Stages 9–10).
