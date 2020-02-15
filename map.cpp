#include "map.h"

#include "globals.h"
#include "graphics.h"

/**
 * The Map structure. This holds a HashTable for all the MapItems, along with
 * values for the width and height of the Map.
 */
struct Map {
    HashTable* items;
    int w, h;
};

/**
 * Storage area for the maps.
 * This is a global variable, but can only be access from this file because it
 * is static.
 */
static Map map[2];                                                              // make array of map structs for 2 maps
static int active_map;

/**
 * The first step in HashTable access for the map is turning the two-dimensional
 * key information (x, y) into a one-dimensional unsigned integer.
 * This function should uniquely map (x,y) onto the space of unsigned integers.
 */
static unsigned XY_KEY(int X, int Y) {
    return X*(map[active_map].h)+Y;                                             // simple key algorithm for current tile using height of current map
}

/**
 * This is the hash function actually passed into createHashTable. It takes an
 * unsigned key (the output of XY_KEY) and turns it into a hash value (some
 * small non-negative integer).
 */
unsigned map_hash(unsigned key)
{
    return key%(NUMBUCKETS);
}

void maps_init()
{
    map[0].items = createHashTable(map_hash, NUMBUCKETS);                       // two hashtables, with two widths and heights defined in globals.h
    map[1].items = createHashTable(map_hash, NUMBUCKETS);
    map[0].w = WIDTH1;
    map[0].h = HEIGHT1;
    map[1].w = WIDTH2;
    map[1].h = HEIGHT2;
}

Map* get_active_map()
{
    return &map[active_map];                                                    // returns address of active map
}

Map* set_active_map(int m)
{
    active_map = m;                                                             // sets active map by returning address of new active map
    if(active_map == 0) return &map[0];
    if(active_map == 1) return &map[1];
    return NULL;
}

void print_map()
{
    // As you add more types, you'll need to add more items to this array.
    char lookup[] = {'1', '2', 'P', 'R', 'B', 'N', 'D', 'S', 'G', 'K', 'Q', 'H', 'O', 'Z'};  // used for serial debugging
    for(int y = 0; y < map_height(); y++)
    {
        for (int x = 0; x < map_width(); x++)
        {
            MapItem* item = get_here(x,y);
            if (item) pc.printf("%c", lookup[item->type]);
            else pc.printf(" ");
        }
        pc.printf("\r\n");
    }
}

int map_width()
{
    Map *map = get_active_map();                                                // active map width
    return map->w;
}

int map_height()
{
    Map *map = get_active_map();                                                // active map height
    return map->h;
}

int map_area()
{
    Map *map = get_active_map();                                                // active map area
    return (map->h * map->w);
}

MapItem* get_north(int x, int y)
{
    Map *map = get_active_map();                                                // gets active map
    int key = XY_KEY(x,y-1);                                                    // gets key of tile to north
    return (MapItem*) getItem(map->items,key);                                  // uses getItem to return object type as MapItem*
}

MapItem* get_south(int x, int y)
{
    Map *map = get_active_map();                                                // gets active map
    int key = XY_KEY(x,y+1);                                                    // gets key of tile to south
    return (MapItem*) getItem(map->items,key);                                  // uses getItem to return object type as MapItem*
}

MapItem* get_east(int x, int y)
{
    Map *map = get_active_map();                                                // gets active map
    int key = XY_KEY(x+1,y);                                                    // gets key of tile to east
    return (MapItem*) getItem(map->items,key);                                  // uses getItem to return object type as MapItem*
}

MapItem* get_west(int x, int y)
{
    Map *map = get_active_map();                                                // gets active map
    int key = XY_KEY(x-1,y);                                                    // gets key of tile to west
    return (MapItem*) getItem(map->items,key);                                  // uses getItem to return object type as MapItem*
}

MapItem* get_here(int x, int y)
{
    Map *map = get_active_map();                                                // gets active map
    int key = XY_KEY(x,y);                                                      // gets key of tile player is standing on
    return (MapItem*) getItem(map->items,key);                                  // uses getItem to return object type as MapItem*
}

void map_erase(int x, int y)
{
    unsigned int key = XY_KEY(x,y);                                             // gets key of tile defined by x,y arguments
    free((MapItem*)removeItem(map[active_map].items,key));                      // uses removeItem to clear tile
}

void add_wall1(int x, int y, int dir, int len)                                  // wall1 used for surrounding walls on map 0
{
    for(int i = 0; i < len; i++)
    {
        MapItem* w1 = (MapItem*) malloc(sizeof(MapItem));
        w1->type = TREE;
        w1->draw = draw_wall1;
        w1->walkable = false;
        w1->data = 0;
        unsigned key = (dir == HORIZONTAL) ? XY_KEY(x+i, y) : XY_KEY(x, y+i);
        void* val = insertItem(get_active_map()->items, key, w1);
        if (val) free(val); // If something is already there, free it
    }
}

void add_wall2(int x, int y, int dir, int len)                                  // wall2 used for surrounding and inner walls on map 1
{
    for(int i = 0; i < len; i++)
    {
        MapItem* w2 = (MapItem*) malloc(sizeof(MapItem));
        w2->type = DUNGEONBRICK;
        w2->draw = draw_wall2;
        w2->walkable = false;
        w2->data = 0;
        unsigned key = (dir == HORIZONTAL) ? XY_KEY(x+i, y) : XY_KEY(x, y+i);
        void* val = insertItem(get_active_map()->items, key, w2);
        if (val) free(val); // If something is already there, free it
    }
}

void add_river(int x, int y, int dir, int len)                                  // river used to block access to 2nd quest area in map 0
{
    for(int i = 0; i < len; i++)
    {
        MapItem* river = (MapItem*) malloc(sizeof(MapItem));
        river->type = RIVER;
        river->draw = draw_river;
        river->walkable = false;
        river->data = 0;
        unsigned key = (dir == HORIZONTAL) ? XY_KEY(x+i, y) : XY_KEY(x, y+i);
        void* val = insertItem(get_active_map()->items, key, river);
        if (val) free(val); // If something is already there, free it
    }
}

void add_flag(int x, int y)                                                     // flag used as waypoint marker
{
    MapItem* flag = (MapItem*) malloc(sizeof(MapItem));
    flag->type = FLAG;
    flag->draw = draw_flag;
    flag->walkable = true;
    flag->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), flag);
    if (val) free(val); // If something is already there, free it
}

void add_plant(int x, int y)                                                    // plants used as scenery in map 0 to see movement
{
    MapItem* plant = (MapItem*) malloc(sizeof(MapItem));
    plant->type = PLANT;
    plant->draw = draw_plant;
    plant->walkable = true;
    plant->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), plant);
    if (val) free(val); // If something is already there, free it
}

void add_gate1(int x, int y)                                                    // gate1 used as door until quest 1 complete
{
    MapItem* gate1 = (MapItem*) malloc(sizeof(MapItem));
    gate1->type = GATE1;
    gate1->draw = draw_gate1;
    gate1->walkable = false;
    gate1->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), gate1);
    if (val) free(val); // If something is already there, free it
}

void add_gate2(int x, int y)                                                    // gate2 used as door until quest 2 complete
{
    MapItem* gate2 = (MapItem*) malloc(sizeof(MapItem));
    gate2->type = GATE2;
    gate2->draw = draw_gate2;
    gate2->walkable = false;
    gate2->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), gate2);
    if (val) free(val); // If something is already there, free it
}

void add_NPC(int x, int y)                                                      // NPC character which gives player dialogue and quests
{
    MapItem* npc = (MapItem*) malloc(sizeof(MapItem));
    npc->type = NPC;
    npc->draw = draw_NPC;
    npc->walkable = false;
    npc->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), npc);
    if (val) free(val); // If something is already there, free it
}

void add_slime(int x, int y)                                                    // slime which needs to be collected for quest 1
{
    MapItem* slime = (MapItem*) malloc(sizeof(MapItem));
    slime->type = SLIME;
    slime->draw = draw_slime;
    slime->walkable = false;
    slime->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), slime);
    if (val) free(val); // If something is already there, free it
}

void add_ghost(int x, int y)                                                    // ghosts which need to be avoided for quest 2
{
    MapItem* ghost = (MapItem*) malloc(sizeof(MapItem));
    ghost->type = GHOST;
    ghost->draw = draw_ghost;
    ghost->walkable = true;
    ghost->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), ghost);
    if (val) free(val); // If something is already there, free it
}

void add_key(int x, int y)                                                      // key item used to access room for final zone
{
    MapItem* key = (MapItem*) malloc(sizeof(MapItem));
    key->type = KEY;
    key->draw = draw_key;
    key->walkable = false;
    key->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), key);
    if (val) free(val); // If something is already there, free it
}

void add_rock(int x, int y)                                                     // movable rock used to block gate after quest 1
{
    MapItem* rock = (MapItem*) malloc(sizeof(MapItem));
    rock->type = ROCK;
    rock->draw = draw_rock;
    rock->walkable = false;
    rock->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), rock);
    if (val) free(val); // If something is already there, free it
}

void add_heart(int x, int y)                                                     // heart item that increases amount of lives
{
    MapItem* heart = (MapItem*) malloc(sizeof(MapItem));
    heart->type = HEART;
    heart->draw = draw_heart;
    heart->walkable = false;
    heart->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x, y), heart);
    if (val) free(val); // If something is already there, free it
}

void add_portal(int x, int y)                                                   // portal used for switching between maps
{
    MapItem* portal = (MapItem*) malloc(sizeof(MapItem));
    portal->type = PORTAL;
    portal->draw = draw_portal;
    portal->walkable = false;
    portal->data = 0;
    void* val = insertItem(get_active_map()->items, XY_KEY(x,y), portal);
    if (val) free(val); // If something is already there, free it
}