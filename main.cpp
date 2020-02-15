// Project includes
#include "globals.h"
#include "hardware.h"
#include "map.h"
#include "graphics.h"
#include "speech.h"

#include "speaker.h"                                                            // added speaker.h file for speaker output

GameInputs in;

// Functions in this file
int get_action (GameInputs inputs);                                             // what player does
int update_game (int action);                                                   // game state function
void draw_game (int init);                                                      // draw game function
void init_main_map ();                                                          // initialize main map
void init_sub_map ();                                                           // initialize second map
int do_action(MapItem* item, int direction, int x, int y);                      // use action button
void npcAction();                                                               // NPC dialogue progression
int main ();
void game_over();                                                               // game over screen
void draw_start();                                                              // start screen

Speaker mySpeaker(p26);                                                         // defined speaker with pin 26

int go_right(int x, int y);                                                     // movement check cases
int go_left(int x, int y);
int go_up(int x, int y);
int go_down(int x, int y);

/**
 * The main game state. Must include Player locations and previous locations for
 * drawing to work properly. Other items can be added as needed.
 */
struct {
    int x,y;                                                                    // Current locations
    int px, py;                                                                 // Previous locations
    int wx, wy;                                                                 // Waypoint locations
    int has_key;                                                                // key check, 1 if quest 1 complete, 2 if quest 2 complete
    int map;                                                                    // active map
    int health, lives;                                                          // player health and lives
    int phealth, plives;                                                        // previoud health and lives
    int NPCprogress;                                                            // NPC dialogue progress
    int omni_mode;                                                              // omniscient mode tracker
    int slimeCount;                                                             // slime counter for quest 1 and previous slime count
    int waypoint;                                                               // waypoint active or not
    int has_heart;                                                              // check if player has powerup
} Player;

/**
 * Given the game inputs, determine what kind of update needs to happen.
 * Possbile return values are defined below.
 */
#define NO_ACTION 0
#define ACTION_BUTTON 1
#define MENU_BUTTON 2
#define GO_LEFT 3
#define GO_RIGHT 4
#define GO_UP 5
#define GO_DOWN 6
#define OMNI_MODE 7
#define WAYPOINT  8                                                             // added condition for waypoint

int get_action(GameInputs inputs)
{
    // for omni mode
    if (!inputs.b2) return OMNI_MODE;

    // for action button
    if (!inputs.b1) return ACTION_BUTTON;

    // for waypoints
    if (!inputs.b4)  return WAYPOINT;

    // for movement using accelerometer
    if (inputs.ay >= 0.4) return GO_UP;
    if (inputs.ay <= -0.4) return GO_DOWN;
    if (inputs.ax <= -0.4) return GO_LEFT;
    if (inputs.ax >= 0.4) return GO_RIGHT;

    // do nothing if none of the above are done
    return NO_ACTION;
}

/**
 * Update the game state based on the user action. For example, if the user
 * requests GO_UP, then this function should determine if that is possible by
 * consulting the map, and update the Player position accordingly.
 *
 * Return values are defined below. FULL_DRAW indicates that for this frame,
 * draw_game should not optimize drawing and should draw every tile, even if
 * the player has not moved.
 */
#define NO_RESULT 0
#define GAME_OVER 1
#define FULL_DRAW 2
#define GAME_LOST 3                                                             // added condition for losing game
int update_game(int action)
{
    // Save player previous location before updating

    Player.px = Player.x;
    Player.py = Player.y;

    // Do different things based on the each action.
    // You can define functions like "go_up()" that get called for each case.
    switch(action) {
        case GO_UP:                                                             // GO_UP received from accelerometer
            if (go_up(Player.px,Player.py)) {
                Player.y--;                                                     // if player can go up, change player position
                break;
            } else break;
        case GO_LEFT:                                                           // GO_LEFT received from accelerometer
            if (go_left(Player.px,Player.py)) {
                Player.x--;                                                     // if player can go left, change player position
                break;
            } else break;
        case GO_DOWN:                                                           // GO_DOWN received from accelerometer
            if (go_down(Player.px,Player.py)) {
                Player.y++;                                                     // if player can go down, change player position
                break;
            } else break;
        case GO_RIGHT:                                                          // GO_RIGHT received from accelerometer
            if (go_right(Player.px,Player.py)) {
                Player.x++;                                                     // if player can go right, change player position
                break;
            } else break;
        case ACTION_BUTTON:                                                     // ACTION_BUTTON input received, checks 4 tiles around player to determine what can be done
            action = do_action(get_north(Player.x,Player.y), 1, Player.x, Player.y);
            action = do_action(get_south(Player.x,Player.y), 2, Player.x, Player.y);
            action = do_action(get_east(Player.x,Player.y), 3, Player.x, Player.y);
            action = do_action(get_west(Player.x,Player.y), 4, Player.x, Player.y);
            if(NO_ACTION) return action;
            break;
        case OMNI_MODE:                                                         // OMNI_MODE input received
            Player.omni_mode = !Player.omni_mode;                               // toggles omni_mode
            if(Player.omni_mode) print_omni();                                  // if enabled, ghost icon is printed in lower left corner
            else clear_omni();                                                  // else if disabled, ghost icon is cleared
            break;
        case WAYPOINT:                                                          // WAYPOINT input received, only usable on map 0
            if(Player.waypoint == 0 && Player.map == 0) {                       // if waypoint not placed and player on map 0
                add_flag(Player.px, Player.py);                                 // place flag at player location
                Player.wx = Player.px;                                          // save waypoint coordinates
                Player.wy = Player.py;
                Player.waypoint = 1;                                            // waypoint placed
                char *line1 = "Waypoint Placed!";
                char *line2 = "use B1 to TP";
                speech(line1, line2);
                return FULL_DRAW;
            } else if(Player.waypoint == 1 && Player.map == 0) {                // if waypoint already placed and player on map 0
                map_erase(Player.wx, Player.wy);                                // clear waypoint tile
                Player.x = Player.wx;                                           // teleport player to waypoint coordinates
                Player.y = Player.wy;
                Player.waypoint = 0;                                            // waypoint not placed
                draw_game(FULL_DRAW);
                char *line1 = "Waypoint Deleted!";
                char *line2 = "use B1 to Place";
                speech(line1, line2);
                return FULL_DRAW;
            } else if(Player.map == 1) {                                        // player can't use waypoint on maze
                char *line1 = "No cheating! Go";
                char *line2 = "through the maze!";
                speech(line1, line2);
                return FULL_DRAW;
            }
            break;
        default:
            break;
    }
    return NO_RESULT;
}

int do_action(MapItem *item, int direction, int x, int y)                       // function for determining what action button does depending on adjacent tile
{
    char *line1;
    char *line2;
    switch(item->type) {                                                        // switch statement inspects type of mapitem
        case HEART:                                                             // HEART is item that reduces damage taken by ghosts from 20 to 10 per 100ms
            if(direction == 1) {                                                // all directions are checked so wrong tile isn't cleared
                map_erase(x, y-1);
            } else if(direction == 2) {
                map_erase(x, y+1);
            } else if(direction == 3) {
                map_erase(x+1, y);
            } else if(direction == 4) {
                map_erase(x-1, y);
            }
            line1 = "You feel the";
            line2 = "power surging!";
            speech(line1, line2);
            line1 = "within you!";
            line2 = "You now take";
            speech(line1, line2);
            line1 = "reduced damage";
            line2 = "from ghosts!";
            speech(line1, line2);
            Player.has_heart = 1;
            draw_lower_status(Player.health, Player.has_heart);                 // health bar is updated to blue color once powerup picked up
            draw_game(FULL_DRAW);
            return FULL_DRAW;
        case ROCK:                                                              // ROCK is moveable to walkable tiles after player completes quest 1
            if(Player.has_key == 1 || Player.omni_mode) {
                if(direction == 1) {                                            // all directions are checked to make sure rock can move
                    if(get_north(Player.x, Player.y-1)->walkable == false) return NO_RESULT;
                    else {
                        map_erase(Player.x, Player.y-2);
                        map_erase(Player.x, Player.y-1);
                        add_rock(Player.x, Player.y-2);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    }
                } else if(direction == 2) {
                    if(get_south(Player.x, Player.y+1)->walkable == false) return NO_RESULT;
                    else {
                        map_erase(Player.x, Player.y+2);
                        map_erase(Player.x, Player.y+1);
                        add_rock(Player.x, Player.y+2);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    }
                } else if(direction == 3) {
                    if(get_east(Player.x+1, Player.y)->walkable == false) return NO_RESULT;
                    else {
                        map_erase(Player.x+2, Player.y);
                        map_erase(Player.x+1, Player.y);
                        add_rock(Player.x+2, Player.y);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    }
                } else if(direction == 4) {
                    if(get_east(Player.x-1, Player.y)->walkable == false) return NO_RESULT;
                    else {
                        map_erase(Player.x-2, Player.y);
                        map_erase(Player.x-1, Player.y);
                        add_rock(Player.x-2, Player.y);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    }
                }
            } else if(Player.has_key == 0) {                                    // if player has not completed quest 1, they cannot move the rock
                line1 = "You are not";
                line2 = "strong enough to";
                speech(line1, line2);
                line1 = "move this rock.";
                line2 = "Capture slimes!";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            }
            return FULL_DRAW;
        case KEY:                                                               // if item is key, key is removed and Player.has_key is incremented
            Player.has_key = 2;
            if(direction == 1) {                                                // all directions are checked so wrong tile isn't cleared
                map_erase(x, y-1);
            } else if(direction == 2) {
                map_erase(x, y+1);
            } else if(direction == 3) {
                map_erase(x+1, y);
            } else if(direction == 4) {
                map_erase(x-1, y);
            }
            line1 = "You got the Key!";
            line2 = "Talk to The Eye";
            speech(line1, line2);
            draw_game(FULL_DRAW);
            return FULL_DRAW;
        case NPC:                                                               // if item is NPC, npcAction is called for various quest dialogue
            npcAction();
            draw_game(FULL_DRAW);
            return FULL_DRAW;
        case GATE1:                                                             // if item is GATE1, tile will be erased if player has completed quest 1
            if(Player.has_key == 1 || Player.omni_mode) {                       // or if omni_mode enabled
                if(direction == 1) {
                    map_erase(x, y-1);
                } else if(direction == 2) {
                    map_erase(x, y+1);
                } else if(direction == 3) {
                    map_erase(x+1, y);
                } else if(direction == 4) {
                    map_erase(x-1, y);
                }
                line1 = "Gate unlocked!";
                line2 = "Talk to The Eye";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            } else {                                                            // otherwise player must complete quest 1
                line1 = "This gate is";
                line2 = "locked.";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            }
        case GATE2:                                                             // if item is GATE2, player can't open until quest 2 complete and NPC dialogue has progressed
            if((Player.has_key == 2 && Player.NPCprogress == 3)|| Player.omni_mode) { // or if omni_mode enabled
                if(direction == 1) {
                    map_erase(x, y-1);
                } else if(direction == 2) {
                    map_erase(x, y+1);
                } else if(direction == 3) {
                    map_erase(x+1, y);
                } else if(direction == 4) {
                    map_erase(x-1, y);
                }
                line1 = "Door unlocked!";
                line2 = "Talk to The Eye";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            } else if(Player.has_key == 2 && Player.NPCprogress == 2) {         // else player must talk to NPC first
                line1 = "Better give the";
                line2 = "keys back first.";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            } else {                                                            // else player must get quest instructions
                line1 = "This door is";
                line2 = "locked.";
                speech(line1, line2);
                draw_game(FULL_DRAW);
                return FULL_DRAW;
            }
        case SLIME:                                                             // if item is SLIME, tile is erased and slime count is incremented
            if((Player.slimeCount <= 5) || Player.omni_mode) {
                if(direction == 1) map_erase(x, y-1);
                else if(direction == 2) map_erase(x, y+1);
                else if(direction == 3) map_erase(x+1, y);
                else if(direction == 4) map_erase(x-1, y);
                draw_game(FULL_DRAW);
                Player.slimeCount++;
                if(Player.slimeCount <= 5) {
                    draw_slimeCount(Player.slimeCount);
                }
                return FULL_DRAW;
            }
        case PORTAL:                                                            // if item is PORTAL, player is locked in map 0 until quest given, and
            switch(Player.map) {                                                // locked in map 1 until quest is completed
                case 0:
                    if(Player.NPCprogress == 1 || Player.omni_mode) {           // or if omni_mode enabled
                        Player.map = 1;
                        set_active_map(Player.map);
                        Player.x = 15;
                        Player.y = 22;
                        draw_slimeCount(Player.slimeCount);                     // slime counter appears on top of screen
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    } else if(Player.NPCprogress == 0) {
                        line1 = "Talk to The Eye";
                        line2 = "north of here";
                        speech(line1, line2);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    } else {
                        line1 = "Head East to ";
                        line2 = "the gate";
                        speech(line1, line2);
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    }
                case 1:
                    if(((Player.slimeCount == 5) && Player.NPCprogress == 2) || Player.omni_mode) {
                        Player.map = 0;
                        set_active_map(Player.map);
                        Player.x = 5;
                        Player.y = 43;
                        add_NPC(31, 43);
                        map_erase(6,5);
                        clear_slimeCount();                                     // slime counter cleared from top of screen
                        draw_game(FULL_DRAW);
                        return FULL_DRAW;
                    } else {
                        if(Player.slimeCount < 5) {
                            line1 = "Capture 5 slimes!";
                            line2 = "";
                            speech(line1, line2);
                            draw_game(FULL_DRAW);
                            return FULL_DRAW;
                        } else {
                            line1 = "Talk to The Eye";
                            line2 = "";
                            speech(line1, line2);
                            draw_game(FULL_DRAW);
                            return FULL_DRAW;
                        }
                    }
            }
        default :
            return NO_RESULT;
    }
}

void npcAction()                                                                // function for determining what dialogue NPC will say
{
    char *line1;
    char *line2;
    switch(Player.NPCprogress) {
        case 0:                                                                 // player given quest 1
            line1 = "The Eye: Ah, a";
            line2 = "Traveller! I";
            speech(line1,line2);
            line1 = "have a small";
            line2 = "problem I could";
            speech(line1,line2);
            line1 = "use your help";
            line2 = "with. Capture";
            speech(line1,line2);
            line1 = "5 slimes from my";
            line2 = "dungeon. You can";
            speech(line1,line2);
            line1 = "get there by";
            line2 = "taking the portal";
            speech(line1,line2);
            line1 = "south of here";
            line2 = "Good Luck!";
            speech(line1,line2);
            Player.NPCprogress = 1;
            break;

        case 1:
            if(Player.slimeCount == 5 || Player.omni_mode) {                    // player told to move to quest 2 zone
                line1 = "Excellent work!";
                line2 = "You are now";
                speech(line1, line2);
                line1 = "strong enough to";
                line2 = "move rocks! Also,";
                speech(line1, line2);
                line1 = "could I ask for";
                line2 = "another favor?";
                speech(line1,line2);
                line1 = "Go through the";
                line2 = "portal and head";
                speech(line1, line2);
                line1 = "East. Move the";
                line2 = "rock and open";
                speech(line1,line2);
                line1 = "up the gate";
                line2 = "on the river.";
                speech(line1,line2);
                line1 = "You could also";
                line2 = "head towards ";
                speech(line1, line2);
                line1 = "the NorthEast";
                line2 = "corner for a ";
                speech(line1, line2);
                line1 = "treasure I ";
                line2 = "uncovered...";
                speech(line1, line2);
                line1 = "I'll see you";
                line2 = "at the gate!";
                speech(line1, line2);
                Player.NPCprogress = 2;
                map_erase(13, 21);
                Player.has_key = 1;
                break;
            } else if (Player.slimeCount < 5) {
                line1 = "What are you";
                line2 = "waiting for? Go";
                speech(line1,line2);
                line1 = "get those slimes";
                line2 = "Traveller!";
                speech(line1,line2);
                break;
            }
        case 2:
            if(Player.has_key == 2 || Player.omni_mode) {
                Player.has_key = 2;
                line1 = "Impressive! I";
                line2 = "knew that I could";
                speech(line1,line2);
                line1 = "trust you to get";
                line2 = "my keys! Come ";
                speech(line1,line2);
                line1 = "inside for ";
                line2 = "your reward!";
                speech(line1,line2);
                map_erase(31, 43);
                add_NPC(45, 43);
                Player.NPCprogress = 3;
                break;
            } else if (Player.has_key == 1) {                                   // player given quest 2
                Player.has_key = 1;
                line1 = "I dropped my keys";
                line2 = "and now I can't";
                speech(line1,line2);
                line1 = "get inside my ";
                line2 = "house... Could";
                speech(line1,line2);
                line1 = "you get them for";
                line2 = "me? They are";
                speech(line1,line2);
                line1 = "outside the north";
                line2 = "part of my house.";
                speech(line1,line2);
                line1 = "Watch out for the";
                line2 = "ghosts,they hurt!";
                speech(line1,line2);
                draw_lifeCount(Player.lives);
                if(Player.omni_mode) Player.has_key = 2;
                break;
            } else
                break;
        case 3:
            return game_over();                                                 // game won
        default:
            break;
    }
}

int go_right(int x, int y)
{
    MapItem *item = get_east(x, y);                                             // get item to right
    if ( (item->walkable) || Player.omni_mode) return 1;                        // check if walkable
    else return 0;
}

int go_left(int x, int y)
{
    MapItem *item = get_west(x, y);                                             // get item to left
    if ( (item->walkable) || Player.omni_mode) return 1;                        // check if walkable
    else return 0;
}

int go_up(int x, int y)
{
    MapItem *item = get_north(x, y);                                            // get item to north
    if ((item->walkable) || Player.omni_mode) return 1;                         // check if walkable
    else return 0;
}

int go_down(int x, int y)
{
    MapItem *item = get_south(x, y);                                            //get item to south
    if ((item->walkable) || Player.omni_mode) return 1;                         // check if walkable
    else return 0;
}

/**
 * Entry point for frame drawing. This should be called once per iteration of
 * the game loop. This draws all tiles on the screen, followed by the status
 * bars. Unless init is nonzero, this function will optimize drawing by only
 * drawing tiles that have changed from the previous frame.
 */
void draw_game(int init)
{
    // Draw game border first
    if(init) draw_border();

    // Iterate over all visible map tiles
    for (int i = -5; i <= 5; i++) { // Iterate over columns of tiles
        for (int j = -4; j <= 4; j++) { // Iterate over one column of tiles
            // Here, we have a given (i,j)

            // Compute the current map (x,y) of this tile
            int x = i + Player.x;
            int y = j + Player.y;

            // Compute the previous map (px, py) of this tile
            int px = i + Player.px;
            int py = j + Player.py;

            // Compute u,v coordinates for drawing
            int u = (i+5)*11 + 3;
            int v = (j+4)*11 + 15;

            // Figure out what to draw
            DrawFunc draw = NULL;

            if (x >= 0 && y >= 0 && x < map_width() && y < map_height()) { // Current (i,j) in the map
                MapItem* curr_item = get_here(x, y);
                MapItem* prev_item = get_here(px, py);
                if (init || curr_item != prev_item) { // Only draw if they're different
                    if (curr_item) { // There's something here! Draw it
                        draw = curr_item->draw;
                    } else { // There used to be something, but now there isn't
                        draw = draw_nothing;
                    }
                }
            } else if (init) { // If doing a full draw, but we're out of bounds, draw the walls.
                if(Player.map == 0) draw = draw_wall1;
                if(Player.map == 1) draw = draw_wall2;
            }

            // Actually draw the tile
            if (draw && !((i == 0) && (j == 0))) draw(u, v);
        }
    }

    draw_player(Player.x, Player.y, Player.has_key);

    // Draw status bars
    if((Player.x!=Player.px) || (Player.py != Player.y)) {                      // only update coordinates if player moved
        draw_upper_status(Player.x, Player.y);
    }
    if((Player.health != Player.phealth) || (Player.lives != Player.plives)) {  // only update health bar if health or lives changed
        draw_lower_status(Player.health, Player.has_heart);
    }
}


/**
 * Initialize the main world map. Add walls around the edges, interior chambers,
 * and plants in the background so you can see motion.
 */
void init_main_map()
{
    // "Random" plants
    add_plant(5,  4);
    add_plant(19, 8);
    add_plant(26, 10);
    add_plant(33, 12);
    add_plant(47,16);
    add_plant(13,20);
    add_plant(20, 22);
    add_plant(27, 24);
    add_plant(41, 28);
    add_plant(7, 32);
    add_plant(14, 34);
    add_plant(21, 36);
    add_plant(35, 40);
    add_plant(8, 44);
    pc.printf("plants\r\n");

    add_rock(45,3);
    add_rock(46,4);
    add_rock(46,2);
    add_rock(47,3);
    pc.printf("rock\r\n");

    add_heart(46,3);
    pc.printf("powerup\r\n");

    add_NPC(6,5);
    pc.printf("NPC\r\n");

    add_portal(5,45);
    pc.printf("portal\r\n");

    add_ghost(38,34);
    add_ghost(38,35);
    add_ghost(38,36);
    add_ghost(38,37);

    add_ghost(40,34);
    add_ghost(40,36);
    add_ghost(40,37);
    add_ghost(40,38);

    add_ghost(42,34);
    add_ghost(42,35);
    add_ghost(42,36);
    add_ghost(42,38);

    add_ghost(44,35);
    add_ghost(44,36);
    add_ghost(44,37);
    add_ghost(44,38);

    add_ghost(46,34);
    add_ghost(46,35);
    add_ghost(46,36);
    add_ghost(46,37);
    pc.printf("ghosts\r\n");

    add_key(48, 36);
    pc.printf("key\r\n");

    add_river(30,             33,            HORIZONTAL, 19);
    add_river(29,             33,            VERTICAL,   7);
    add_river(29,             41,            VERTICAL,   8);
    pc.printf("River done!\r\n");

    add_rock(28,40);
    pc.printf("rock\r\n");

    add_gate1(29,40);
    pc.printf("gate1\r\n");

    add_gate2(35,40);
    add_gate2(35,41);
    pc.printf("gates2\r\n");

    add_wall2(35,             42,             VERTICAL, 7);
    add_wall2(35,             39,             HORIZONTAL, 15);
    pc.printf("wall2 done!\r\n");

    pc.printf("Adding walls!\r\n");
    add_wall1(0,              0,              HORIZONTAL, map_width());
    add_wall1(0,              map_height()-1, HORIZONTAL, map_width());
    add_wall1(0,              0,              VERTICAL,   map_height());
    add_wall1(map_width()-1,  0,              VERTICAL,   map_height());
    pc.printf("Walls done!\r\n");

    print_map();
}

void init_sub_map()
{
    Map* map = set_active_map(1);

    add_wall2(1, 19, HORIZONTAL, 15);
    add_wall2(5, 14, HORIZONTAL, 11);
    add_wall2(1, 5, HORIZONTAL, 10);
    add_wall2(20, 14, HORIZONTAL, 5);
    add_wall2(1, 9, HORIZONTAL, 5);
    add_wall2(10, 9, HORIZONTAL, 6);
    add_wall2(15, 4, HORIZONTAL, 6);
    add_wall2(20, 20, VERTICAL, 4);
    add_wall2(20, 10, VERTICAL, 4);
    add_wall2(5, 10, VERTICAL, 4);
    add_wall2(10, 6, VERTICAL, 3);
    add_wall2(15, 1, VERTICAL, 3);
    pc.printf("maze built\r\n");

    add_portal(16,21);
    pc.printf("portal\r\n");

    add_NPC(13,21);
    pc.printf("NPC\r\n");

    add_slime(2,2);
    add_slime(17,2);
    add_slime(2,12);
    add_slime(22,12);
    add_slime(22,21);
    pc.printf("slimes\r\n");

    pc.printf("Adding walls!\r\n");
    add_wall2(0,              0,              HORIZONTAL, map_width());
    pc.printf("top!\r\n");
    add_wall2(0,              map_height()-1, HORIZONTAL, map_width());
    pc.printf("bottom\r\n");
    add_wall2(0,              0,              VERTICAL,   map_height());
    pc.printf("left\r\n");
    add_wall2(map_width()-1,  0,              VERTICAL,   map_height());
    pc.printf("right\r\n");

    print_map();
}
/**
 * Program entry point! This is where it all begins.
 * This function orchestrates all the parts of the game. Most of your
 * implementation should be elsewhere - this holds the game loop, and should
 * read like a road map for the rest of the code.
 */
int main()
{
    // First things first: initialize hardware
    ASSERT_P(hardware_init() == ERROR_NONE, "Hardware init failed!");

    // Initialize the maps
    maps_init();
    init_main_map();
    init_sub_map();

    // Initialize game state
    set_active_map(0);
    Player.x = Player.y = 3;                                                    // Start player at (3,3)

    draw_start();                                                               // show start screen until B2 is held
    uLCD.filled_rectangle(0,8,127,0,BLACK);                                     // clear top bar from start screen

    Player.health = 100;                                                        // initialize player health at 100
    Player.lives  = 3;                                                          // initialize player lives at 3

    uLCD.filled_rectangle(0,118,128,128,BLACK);                                 // clear bottom area from start screen

    // Initial drawing
    draw_game(true);
    draw_player(Player.x, Player.y, Player.has_key);

    // Main game loop
    while(1) {
        // Timer to measure game update speed
        Timer t;
        t.start();

        // Actuall do the game update:
        // 1. Read inputs
        in = read_inputs();
        // 2. Determine action (get_action)
        int action = get_action(in);
        // 3. Update game (update_game)
        Player.phealth = Player.health;
        Player.plives   = Player.lives;

        int update = update_game(action);

        char* line1;
        char* line2;
        if(((get_here(Player.x,Player.y))->type == GHOST) && !Player.omni_mode) {
            if(Player.has_heart == 0) Player.health = Player.health-20;         // player loses 20 health for standing in ghost every 100ms
            else if(Player.has_heart == 1) Player.health = Player.health-10;    // player loses 10 health for standing in ghost with powerup active
            if(Player.health == 0) {
                uLCD.filled_rectangle(70,122,120,128,RED);                      // show red health bar once dead
                Player.lives--;
                switch(Player.lives) {                                          // messages showing less lives
                    case 2:
                        line1 = "You have lost";
                        line2 = "a life! 2 left.";
                        speech(line1, line2);
                        break;
                    case 1:
                        line1 = "You have lost";
                        line2 = "a life! 1 left.";
                        speech(line1, line2);
                        break;
                    case 0:
                        line1 = "You have lost";
                        line2 = "a life! 0 left.";
                        speech(line1, line2);
                        if(Player.lives == 0) update = GAME_LOST;               // show game failed screen
                        break;
                    default:
                        break;
                }
                Player.x = 30;                                                  // reset 2nd quest (only place player can die)
                Player.y = 40;
                if(Player.has_key == 2) {                                       // reset key
                    Player.has_key = 1;
                    add_key(48, 36);
                }
                Player.health = 100;                                            // reset player health
                draw_lifeCount(Player.lives);
                draw_game(FULL_DRAW);                                           // redraw game
            }
        }
        // 3b. Check for game over
        if(update == GAME_LOST) {                                               // show game lost screen
            uLCD.filled_rectangle(0,0,128,128,BLACK);
            uLCD.locate(1,3);
            uLCD.color(RED);
            uLCD.text_width(2);
            uLCD.text_height(2);
            uLCD.printf("GAMEOVER");

            uLCD.locate(1,4);
            uLCD.color(RED);
            uLCD.text_width(2);
            uLCD.text_height(2);
            uLCD.printf("YOU LOSE");

            uLCD.locate(1,7);
            uLCD.color(RED);
            uLCD.text_width(1);
            uLCD.text_height(1);
            uLCD.printf("reset to play");

            mySpeaker.PlayNote(330.0,0.417,0.05);                               // play game lose music theme
            mySpeaker.PlayNote(330.0,0.417,0.05);
            mySpeaker.PlayNote(330.0,0.417,0.05);

            mySpeaker.PlayNote(494.0,0.417,0.05);
            mySpeaker.PlayNote(494.0,0.417,0.05);

            mySpeaker.PlayNote(440.0,0.417,0.05);
            mySpeaker.PlayNote(370.0,0.417,0.05);

            mySpeaker.PlayNote(294.0,0.417,0.05);
            mySpeaker.PlayNote(392.0,0.417,0.05);
            wait(100000000000000);
        }
        if(update == GAME_OVER)  game_over();                                   // show game won screen
        // 4. Draw frame (draw_game)
        draw_game(update);                                                      // update game

        // 5. Frame delay
        t.stop();
        int dt = t.read_ms();
        if (dt < 100) wait_ms(100 - dt);
    }
}

void game_over()
{
    uLCD.filled_rectangle(0,0,128,128,BLACK);
    uLCD.locate(1,3);
    uLCD.color(TEXTGREEN);
    uLCD.text_width(2);
    uLCD.text_height(2);
    uLCD.printf("GAMEOVER");

    uLCD.locate(1,4);
    uLCD.color(TEXTGREEN);
    uLCD.text_width(2);
    uLCD.text_height(2);
    uLCD.printf("YOU WIN");

    uLCD.locate(1,7);
    uLCD.color(TEXTGREEN);
    uLCD.text_width(1);
    uLCD.text_height(1);
    uLCD.printf("reset to play");

    mySpeaker.PlayNote(330.0,0.417,0.05);                                       // play game over music theme
    mySpeaker.PlayNote(523.0,0.200,0.05);
    mySpeaker.PlayNote(587.0,0.200,0.05);
    mySpeaker.PlayNote(659.0,0.200,0.05);
    mySpeaker.PlayNote(698.0,0.200,0.05);
    mySpeaker.PlayNote(698.0,0.417,0.05);
    mySpeaker.PlayNote(784.0,0.417,0.05);
    mySpeaker.PlayNote(880.0,0.417,0.05);
    mySpeaker.PlayNote(988.0,0.417,0.05);
    mySpeaker.PlayNote(1047.0,0.417,0.05);
    wait(10000000000000);
}

void draw_start()
{
    in = read_inputs();                                                         // read input
    int colorSwap = 0;                                                          // toggle for alternating colors on start screen title text
    uLCD.textbackground_color(BLACK);

    char gameTitle[] = "THE LONE TRAVELLER";
    char *bar        = "------------------";
    char *startgame  = "Hold B2 to START";
    char *name       = "By: Shlok Patel";

    while (in.b1) {                                                             // clear start screen if B2 pressed
        in = read_inputs();
        uLCD.locate(0,0);                                                       // print out top and bottom border bars
        uLCD.color(TEXTGREEN);
        uLCD.printf(bar);

        uLCD.locate(0,1);
        uLCD.color(TEXTGREEN);
        uLCD.printf(bar);

        uLCD.locate(0,13);
        uLCD.color(TEXTGREEN);
        uLCD.printf(bar);

        uLCD.locate(0,14);
        uLCD.color(TEXTGREEN);
        uLCD.printf(bar);

        uLCD.locate(1,3);
        for(int i = 0; i<9; i++) {                                              // print out THE LONE
            if (colorSwap)uLCD.color(GOLD);
            else uLCD.color(SILVER);
            uLCD.text_width(2);
            uLCD.text_height(2);
            uLCD.printf("%c", gameTitle[i]);
            colorSwap = !colorSwap;
        }
        in = read_inputs();

        uLCD.locate(0,4);
        for(int i = 9; i<19; i++) {                                             // print out TRAVELLER
            uLCD.text_bold(TEXTBOLD);
            if(colorSwap) uLCD.color(SILVER);
            else uLCD.color(GOLD);
            uLCD.text_width(2);
            uLCD.text_height(2);
            uLCD.printf("%c", gameTitle[i]);
            colorSwap = !colorSwap;
        }
        in = read_inputs();

        uLCD.text_height(1);                                                    // print out Hold B2 to START
        uLCD.text_width(1);
        uLCD.locate(1,11);
        uLCD.color(RED);
        uLCD.printf(startgame);
        in = read_inputs();

        uLCD.text_height(1);                                                    // print out By: Shlok Patel
        uLCD.text_width(1);
        uLCD.locate(3,15);
        uLCD.color(TEXTGREEN);
        uLCD.printf(name);
        in = read_inputs();
    }
}