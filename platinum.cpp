/** Author : Kevin Prouvot
 * 
 * Summary :
 * 
 * - Include
 * - Headers
 * - Constants
 * - Global var
 * - Classes
 *   - Zone
 *   - ZoneIntend
 *   - Mood
 *   - Continent
 *   - Pod
 *   - Overlord
 *   - Overmind
 * - PathFinding
 *   - Path finding with closure and weight
 * - Commands
 *   - Move
 *   - AddMove
 *   - Create
 *   - AddCreate
 * - Initialisation
 * - Update
 *   - UpdateCommands
 *   - UpdatePlatinum
 *   - UpdateZones
 *   - UpdateOvermind
 *   - UpdatePods
 * - Clear
 * - Main()
 * 
 * */

/**INCLUDE**/
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <time.h>

using namespace std;


/** HEADERS **/
struct Global;

class Zone;
class ZoneIntend;
class Mood;
class Continent;
class Overlord;
class Overmind;
class Continent;
class Pod;

struct Move;
void addMove(int podsCount, Zone* zoneOrigin, Zone* zoneDestination);
struct Create;
void addCreate(int podsCount, Zone* zoneDestination);

ZoneIntend* pathFinding(Zone* origin, function<bool (Zone*)> func);

void initOvermind();
void rec_continent(Continent* c, Zone* z);
void initContinents();

void updateCommands();
void updatePlatinum();
void updateZones();
void updateOverlords();
void updatePods();

void clear();


/** CONTANTS **/
const int DEFAULT_RATIO = 10;
const int MAX_WEIGHT_RATIO = 10000;
const int MIN_WEIGHT_RATIO = -10000;
const int WEALTH_CONCENTRATION_FACTOR = 50;
const int POD_PRICE = 20;
const int MAX_NEIGHBOURS = 6;


/** GLOBAL VAR **/
vector<Zone*> zones;                // Zone list
vector<Continent*> continents;      // Continent list
vector<Move> moves;                 // List of Move command
vector<Create> creates;             // List of Create command
vector<Pod*> pods;                  // List of pods
Overmind* overmind;

/** CLASSES **/
/*
  represent a Tile on map.
  Zone belonging to a continent.
  In a worry of simplicity, our pods will be placed into myPods instead of pX
*/
class Zone {
    public:
        int id;                     // this zone's ID
        int platinum;               // platinium in this zone
        int owner;                  // the player who owns this zone (-1 otherwise)
        int myPods;                 // Player's pods, even if player get 3 as Id, his pods will still be there
        vector<Zone*> links;        // neighbours
        int p1;                     // player 1's PODs on this zone
        int p2;                     // player 2's PODs on this zone
        int p3;                     // player 3's PODs on this zone
        Continent* continent;       // continent where the zone is

        int myId;                   // Player Id;
        int value;                  // value fixed by the overlord
        int podDanger;              // potentiel enemy value for the next turn;
        
        bool blacklisted;           // usefull to build continent
        Zone* ancestor;             // usefull with pathfinding
        
        Zone(int playerId)
        {
            platinum = 0;
            _podIntend = 0;
            myPods = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;
            blacklisted = false;
            myId = playerId;
            podDanger = 0;
        }
        
        //Zone's flags
        bool isNeutral() 
        {
            return (owner == -1);
        }
        
        bool isMine() 
        {
            return owner == myId;
        }
        
        bool isHostil() 
        {
            return (owner != myId && owner != -1);
        }
        
        bool hasEnemyPodOnIt() 
        {
            return ((p1 > 0 || p2 > 0 || p3 > 0) && myPods == 0);
        }
        
        bool isPeacefull()
        {
            return isMine() && !hasEnemyPodOnIt();
        }
        
        int getMaxEnemyPod()
        {
            return max(p1, max(p2, p3));
        }
        
        bool hasSupremacy()
        {
            return myPods + _podIntend > getMaxEnemyPod();
        }
        
        bool willLooseSupremacy()
        {
            return (hasEnemyPodOnIt() || hasDanger() || isHostil()) && ((myPods + _podIntend) < (getMaxEnemyPod() + podDanger));
        }
        
        bool willBeOk()
        {
            return _podIntend > getMaxEnemyPod();
        }
        
        bool hasPlatinum() 
        {
            return platinum > 0;
        }
        
        bool hasBigPlatinum()
        {
            return platinum > 4;
        }
        
        bool hasIntend()
        {
            return _podIntend > 0;
        }
        
        bool hasFriendOnIt()
        {
            return myPods > 0;
        }
        
        bool hasDanger()
        {
            return podDanger > 0;
        }
        
        bool isOverFlow()
        {
            return myPods + _podIntend > 3;
        }
        
        bool isInDanger()
        {
            bool result = !isPeacefull();
            if(!result && hasDanger()){
                return willBeOk();
            }
            return result;
        }
        
        bool isVirgin()
        {
            return isNeutral() && !hasIntend();
        }
        
        bool isTotallySecurized()
        {
            bool result = isPeacefull();
            if(result){
                for(Zone* z : links)
                {
                    if(!isPeacefull())
                    {
                        result = false;
                    }
                }
            }
            return result;
        }
        
        //Zone's operator
        bool operator==(const Zone& z1)
        {
            return id == z1.id;
        }
        
        bool operator<( Zone* z)
        {
            return value < z->value;
        }
        
        int getIntend()
        {
            return _podIntend;
        }
        
        void addIntend(int i)
        {
            _podIntend += i;
            
        }
        
        bool isOnWar()
        {
            return hasEnemyPodOnIt() && hasFriendOnIt();
        }
        
        //Zone's methods
        void clearIntend()
        {
            _podIntend = 0;
        }
    private :
        int _podIntend;              // number of pod intend to move on this zone
};



/*
  A move intend
  - attribute 'zone' is the nearest zone to go in order to rally destination
  - distance is the distance from origin to destination
  - weight is the pertinence of this intend
*/
class ZoneIntend {
    public:
        Zone* zone;
        int distance;
        int weight;
        
        ZoneIntend()
        {
            zone = nullptr;
            distance = 0;
            weight = 0;
        }
};

/*
  Moods are global attitudes.
  - 'name' is the mood's name
  - 'baseValue' is the weight accorded by the overlord to this mood
  - 'default value' is the first given value
  - 'catcher' is a boolean function which represents the typical seeked zone of the mood
  - <optionnal> 'condition' conditional catcher for immobility
  - <optionnal> 'conditionValue' weigth of immobility
  - 'possibleZone' number of zone responding to the catcher by continent
*/
class Mood {
    public:
        
    
        Mood(string name, int value, function<bool (Zone*)> seekedZone)
        {
            _name = name;
            _baseValue = value;
            _defaultValue = value;
            _catcher = seekedZone;
            _isConditionnal = false;
        }
        
        Mood(string name, int value, function<bool (Zone*)> seekedZone, function<bool (Zone*)> condition, int conditionValue)
        {
            _name = name;
            _baseValue = value;
            _defaultValue = value;
            _catcher = seekedZone;
            _isConditionnal = true;
            _condition = condition;
            _conditionValue = conditionValue;
        }
        
        ZoneIntend* getIntend(Zone* currentZone)
        {
            if(_isConditionnal && _condition(currentZone)){
                ZoneIntend* result = new ZoneIntend();
                result->weight = _conditionValue;
                result->zone = currentZone;
            }
            else{
                return getIntendedZone(currentZone);
            }
        }
        
        ZoneIntend* getIntendedZone(Zone* currentZone)
        {
            ZoneIntend* result;
            if(isDisabled())
            {
                result = new ZoneIntend();
                result->weight = MIN_WEIGHT_RATIO;
            }
            else{
                result = pathFinding(
                    currentZone,
                    _catcher);
                if(result->zone == nullptr)
                {
                    result->weight = MIN_WEIGHT_RATIO;
                }
                else
                {
                    result->weight = _baseValue - result->distance;
                }
            }
            return result;
        }
        
        bool isDisabled()
        {
            return _possibleZone == 0;
        }
        
        void addValue(int x)
        {
            _baseValue += x;
        }
        
        void reset()
        {
            _baseValue = _baseValue;
        }
        
        string getName()
        {
            return _name;
        }
        
        function<bool (Zone*)> getCatcher()
        {
            return _catcher;
        }
        
        void clearPossibleZones()
        {
            _possibleZone = 0;
        }
        
        void incrementPossibleZones()
        {
            _possibleZone++;
        }
    
    private:
        string _name;
        int _baseValue;
        int _defaultValue;
        function<bool (Zone*)> _catcher;
        function<bool (Zone*)> _condition;
        int _possibleZone;
        bool _isConditionnal;
        int _conditionValue;
};

/*
  A continent represents linked zones
*/
class Continent {
    public:
        int id;                             // Continent's id
        string name;
        int platinum;                       // Platinum amount on this continent
        int wealthConcentration;            // Platinum density ratio
        int value;                          // Total value estimated by the overmind
        vector<Zone*> myZones;              // Zone of the continent
        vector<Mood*> moods;                // Available moods
        bool isOwned;                       // is totally owned by player
        bool isLost;                        // is totally lost by player
        int myPods; 												// Number of main player pods on this continent
        int p1;															//   "       player 1     "    "   "     "   
        int p2;															//   "       player 2     "    "   "     "
        int p3;															//   "       player 3     "    "   "     "
        bool platinumZoneOccupied;					// all the platinul zone are occuped
        int intends;												// numbers of intends on this continent
        
        Continent(int idContinent)
        {
            id = idContinent;
            platinum = 0; 
            isOwned = false;
            isLost = false;
            myPods = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;
            platinumZoneOccupied= false;
            
            switch(id)
            {
                case (0):
                    name = "North_America";
                break;
                case (1):
                    name = "South_Am.Africa";
                break;
                    
                case (2):
                    name = "Enrasia";
                break;
                case (3):
                    name = "Oceania";
                break;
                default:
                    name = "Japan";
                break;
            }
        }
        
        vector<Mood*> getMoods()
        {
            return moods;
        }
        
        void setMoods(vector<Mood*> m)
        {
            moods = m;
        }
        
        int getSize()
        {
            return myZones.size();
        }
        
        void computeWealthConcentration()
        {
            wealthConcentration = float((float)platinum/getSize()) * WEALTH_CONCENTRATION_FACTOR;
        }
        
        void computeValue()
        {
            value = wealthConcentration + getSize();
        }
        
        int getTotalEnemyPods()
        {
            return p1 + p2 + p3;
        }
        
        int getMaxEnemyPod()
        {
            return max(p1, max(p2, p3));
        }
        
        bool isSafe()
        {
            return (p1 == 0 && p2 == 0 && p3 == 0);
        }
        
        bool isIgnored()
        {
            return isOwned || isLost;
        }
        
        bool hasNoFriendlyPod()
        {
            return myPods == 0 && intends == 0;
        }
        
        void clearPods()
        {
            myPods = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;
        }
        
        void clearIntends()
        {
            intends = 0;
        }
};

/*
  Pods 
  pod is affected to a zone and moves depending of his mood.
  pod have a certain amount of will. This will is affected by moods
*/
class Pod {
    public:
        Zone* currentZone;      // zone where the pod is
        int will;               // pod's will.
        vector<string> debug;   // debug information       
        Mood* lastMood;  				// last mood
        Zone* intend;						// destination intend
        
        Pod(Zone* pos)
        {
            currentZone = pos;
            will = MIN_WEIGHT_RATIO;
        }
        
        void move(Zone* z)
        {
            addMove(1, currentZone, z);
        }
        
        Continent* getContinent()
        {
            return currentZone->continent;
        }
        
        void handleWar()
        {
            currentZone->p1 = max(0, currentZone->p1 - 1);
            currentZone->p2 = max(0, currentZone->p1 - 1);
            currentZone->p3 = max(0, currentZone->p1 - 1);
        }
        
        void update()
        {
            if(currentZone->hasEnemyPodOnIt())
            {
                handleWar();
            }
            else{
                intend = nullptr;
                for(Mood* m : getContinent()->getMoods())
                {
                    //clock_t start;
                    //start = clock();
                    
                    ZoneIntend* zi = m->getIntend(currentZone);
                    if(zi->weight > will)
                    {
                        intend = zi->zone;
                        will = zi->weight;
                        lastMood = m;
                    }
                    
                    //int duration = ((clock() - start ) * 1000 )/ ((double)CLOCKS_PER_SEC);
                    //stringstream s;
                    //s << "Update mood " << m->getName() << " time : " << duration  << "ms" << endl;
                    //debug.push_back(s.str());
                }
                
                if(intend != nullptr && intend != currentZone)
                {
                    //cerr << "Pod on Z-" << currentZone->id << ", will : " << will << endl;  
                    move(intend);
                }
            }
        }
};

/*
  Overlord decide everything in his continent from the creation of the pod to their moods.
  An overlord belonging to a continent.
*/
class Overlord {
    public:
        Continent* continent;
        vector<Zone*> sortedZones;
        
        int ratio;                   // Ratio (in pourcent) fixed by overmind
        
        vector<Mood*> moods;
        
        Overlord(Continent* c)
        {
            continent = c;
            c->setMoods(moods);
        }
        
        vector<Zone*> getZones()
        {
            return continent->myZones;
        }
        
        int getContinentValue()
        {
            return continent->value;
        }
        
        bool hasDoneWork()
        {
            if(continent->isOwned)
            {
                cerr << "Work done Master" << endl;
            }
            return continent->isOwned;
        }
        
        bool hasLost()
        {
            if(continent->isLost)
            {
                cerr << "I failed Master" << endl;
            }
            
            return continent->isLost;
        }
        
        
        
        //Update continent's informations
        //Update Moods status
        void feedback()
        {
            int ownedZones = 0;
            int enemyZones = 0;
            
            for(Mood* m : moods)
            {
                m->clearPossibleZones();
            }
            continent->platinumZoneOccupied = true;
            for(Zone* z : getZones())
            {
                if(z->hasPlatinum() && !z->isMine())
                {
                    continent->platinumZoneOccupied=false;
                }
                
                
                for(Mood* m : moods)
                {
                    if(m->getCatcher()(z))
                    {
                        m->incrementPossibleZones();
                    }
                    
                }
                if(z->isPeacefull())
                {
                    ownedZones++;
                }
                if(z->isHostil())
                {
                    enemyZones++;
                }
            }
            continent->isOwned = (ownedZones == continent->getSize());
            continent->isLost = (enemyZones == continent->getSize());
        }
        
        void computeRatio(int worldValue)
        {
            ratio = (getContinentValue() * 100)/ worldValue;
        }
};

/*
  The Overmind is the main hub
  He spawns one overlord per continent
  He knows everything
  He decides the budget of each overlord
*/
class Overmind
{
    public:
        vector<Overlord*> overlords;        // Overlords list
        int playerCount;                    // Number of playes
        int myId;                           // Id identifier
        int zoneCount;                      // total number of zones                         
        int linkCount;                      // total number of links between zones
        int platinum;                       // Platinum ressources
        int worldValue;                     // total Value
        bool isFirstTurn; 									// flag for the first turn
        
        void spawnOverlord(Continent* c)
        {
            Overlord* o = new Overlord(c);
            overlords.push_back(o);
            worldValue = 0;
            o->moods = initMoods();
            o->continent->setMoods(o->moods);
            isFirstTurn = true;
        }
        
        void ignoreOverlord(Overlord* o)
        {
            overlords.erase(remove(overlords.begin(), overlords.end(), o), overlords.end());
            worldValue -= o->getContinentValue();
        }
        
        int overlordsCount()
        {
            return overlords.size();
        }
        
        void purchasePod(int count, Zone* destination)
        {
            
            if(!destination->isHostil() && platinum >= POD_PRICE){
                platinum -= POD_PRICE;
                addCreate(count, destination);
                cerr << " Purchased " << count << " pod on " << destination->id << ", value : " << destination->value << endl;
                cerr << " platinum left " << platinum << endl;
            }
            else
            {
                cerr << "Purchase error" << endl;
            }
        }
        
        void getOverlordsFeedBack()
        {
            // get feedback
            for(Overlord* o : overlords)
            {
                o->computeRatio(worldValue);
                o->feedback();
                if(o->hasDoneWork() || o->hasLost())
                {
                    ignoreOverlord(o);
                }
            }
        }
        
        static bool zoneCompareLess(const Zone* lhs, const Zone* rhs)
        {
            return lhs->value > rhs->value;
        }
        
        static bool platinumCompareLess(const Zone* lhs, const Zone* rhs)
        {
            return lhs->platinum > rhs->platinum;
        }
        
        vector<Mood*> initMoods()
        {
            vector<Mood*> moods;
            
            Mood* aggressive = new Mood(
                "aggressivity",
                DEFAULT_RATIO + 2 - playerCount,
                [] (Zone* z) { return !z->hasSupremacy();}
            );
            
            Mood* defensive = new Mood(
                "defend",
                 DEFAULT_RATIO + 4,
                 [] (Zone* z) { return z->hasBigPlatinum() && z->willLooseSupremacy();},
                 [] (Zone* z) { return z->hasBigPlatinum() && z->willLooseSupremacy();},
                 DEFAULT_RATIO + 20
            );
            
            Mood* conquest = new Mood(
                "conquest",
                DEFAULT_RATIO,
                [] (Zone* z) { return z->isHostil() && z->willLooseSupremacy();}
            );
            
            Mood* greediness = new Mood(
                "greedy",
                DEFAULT_RATIO + 4,
                [] (Zone* z) { return !z->isMine() && z->hasBigPlatinum() && z->willLooseSupremacy();}
            );
            
            Mood* slowExpand = new Mood(
                "slowExpand",
                DEFAULT_RATIO + 3,
                [] (Zone* z) { return !z->isMine() && z->hasPlatinum() && (z->willLooseSupremacy() || z->isVirgin());}
            );
            
            Mood* defaultMood = new Mood(
                "default",
                 DEFAULT_RATIO,
                 [] (Zone* z) { return !z->isMine() && !z->hasIntend();}
            );
            
            //moods.push_back(aggressive);
            moods.push_back(defensive);
            moods.push_back(greediness);
            moods.push_back(slowExpand);
            moods.push_back(conquest);
            moods.push_back(defaultMood);
            
            return moods;
        }
        
        void setZoneValue(Zone* z)
        {
            int value = 0;
            
            //if zone is secure, it looses all his value
            if(z->isHostil() || z->continent->isIgnored()  || z->continent->platinumZoneOccupied)
            {
                z->value = 0;
            }
            else{
                //base Value is platinum on it
                if(isFirstTurn){
                    isFirstTurn = false;
                    if(true)
                    {
                        if(z->platinum == 2)
                        {
                            z->value = 1000;
                        }
                    }
                    else{
                        value = z->platinum * 10;
                    }
                }
                else{
                    if(z->willLooseSupremacy() || z->isNeutral()){
                        value = z->platinum * 5;
                    }
                    value -= z->hasIntend();
                    if(z->continent->hasNoFriendlyPod())
                    {
                        if(playerCount > 2)
                        {
                            z->value += 20;
                        }
                        else{
                            z->value += 10;
                        }
                    }
                }
                if(z->isNeutral())
                {
                    value += 6;
                }
                
                value -= z->myPods;
                value += z->getMaxEnemyPod();
                value += z->podDanger;

                for(Zone* l : z->links)
                {
                    value += l->platinum;
                    if(l->isHostil())
                    {
                        value+=1;
                    }
                }
                value += z->continent->getMaxEnemyPod();
            }
            //cerr << "Zone " << z->id << ", value : " << value << endl;
            z->value = value;
        }
        
        void update()
        {
            getOverlordsFeedBack();
            vector<Zone*> sortedList = zones;
            int i = 0;
            while(platinum >= POD_PRICE)
            {
                stable_sort(sortedList.begin(), sortedList.end(), zoneCompareLess);
                Zone* z = sortedList[i];
                purchasePod(1, z);
                
                if(isFirstTurn && (z->getIntend() >= z->links.size()))
                {
                    z->value = 0;
                }
                if(isFirstTurn && playerCount > 2 && z->getIntend() > 1)
                {
                    z->value = 0;
                }
                
                if(isFirstTurn && playerCount > 3 && z->getIntend() > 0)
                {
                    z->value = 0;
                }
                
                if(z->getIntend() > 2)
                {
                    z->value = 0;
                }
                
            }

        }
};

/***********************************************************************************************************

/** PATH FINTDING **/
/*
  Path finding with closure
*/
ZoneIntend* pathFinding(Zone* origin, function<bool (Zone*)> func)
{
    ZoneIntend* result = new ZoneIntend();
    
    vector<Zone*> blackList;
    vector<Zone*> whiteList;
    vector<Zone*> tempList;
    
    //Check the war case, unit cannot flee on ennemy zones.
    if(origin->hasEnemyPodOnIt())
    {
        for(Zone* z : origin->links)
        {
            if(z->isHostil())
            {
                blackList.push_back(z);
            }
        }
    }
    
    blackList.push_back(origin);
    whiteList.push_back(origin);
    
    //Maybe add a recursion limit, like 9.
    while(result->zone == nullptr && whiteList.size() != 0)
    {
        //cerr << "WhiteList " << whiteList.size() << endl;
        for(Zone* z : whiteList)
        {
            if (func(z))
            {
                result->zone = z;
                result->distance = 1;
                
                while(z->ancestor != origin)
                {
                    z = z->ancestor;
                    result->distance++;
                    result->zone = z;
                }
            }
            else
            {
                for(Zone* neighbour : z->links)
                {
                    if(!(find(blackList.begin(), blackList.end(), neighbour)!=blackList.end()))
                    {
                        neighbour->ancestor = z;
                        tempList.push_back(neighbour);
                    }
                }
            }
        }
        blackList.insert(blackList.end(), tempList.begin(), tempList.end());
        whiteList = tempList;
        tempList.clear();
    }
        
    return result;
}

/** COMMANDS **/
struct Move {
    int podsCount;
    Zone* zoneOrigin;
    Zone* zoneDestination;
};

void addMove(int podsCount, Zone* zoneOrigin, Zone* zoneDestination)
{
    Move m;
    m.podsCount = podsCount;
    m.zoneOrigin = zoneOrigin;
    m.zoneDestination = zoneDestination;
    zoneOrigin->addIntend(-1);
    zoneDestination->addIntend(1);
    
    moves.push_back(m);
}

struct Create {
    int podsCount;
    Zone* zoneDestination;
};

void addCreate(int podsCount, Zone* zoneDestination)
{
    Create c;
    c.podsCount = podsCount;
    c.zoneDestination = zoneDestination;
    zoneDestination->addIntend(1);
    zoneDestination->continent->intends ++;
    
    creates.push_back(c);
}

/** INITIALIZATION **/
// get the information given by the program at beginning
void initOvermind()
{
    overmind = new Overmind();
    
    cin >> overmind->playerCount >> overmind->myId >> overmind->zoneCount >> overmind->linkCount; cin.ignore();
    
    for (int i = 0; i < overmind->zoneCount; i++) {
        Zone* z = new Zone(overmind->myId);
        cin >> z->id >> z->platinum; cin.ignore();
        zones.push_back(z);
        
        //cerr << "zone created : " << z->id << ", " << z->platinum << endl;
    }
    for (int i = 0; i < overmind->linkCount; i++) {
        int zone1;
        int zone2;
        cin >> zone1 >> zone2; cin.ignore();
        zones[zone1]->links.push_back(zones[zone2]);
        zones[zone2]->links.push_back(zones[zone1]);
    }
}

//CONTINENTS
//recurcive function to explore and identify continents.
void rec_continent(Continent* c, Zone* z) 
{
    z->continent = c;
    c->myZones.push_back(z);
    c->platinum += z->platinum;
    z->blacklisted = true;
    
    for(Zone* neighbour : z->links)
    {
        if(!neighbour->blacklisted)
        {
            rec_continent(c, neighbour);
        }
    }
}

//Initialize continents.
void initContinents()
{
    int indexContinent = 0;
    for(Zone* z : zones){
        if(!z->blacklisted)
        {
            Continent* c = new Continent(indexContinent);
            rec_continent(c, z);
            continents.push_back(c);
            
            overmind->spawnOverlord(c);
            
            indexContinent++;
        }
    }
    cerr << "Continents : " << endl;
    for(Continent* c : continents)
    {
        cerr << "name : " << c->name << endl;
        cerr << "    size     : " << c->getSize() << endl;
        cerr << "    platinum : " << c->platinum << endl;
        c->computeWealthConcentration();
        cerr << "    wealConc : " << c->wealthConcentration << endl;
        c->computeValue();
        cerr << "   Value     : " << c->value << endl;
        overmind->worldValue += c->value;
        cerr << endl;
    }
    cerr << "----------------" << endl;
    cerr << "World Value : " << overmind->worldValue << endl; 
    cerr << endl;
}


/** UPDATE **/
//UPDATE COMMANDS
void updateCommands()
{
    //Move commands
    stringstream command;
    
    if(moves.size() > 0){
        for (const auto& m : moves)
        {
            command << m.podsCount << " " << m.zoneOrigin->id << " " << m.zoneDestination->id << " ";
        }
    }
    else{
        command <<  "WAIT";
    }

    cout << command.str() << endl;
    
    //Create commands
    command.str("");
    
    if(creates.size() > 0){
        for (const auto& c : creates)
        {
            command << c.podsCount << " " << c.zoneDestination->id << " ";
        }
    }
    else{
        command << "WAIT";
    }

    cout << command.str() << endl;
}

//UPDATE PLATINUM
void updatePlatinum()
{
    cin >> overmind->platinum; cin.ignore();
}

//UPDATE ZONES
void updateZones()
{
    for (int i = 0; i < overmind->zoneCount; i++) {
        Zone* z = zones[i];

        //In a worry of simplicity, pods are sorted.
        switch(overmind->myId)
        {
            case(0):
                cin >> z->id >> z->owner >> z->myPods >> z->p1 >> z->p2 >> z->p3; cin.ignore();
            break;
            case(1):
                cin >> z->id >> z->owner >> z->p1 >> z->myPods >> z->p2 >> z->p3; cin.ignore();
            break;
            case (2):
                cin >> z->id >> z->owner >> z->p1 >> z->p2 >> z->myPods >> z->p3; cin.ignore();
            break;
            default:
                cin >> z->id >> z->owner >> z->p1 >> z->p2 >> z->p3 >> z->myPods; cin.ignore();
            break;
        }
        if(z->myPods > 0){
            for(int j = 0; j < z->myPods ; j++){
                Pod* p = new Pod(z);
                pods.push_back(p);
            }
        };
        z->continent->p1 += z->p1;
        z->continent->p2 += z->p2;
        z->continent->p3 += z->p3;
        z->continent->myPods += z->myPods;
        z->clearIntend();
        
        overmind->setZoneValue(z);
    }
    for(Zone* z : zones)
    {
        z->podDanger = 0;
        for(Zone* l: z->links)
        {
            z->podDanger += max(0, (l->getMaxEnemyPod() - l->myPods));
        }
    }
}

//UPDATE OVERMIND
void updateOvermind()
{
    overmind->update();
}

//UPDATE PODS
void updatePods()
{
    
    for(Pod* p : pods)
    {
        //clock_t start;
        //start = clock();
    
        
        p->update();
        if(!p->currentZone->continent->isIgnored() && p->lastMood != nullptr && p->intend != nullptr){
            cerr << "pod : " << p->currentZone->id << ", choosen mood " << p->lastMood->getName() << ", go in " << p->intend->id << endl;;
        }
        
        /*int duration = ((clock() - start ) * 1000 )/ ((double)CLOCKS_PER_SEC);
        if(duration > 10)
        {
            cerr << "- Debug Pod : " << p->currentZone->id << endl;
            for(string s : p->debug){
                cerr << s << endl;
            }
        }*/
    }
    
    
}

/** CLEAR **/
void clear()
{
    moves.clear();
    creates.clear();
    pods.clear();
    for(Continent* c : continents)
    {
        c->clearPods();
        c->clearIntends();
    }
}

/** MAIN **/
int main()
{
    initOvermind();
    initContinents();
    
    // game loop
    while (1) {
        clock_t start;
        start = clock();
        
        updatePlatinum();
        updateZones();
        updateOvermind();
        updatePods();
        updateCommands();
        
        clear();
        
        int duration = ((clock() - start ) * 1000 )/ ((double)CLOCKS_PER_SEC);
        cerr << "Time Game Loop : " << duration  << "ms" << endl;
    }
}


