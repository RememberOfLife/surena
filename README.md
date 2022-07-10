# surena

General purpose board game backend and some AI to go with it.

## support
|game|status|
|---|---|
|Chess|DONE|
|Havannah|DONE|
|TicTacToe|DONE|
|TicTacToe.Ultimate|DONE|
|TwixT.PP|DONE|

### games todo

|game|flags|relative prio|
|---|---|---|
|6 Nimmt!|HI, SM, RM|+|
|Alhambra|RM||
|Azul|RM||
|Caesar|HI, RM|++|
|Connect4|PI||
|Go|PI|-|
|Oshi Sumo|SM|+++|
|Quoridor|PI||
|Settlers of Catan|HI, RM||
|Skat|HI, RM|++|
|Tak|PI|++|
|Wizards|HI, RM||

## todos
* rerrorf function fixes
  * if no string given, auto fill with get general error string from game.h
  * boilerplate: free data2 on destruction, make sure not to copy and clone
* use readline in main https://stackoverflow.com/questions/2600528/c-readline-function
* plugin init / cleanup (game+engine)
* use snprintf (https://stackoverflow.com/a/26910616) to get the correct size of the error string in the _rerrorf function
* add uci wrapper engine for general purpose chess engines (same for tak)
* document workflow for non perfect information games
* base64 util func
* separate out squirrelnoise5 into a dep?

## ideas
* need some way to get a true state representing info dump out of a game
  * e.g. 50 move rule in chess isnt encoded in the state
  * manage this with binary state export feature flag? or general state export type specifies
* team games with team rewards
* games with rewards more complicated than just win/loss
* some games may need to have access to some sort of time control for their functionality (munchkin and upcount time control games)
* clone method for the game:
  * option to choose if to discretize if available or not
  * only to copy for a certain privacy view (i.e. which player perspectives actual data to include)
* for simultaneous moves the engine has to merge extra moves into the tree even if there are already moves on a node
  * possibly generate_all_moves_for_all_discretization method for the game class
  * order moves by uint64_t value and then splice possible moves into the actual children
