# simplest chance move game example for imperfect information engine???

# Alhambra

enum for Directions:
North
West
South
East

Tile:
uint8_t type // if 0 then this tile is a leaf, i.e. adjacent to an actual built tile, placement on a leaf is not guaranteed to be legal
uint8_t wall_type
uint8_t[4] WallSegments_ContainingWallIds // id of wall containing the segment; if zero then wall is inactive (e.g. nullified by counter wall)
uint8_t[4] AdjTileIdxs // indices of asjacent tiles in the tile storage
// next and prev leaf ids in the linked list of leaf tiles, undefined for non-leafs
uint8_t prev_leaf
uint8_t next_leaf
//TODO maybe store if leaf would be a legal placement

Wall:
uint8_t id
uint8_t length
bool is_loop
TODO serial coding of all contained wall segments from end to end

PlayerState:
uint score
TODO money
Tile[24_and_expandable] built_tiles
Tile[4_and_expandable] reserve_tiles


buying can be tightly coupled to building or storing the tile, if the turn ending action of taking money or rebuilding the alhambra can be moved forward
otherwise -> extra storage for a players hovered tiles


keep id of longest wall, growing is easy, if a wall shrinks then recheck all walls for the newest longest wall


for wall counting adding and removing just perform the operation
always annihilate any existing walls that get nullified by a new tile, THEN seqeuntially work through all of the new tiles walls

when nullifying walls, check the walls serial segment coding to see if deleting an end segment => just do it and bump length down by one
when nullifying middle walls, check where in the serial segment coding it is, then reassign all wall of the shorter path to a new id and assign new length to the crreated walls
when adding a new wall segment that connects two existing walls, if the connected wall"s" are of same id then this is a loop, set loop flag and don't reassign wall ids; when finally deleting part of a wall that has the loop flag set, this is always a middle piece, but never reassign ids

---

for tile storage / tile placement move generation
build a tree of the built tiles, every leaf is a theoretical build spot
theoretical build spot has: position (maybe tree storage idx??), and a mask of MUST have parameters that can be overlayed over new tiles to check for compatibility (i.e. if any adjacent tile has to have a north wall and HAS to have a west NON-wall, then put it in the mask such that when the ACTUAL mask of the tile to test compatiblity for is operated with it then it gives zero if compatible [hint use OR mask then check for zero masks which are when its not matching])

issues: leaf tiles that are a leaf of multiple parent tiles
just store up to four directional parent pointers?!
or maybe ONLY keep the open leaf nodes as a tree, and the actual parent tiles that have been placed are just listed somewhere inefficiently for seldom recovery

---

storing tiles in a linear array, every tile knows it's position, and it also knows the indices of it's 4 adjacent tiles so adjacent tiles can be gatheres for any newly placed leaf tile node from that set

also when placing or removing apiece that has no walls, check the corner case where this piece may complete the walkway behind the wall to actually count


ACTUAL RULE: keine umbauten freien plätze und bereiche erlaubt
easily checkable by havannah way
wenn lücke zwischen angrenzenden tiles dann muss gerade loop geschlossen sein


default rule: draw money cards as long as <20
so min. 20 and max 28

