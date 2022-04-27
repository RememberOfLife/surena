simplest chance move game example for imperfect information engine! that actually has strategic depth

# Caesar


for default gameboard keep ref sheet for abbreviations to ids

perfect hashing for province to connections:
http://stevehanov.ca/blog/?id=119
generate intermediate table of hash offsets by displacing items from buckets where multiple items fall into the same bucket
thereby finding the hash offset for all items that WOULD fall into that bucket using offset 0



edge cases:

simultaneous placement of last control token: draw between players
senate from province bag: RAW only place senate control tolkens if actually controlling the province of the senate
when playing with border control, place overflowing tokens in a redraw bag and get a random one at resolution time
when a player is not able to place any of the tiles in their hand to a legal spot: they lose the game


18 regions

30 border locations


6 types of province bonuses
16 base influence tokens
3 centurion influence tokens
1 control marker
num of token types: 26
2x player color
2x orientation for war token
2x facedown
+1 null card
= 209

12 control markers

province tokens:
6x senate
4x wealth
4x tactics
4x might
3x centurion
3x poison

influence tokens (16+3centurion):
wild 4/0 3/1 2/2 2/2
sword, shield, boat each
6/0 5/1 4/2 3/3
centurion tokens
wild 3/3
sword 7/0
shield+boat 4/4
