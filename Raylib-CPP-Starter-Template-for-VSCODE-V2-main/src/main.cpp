#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;

// ─── Palette ──────────────────────────────────────────────────────────────────
#define COL_BG       CLITERAL(Color){10, 8, 20, 255}
#define COL_PANEL    CLITERAL(Color){22, 18, 45, 255}
#define COL_BORDER   CLITERAL(Color){80, 60, 160, 255}
#define COL_ACCENT   CLITERAL(Color){180, 100, 255, 255}
#define COL_GOLD     CLITERAL(Color){255, 200, 80, 255}
#define COL_RED      CLITERAL(Color){255, 70, 90, 255}
#define COL_GREEN    CLITERAL(Color){80, 255, 160, 255}
#define COL_BLUE     CLITERAL(Color){80, 180, 255, 255}
#define COL_ORANGE   CLITERAL(Color){255, 140, 50, 255}
#define COL_CYAN     CLITERAL(Color){80, 220, 255, 255}
#define COL_WHITE    WHITE
#define COL_DIMWHITE CLITERAL(Color){200, 190, 220, 255}
#define COL_HP_BAR   CLITERAL(Color){60, 200, 100, 255}
#define COL_MP_BAR   CLITERAL(Color){60, 120, 255, 255}
#define COL_HP_LOW   CLITERAL(Color){255, 60, 60, 255}
#define COL_P1       CLITERAL(Color){255, 160, 60, 255}
#define COL_P2       CLITERAL(Color){80, 200, 255, 255}

// ─── BattleLogger ─────────────────────────────────────────────────────────────
class BattleLogger {
    ofstream logFile;
public:
    BattleLogger(const string& fname) {
        logFile.open(fname);
        if (!logFile.is_open()) throw runtime_error("Failed to open log file: " + fname);
        logFile << "=== BATTLE LOG ===\n";
    }
    void log(const string& msg) { if (logFile.is_open()) logFile << msg << "\n"; }
    ~BattleLogger() {
        if (logFile.is_open()) { logFile << "=== End of Log ===\n"; logFile.close(); }
    }
};

// ─── StatusEffect ─────────────────────────────────────────────────────────────
struct StatusEffect {
    string name;
    int damagePerTurn, duration;
    bool isStun;
    StatusEffect(const string& n, int dmg, int dur, bool stun)
        : name(n), damagePerTurn(dmg), duration(dur), isStun(stun) {}
    bool isExpired() const { return duration <= 0; }
};

// ─── Character (base) ─────────────────────────────────────────────────────────
class Character {
protected:
    string name;
    float  health, maxHealth;
    float  energy, maxEnergy;
    int    attackPower, defense, speed;
    StatusEffect* activeEffect = nullptr;
public:
    Character(const string& n, float h, float e, int atk, int def, int spd)
        : name(n), health(h), maxHealth(h), energy(e), maxEnergy(e),
          attackPower(atk), defense(def), speed(spd) {}
    virtual ~Character() { delete activeEffect; }

    string getName()       const { return name; }
    float  getHealth()     const { return health; }
    float  getMaxHealth()  const { return maxHealth; }
    float  getEnergy()     const { return energy; }
    float  getMaxEnergy()  const { return maxEnergy; }
    int    getAttackPower()const { return attackPower; }
    int    getDefense()    const { return defense; }
    int    getSpeed()      const { return speed; }

    void setHealth(float h){ health = (h<0)?0:h; }
    void setEnergy(float e){ energy = (e<0)?0:e; }

    void applyStatusEffect(StatusEffect* eff){ delete activeEffect; activeEffect = eff; }

    string processStatusEffect(){
        if(!activeEffect) return "";
        if(activeEffect->isExpired()){
            string msg = activeEffect->name + " wore off on " + name + "!";
            delete activeEffect; activeEffect = nullptr;
            return msg;
        }
        string msg;
        if(!activeEffect->isStun){
            float dmg = activeEffect->damagePerTurn;
            health -= dmg; if(health<0) health=0;
            msg = name + " takes " + to_string((int)dmg) + " dmg from " + activeEffect->name + "!";
        } else {
            msg = name + " is still stunned!";
        }
        activeEffect->duration--;
        return msg;
    }

    bool   isStunned()    const { return activeEffect && activeEffect->isStun; }
    string getStatusName()const { return activeEffect ? activeEffect->name : ""; }
    virtual bool isAlive()const { return health > 0; }
    virtual string getClassName()const = 0;
    virtual Color  getClassColor()const = 0;

    virtual string attack(Character* target){
        int dmg = attackPower - target->defense;
        if(dmg<0) dmg=0;
        target->setHealth(target->getHealth()-dmg);
        return name + " attacks " + target->getName() + " for " + to_string(dmg) + " dmg!";
    }
    virtual string useSkill(Character* target) = 0;
};

// ─── Warrior ──────────────────────────────────────────────────────────────────
class Warrior : public Character {
    int shieldBonus, skillCost;
public:
    Warrior(const string& n, float h, float e, int atk, int def, int spd, int sb, int sc)
        : Character(n,h,e,atk,def,spd), shieldBonus(sb), skillCost(sc) {}
    string getClassName()const override { return "Warrior"; }
    Color  getClassColor()const override{ return COL_ORANGE; }
    string useSkill(Character* target) override {
        if(energy < skillCost) return name+" has insufficient energy!";
        energy -= skillCost;
        int dmg = 15 + shieldBonus;
        target->applyStatusEffect(new StatusEffect("Stun",0,1,true));
        target->setHealth(target->getHealth()-dmg);
        return name+" uses Shield Bash on "+target->getName()+" for "+to_string(dmg)+" dmg! [STUNNED]";
    }
};

// ─── Mage ─────────────────────────────────────────────────────────────────────
class Mage : public Character {
    int spellPower;
public:
    Mage(const string& n, float h, float e, int atk, int def, int spd, int sp)
        : Character(n,h,e,atk,def,spd), spellPower(sp) {}
    string getClassName()const override { return "Mage"; }
    Color  getClassColor()const override{ return COL_ACCENT; }
    string useSkill(Character* target) override {
        if(energy < 30) return name+" has insufficient energy!";
        energy -= 30;
        int dmg = 30 + spellPower;
        target->applyStatusEffect(new StatusEffect("Poison",5,3,false));
        target->setHealth(target->getHealth()-dmg);
        return name+" casts Fireball on "+target->getName()+" for "+to_string(dmg)+" dmg! [POISONED]";
    }
};

// ─── Tank ─────────────────────────────────────────────────────────────────────
class Tank : public Character {
    int damageReduction;
public:
    Tank(const string& n, float h, float e, int atk, int def, int spd, int dr)
        : Character(n,h,e,atk,def,spd), damageReduction(dr) {}
    string getClassName()const override { return "Tank"; }
    Color  getClassColor()const override{ return COL_BLUE; }
    string useSkill(Character* /*target*/) override {
        if(energy < 15) return name+" has insufficient energy!";
        energy -= 15;
        int heal = 20 + damageReduction;
        setHealth(getHealth()+heal);
        return name+" uses Iron Wall! Restored "+to_string(heal)+" HP!";
    }
};

// ─── Enums ────────────────────────────────────────────────────────────────────
enum GameState  { STATE_MODE_SELECT, STATE_SETUP, STATE_BATTLE };
enum BattlePhase{ PHASE_P1_CHOOSE, PHASE_P2_CHOOSE, PHASE_ANIMATING, PHASE_BATTLE_OVER };

// ─── Log entry / Particle ─────────────────────────────────────────────────────
struct LogEntry { string text; Color color; };
struct Particle { Vector2 pos,vel; Color color; float life,maxLife,size; bool active=false; };

// ─── Globals ──────────────────────────────────────────────────────────────────
static const int SW=1200, SH=760;
static const int MAX_LOG=14, MAX_PART=300;

static GameState   gState = STATE_MODE_SELECT;
static BattlePhase gPhase = PHASE_P1_CHOOSE;
static bool        gVsAI  = true;   // true = vs computer, false = vs player 2

// Setup screen state
static int    gSetupStep   = 0;    // 0=P1 class, 1=P1 name, 2=P2 class(2P only), 3=P2 name(2P only)
static int    gP1Class=-1, gP2Class=-1;
static string gP1Name="",  gP2Name="";
static bool   gCursorVis=true;
static double gCursorTimer=0;
static int    gActiveInputField=0; // which name field is active

static Character* gP1=nullptr;
static Character* gP2=nullptr;
static BattleLogger* gLogger=nullptr;

static int   gRound=1;
static float gAnimTimer=0;
static float gShakeTimer=0;
static Vector2 gShakeOff={0,0};
static float gP1Flash=0, gP2Flash=0;
static string gResultMsg="";

static vector<LogEntry> gLog;
static Particle gPart[MAX_PART];

// ─── Helpers ──────────────────────────────────────────────────────────────────
static void AddLog(const string& t, Color c=COL_DIMWHITE){
    if(t.empty()) return;
    if((int)gLog.size()>=MAX_LOG) gLog.erase(gLog.begin());
    gLog.push_back({t,c});
}

static void SpawnParticles(Vector2 o, Color c, int n=20){
    int s=0;
    for(int i=0;i<MAX_PART&&s<n;i++){
        if(!gPart[i].active){
            float a=((float)rand()/RAND_MAX)*6.28f;
            float spd=80+((float)rand()/RAND_MAX)*180;
            gPart[i]={o,{cosf(a)*spd,sinf(a)*spd},c,1.0f,1.0f,3+((float)rand()/RAND_MAX)*5,true};
            s++;
        }
    }
}

static void UpdateParticles(float dt){
    for(int i=0;i<MAX_PART;i++){
        if(!gPart[i].active) continue;
        gPart[i].pos.x+=gPart[i].vel.x*dt;
        gPart[i].pos.y+=gPart[i].vel.y*dt;
        gPart[i].vel.y+=200*dt;
        gPart[i].life-=dt;
        if(gPart[i].life<=0) gPart[i].active=false;
    }
}

static void DrawParticles(){
    for(int i=0;i<MAX_PART;i++){
        if(!gPart[i].active) continue;
        float a=gPart[i].life/gPart[i].maxLife;
        Color c=gPart[i].color; c.a=(unsigned char)(a*255);
        DrawCircleV(gPart[i].pos,gPart[i].size*a,c);
    }
}

static void DrawPanel(float x,float y,float w,float h,Color fill,Color border){
    DrawRectangleRounded({x+1,y+1,w-2,h-2},0.07f,8,fill);
    DrawRectangleRoundedLines({x,y,w,h},0.07f,8,1.5f,border);
}

static void DrawBar(float x,float y,float w,float h,float val,float maxV,Color fg,Color bg){
    DrawRectangleRounded({x,y,w,h},0.5f,6,bg);
    float r=(maxV>0)?(val/maxV):0;
    if(r>1)r=1; if(r<0)r=0;
    if(r>0) DrawRectangleRounded({x,y,w*r,h},0.5f,6,fg);
}

static void DrawTextC(const char* t,float cx,float y,int sz,Color c){
    DrawText(t,(int)(cx-MeasureText(t,sz)/2),(int)y,sz,c);
}

static bool DrawButton(const char* lbl,float x,float y,float w,float h,
                        Color bg,Color hbg,Color tc=COL_WHITE){
    Rectangle r={x,y,w,h};
    bool hov=CheckCollisionPointRec(GetMousePosition(),r);
    DrawRectangleRounded(r,0.3f,6,hov?hbg:bg);
    DrawRectangleRoundedLines(r,0.3f,6,1.5f,hov?COL_WHITE:COL_BORDER);
    int tw=MeasureText(lbl,16);
    DrawText(lbl,(int)(x+w/2-tw/2),(int)(y+h/2-8),16,tc);
    return hov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ─── Create character by class id ─────────────────────────────────────────────
static Character* MakeCharacter(int classId, const string& name){
    if(classId==1) return new Warrior(name,150,60,30,15,12,10,20);
    if(classId==2) return new Mage(name,100,120,25,8,15,20);
    return             new Tank(name,200,50,20,20,8,15);
}

static const char* SkillName(int classId){
    if(classId==1) return "SHIELD BASH";
    if(classId==2) return "FIREBALL";
    return "IRON WALL";
}

// ─── Reset ────────────────────────────────────────────────────────────────────
static void ResetAll(){
    delete gP1; gP1=nullptr;
    delete gP2; gP2=nullptr;
    delete gLogger; gLogger=nullptr;
    gLog.clear();
    gRound=1; gPhase=PHASE_P1_CHOOSE;
    gResultMsg="";
    gAnimTimer=0; gShakeTimer=0; gShakeOff={0,0};
    gP1Flash=0; gP2Flash=0;
    gSetupStep=0; gP1Class=-1; gP2Class=-1;
    gP1Name=""; gP2Name="";
    for(int i=0;i<MAX_PART;i++) gPart[i].active=false;
    gState=STATE_MODE_SELECT;
}

// ─── Execute one character's action against a target ─────────────────────────
// attacker=0 means P1, attacker=1 means P2
static void ExecuteAction(int attacker, bool useSkill){
    Character* src  = (attacker==0)?gP1:gP2;
    Character* dest = (attacker==0)?gP2:gP1;
    Color srcColor  = (attacker==0)?COL_P1:COL_P2;
    float& destFlash= (attacker==0)?gP2Flash:gP1Flash;

    string msg;
    if(useSkill){
        msg=src->useSkill(dest);
        SpawnParticles((attacker==0)?Vector2{900,310}:Vector2{310,310}, src->getClassColor(),30);
        destFlash=0.9f;
    } else {
        msg=src->attack(dest);
        SpawnParticles((attacker==0)?Vector2{900,310}:Vector2{310,310}, srcColor,15);
        destFlash=0.5f;
    }
    gShakeTimer=0.3f;
    AddLog(msg, srcColor);
    if(gLogger) gLogger->log(msg);

    // process attacker's own status
    string se=src->processStatusEffect();
    if(!se.empty()){ AddLog(se,COL_GREEN); if(gLogger) gLogger->log(se); }

    // check if target is dead
    if(!dest->isAlive()){
        gResultMsg = src->getName()+" WINS!";
        gPhase=PHASE_BATTLE_OVER;
        if(gLogger) gLogger->log("RESULT: "+gResultMsg);
        AddLog("=== "+gResultMsg+" ===", (attacker==0)?COL_P1:COL_P2);
        SpawnParticles({SW/2.0f,SH/2.0f},COL_GOLD,100);
        return;
    }

    // advance turn
    if(attacker==0){
        gPhase=PHASE_P2_CHOOSE;
        gAnimTimer=gVsAI?0.7f:0.0f;
    } else {
        gRound++;
        gPhase=PHASE_P1_CHOOSE;
    }
}

// ─── AI decides and acts ──────────────────────────────────────────────────────
static void AIAct(){
    bool useSkill=(gP2->getEnergy()>=20);
    ExecuteAction(1,useSkill);
}

// ─── Handle stun-skip for a player ───────────────────────────────────────────
static void HandleStunSkip(int attacker){
    Character* src=(attacker==0)?gP1:gP2;
    Color col=(attacker==0)?COL_P1:COL_P2;
    string msg=src->getName()+" is stunned and skips their turn!";
    AddLog(msg,COL_GOLD);
    if(gLogger) gLogger->log(msg);
    string se=src->processStatusEffect();
    if(!se.empty()){ AddLog(se,COL_GREEN); if(gLogger) gLogger->log(se); }

    if(attacker==0){
        gPhase=PHASE_P2_CHOOSE;
        gAnimTimer=gVsAI?0.7f:0.0f;
    } else {
        gRound++;
        gPhase=PHASE_P1_CHOOSE;
    }
}

// ─── Start battle ─────────────────────────────────────────────────────────────
static void StartBattle(){
    gLogger=new BattleLogger("battle_log.txt");
    gLogger->log("Battle: "+gP1->getName()+" vs "+gP2->getName());
    gLog.clear();
    gRound=1;
    AddLog("=== BATTLE BEGIN ===",COL_GOLD);
    AddLog(gP1->getName()+" (P1)  vs  "+gP2->getName()+(gVsAI?" (AI)":" (P2)"),COL_ACCENT);
    bool p1First=gP1->getSpeed()>=gP2->getSpeed();
    if(!p1First){ gPhase=PHASE_P2_CHOOSE; gAnimTimer=gVsAI?0.7f:0.0f; }
    else          gPhase=PHASE_P1_CHOOSE;
    AddLog((p1First?gP1->getName():gP2->getName())+" goes first!",COL_GOLD);
}

// ─── Draw Character Card ──────────────────────────────────────────────────────
static void DrawCharCard(Character* c,float x,float y,float w,float h,
                          float flashVal, Color playerBadge, const char* playerLabel,
                          bool shakeLeft){
    float sx=0,sy=0;
    if(gShakeTimer>0){ sx=shakeLeft?-gShakeOff.x:gShakeOff.x; sy=gShakeOff.y; }
    x+=sx; y+=sy;

    Color border=c->getClassColor();
    if(flashVal>0){
        unsigned char fv=(unsigned char)(flashVal*255);
        border={255,fv,fv,255};
    }
    DrawPanel(x,y,w,h,COL_PANEL,border);

    // player badge (P1 / P2 / AI)
    int pbw=MeasureText(playerLabel,13)+12;
    DrawRectangleRounded({x+w-pbw-8,y+8,(float)pbw,22},0.4f,6,playerBadge);
    DrawText(playerLabel,(int)(x+w-pbw-2),(int)(y+11),13,COL_BG);

    // class badge
    const char* cls=c->getClassName().c_str();
    int cbw=MeasureText(cls,13)+12;
    DrawRectangleRounded({x+8,y+8,(float)cbw,22},0.4f,6,c->getClassColor());
    DrawText(cls,(int)(x+14),(int)(y+11),13,COL_BG);

    // name
    DrawText(c->getName().c_str(),(int)(x+10),(int)(y+38),22,COL_WHITE);

    // status badge
    string se=c->getStatusName();
    if(!se.empty()){
        Color sc=(se=="Poison")?COL_GREEN:COL_GOLD;
        const char* scc=se.c_str();
        int sw2=MeasureText(scc,12)+10;
        DrawRectangleRounded({x+10,y+h-30,(float)sw2,20},0.4f,6,sc);
        DrawText(scc,(int)(x+15),(int)(y+h-27),12,COL_BG);
    }

    // HP bar
    float bx=x+10,by=y+68,bw=w-20,bh=14;
    float ratio=c->getHealth()/c->getMaxHealth();
    Color hpc=(ratio<0.3f)?COL_HP_LOW:COL_HP_BAR;
    DrawBar(bx,by,bw,bh,c->getHealth(),c->getMaxHealth(),hpc,{40,20,20,255});
    DrawText(TextFormat("HP  %d / %d",(int)c->getHealth(),(int)c->getMaxHealth()),
             (int)bx,(int)(by+17),12,COL_DIMWHITE);

    // EN bar
    float ey=by+34;
    DrawBar(bx,ey,bw,bh,c->getEnergy(),c->getMaxEnergy(),COL_MP_BAR,{20,20,50,255});
    DrawText(TextFormat("EN  %d / %d",(int)c->getEnergy(),(int)c->getMaxEnergy()),
             (int)bx,(int)(ey+17),12,COL_DIMWHITE);

    // stats
    float sy2=y+h-58;
    DrawText(TextFormat("ATK %d",c->getAttackPower()),(int)(x+10),(int)sy2,13,COL_DIMWHITE);
    DrawText(TextFormat("DEF %d",c->getDefense()),(int)(x+10),(int)(sy2+18),13,COL_DIMWHITE);
    DrawText(TextFormat("SPD %d",c->getSpeed()),(int)(x+w/2),(int)sy2,13,COL_DIMWHITE);

    if(c->isStunned()){
        float p=(sinf(GetTime()*6)+1)*0.5f;
        Color sc={255,220,0,(unsigned char)(180+75*p)};
        DrawTextC("STUNNED",(int)(x+w/2),(int)(y+h/2-10),18,sc);
    }
}

// ─── Draw Combat Log ──────────────────────────────────────────────────────────
static void DrawLog(float x,float y,float w,float h){
    DrawPanel(x,y,w,h,COL_PANEL,COL_BORDER);
    DrawText("COMBAT LOG",(int)(x+12),(int)(y+8),14,COL_ACCENT);
    DrawLine((int)(x+10),(int)(y+26),(int)(x+w-10),(int)(y+26),COL_BORDER);
    float lh=(h-34.0f)/MAX_LOG;
    for(int i=0;i<(int)gLog.size();i++){
        float alpha=(float)(i+1)/gLog.size();
        Color c=gLog[i].color; c.a=(unsigned char)(alpha*220);
        DrawText(gLog[i].text.c_str(),(int)(x+12),(int)(y+32+i*lh),12,c);
    }
}

// ─── Draw Action Panel for a player ──────────────────────────────────────────
// Returns true if the player took an action this frame
static bool DrawActionPanel(float x,float y,float w,float h,
                             int playerIdx, bool isAITurn, float dt){
    Character* actor =(playerIdx==0)?gP1:gP2;
    Color pCol       =(playerIdx==0)?COL_P1:COL_P2;
    const char* pTag =(playerIdx==0)?"PLAYER 1 TURN":"PLAYER 2 TURN";
    if(isAITurn&&playerIdx==1) pTag="AI TURN";
    int classId      =(playerIdx==0)?gP1Class:gP2Class;

    DrawPanel(x,y,w,h,COL_PANEL,pCol);

    // header strip
    DrawRectangleRounded({x,y,(float)w,38},0.07f,6,
        {(unsigned char)(pCol.r/3),(unsigned char)(pCol.g/3),(unsigned char)(pCol.b/3),255});
    DrawTextC(pTag,(float)(x+w/2),(float)(y+10),16,pCol);
    DrawLine((int)(x+10),(int)(y+38),(int)(x+w-10),(int)(y+38),pCol);

    // If AI turn
    if(isAITurn&&playerIdx==1){
        DrawRectangleRounded({x+15,y+50,(float)(w-30),60},0.3f,6,{20,18,40,180});
        DrawTextC("Thinking...",(float)(x+w/2),(float)(y+68),18,COL_DIMWHITE);
        float angle=(float)GetTime()*180;
        DrawCircleSector({(float)(x+w/2),(float)(y+145)},18,angle,angle+240,8,COL_ACCENT);
        gAnimTimer-=dt;
        if(gAnimTimer<=0) AIAct();
        return false;
    }

    // Stun skip
    if(actor->isStunned()){
        DrawRectangleRounded({x+15,y+50,(float)(w-30),60},0.3f,6,{60,50,0,180});
        DrawTextC("STUNNED — TURN SKIPPED",(float)(x+w/2),(float)(y+68),14,COL_GOLD);
        gAnimTimer-=dt;
        if(gAnimTimer<=0){
            gAnimTimer=0.5f;
            HandleStunSkip(playerIdx);
        }
        return false;
    }

    // Buttons
    if(DrawButton("  BASIC ATTACK  ",x+15,y+52,w-30,52,
                  {60,20,20,255},{120,30,30,255})){
        ExecuteAction(playerIdx,false);
        return true;
    }

    string sklabel=string("  SKILL: ")+SkillName(classId)+"  ";
    if(DrawButton(sklabel.c_str(),x+15,y+116,w-30,52,
                  {30,20,80,255},{70,40,180,255})){
        ExecuteAction(playerIdx,true);
        return true;
    }

    DrawText(TextFormat("Energy: %.0f / %.0f",actor->getEnergy(),actor->getMaxEnergy()),
             (int)(x+20),(int)(y+182),13,COL_DIMWHITE);

    // keyboard shortcuts
    if(IsKeyPressed(KEY_ONE)||IsKeyPressed(KEY_KP_1)){
        ExecuteAction(playerIdx,false); return true;
    }
    if(IsKeyPressed(KEY_TWO)||IsKeyPressed(KEY_KP_2)){
        ExecuteAction(playerIdx,true);  return true;
    }

    DrawText("[1] Attack  [2] Skill",(int)(x+18),(int)(y+h-30),13,COL_BORDER);
    return false;
}

// ─── Draw battle screen ───────────────────────────────────────────────────────
static void DrawBattle(float dt){
    // hex bg
    for(int gy=-1;gy<9;gy++)
    for(int gx=-1;gx<16;gx++){
        float hx=gx*80+(gy%2)*40, hy=gy*70+60;
        DrawPoly({hx,hy},6,36,30,{40,30,70,25});
    }

    // shake
    if(gShakeTimer>0){
        gShakeTimer-=dt;
        float mag=gShakeTimer*10;
        gShakeOff={(float)((rand()%2?1:-1)*((float)rand()/RAND_MAX)*mag),
                   (float)((rand()%2?1:-1)*((float)rand()/RAND_MAX)*mag)};
    } else gShakeOff={0,0};

    if(gP1Flash>0){ gP1Flash-=dt*3; if(gP1Flash<0) gP1Flash=0; }
    if(gP2Flash>0){ gP2Flash-=dt*3; if(gP2Flash<0) gP2Flash=0; }

    // top bar
    DrawRectangle(0,0,SW,50,{15,10,30,230});
    DrawTextC(TextFormat("ROUND  %d",gRound),SW/2,12,24,COL_GOLD);

    // character cards
    float cardW=360, cardH=270;
    DrawCharCard(gP1,15,58,cardW,cardH,gP1Flash,COL_P1,"P1",false);
    DrawCharCard(gP2,SW-cardW-15,58,cardW,cardH,gP2Flash,
                 gVsAI?CLITERAL(Color){200,60,60,255}:COL_P2,
                 gVsAI?"AI":"P2",true);

    // VS
    float vsp=0.9f+0.1f*sinf(GetTime()*2);
    DrawTextC("VS",SW/2,165,(int)(36*vsp),COL_ACCENT);

    // mode badge
    const char* modeLbl=gVsAI?"VS COMPUTER":"VS PLAYER 2";
    DrawTextC(modeLbl,SW/2,60,14,COL_DIMWHITE);

    // log
    float logW=480, logH=270, logX=SW/2-logW/2, logY=340;
    DrawLog(logX,logY,logW,logH);

    // action panels
    float apW=310, apH=270, apY=340;
    float apX_left=15, apX_right=SW-apW-15;

    if(gPhase==PHASE_BATTLE_OVER){
        // full-width result panel
        bool p1won=gP1->isAlive();
        Color rc=p1won?COL_P1:COL_P2;
        float pulse=(sinf(GetTime()*3)+1)*0.5f;

        DrawPanel(apX_left,apY,apW,apH,COL_PANEL,rc);
        DrawTextC(gResultMsg.c_str(),(float)(apX_left+apW/2),(float)(apY+40),(int)(26+4*pulse),rc);
        DrawTextC("Battle Complete!",(float)(apX_left+apW/2),(float)(apY+86),14,COL_DIMWHITE);
        if(DrawButton("  PLAY AGAIN  ",apX_left+20,apY+120,apW-40,48,
                       {50,30,90,255},{100,60,180,255})) { ResetAll(); }
        if(DrawButton("  MAIN MENU  ",apX_left+20,apY+180,apW-40,48,
                       {30,20,60,255},{60,40,120,255})) { ResetAll(); }

        DrawPanel(apX_right,apY,apW,apH,COL_PANEL,rc);
        DrawTextC("FINAL STATS",(float)(apX_right+apW/2),(float)(apY+16),16,COL_ACCENT);
        DrawText(TextFormat("%s: %.0f HP left",gP1->getName().c_str(),gP1->getHealth()),
                 (int)(apX_right+15),(int)(apY+50),14,COL_P1);
        DrawText(TextFormat("%s: %.0f HP left",gP2->getName().c_str(),gP2->getHealth()),
                 (int)(apX_right+15),(int)(apY+72),14,gVsAI?CLITERAL(Color){200,60,60,255}:COL_P2);
        DrawText(TextFormat("Rounds fought: %d",gRound),
                 (int)(apX_right+15),(int)(apY+105),14,COL_DIMWHITE);
    }
    else if(gPhase==PHASE_P1_CHOOSE){
        DrawActionPanel(apX_left,apY,apW,apH,0,false,dt);
        // right panel just shows "waiting"
        DrawPanel(apX_right,apY,apW,apH,COL_PANEL,COL_BORDER);
        DrawTextC("Waiting...",(float)(apX_right+apW/2),(float)(apY+apH/2-10),16,COL_BORDER);
    }
    else if(gPhase==PHASE_P2_CHOOSE){
        // left panel shows "waiting"
        DrawPanel(apX_left,apY,apW,apH,COL_PANEL,COL_BORDER);
        DrawTextC("Waiting...",(float)(apX_left+apW/2),(float)(apY+apH/2-10),16,COL_BORDER);
        DrawActionPanel(apX_right,apY,apW,apH,1,gVsAI,dt);
    }

    UpdateParticles(dt);
    DrawParticles();

    // bottom bar
    DrawRectangle(0,SH-34,SW,34,{10,8,20,220});
    DrawText("Mouse click buttons  |  [1] Attack  [2] Skill  |  Turn-Based RPG Battle",
             18,SH-23,13,COL_BORDER);
}

// ─── Class selection sub-widget ───────────────────────────────────────────────
// Returns true if a class was just clicked
static bool DrawClassPicker(float startX,float startY,int& selectedClass,Color accentCol){
    struct CInfo{ const char* name; const char* d1; const char* d2; Color col; int id; };
    CInfo cls[]={
        {"WARRIOR","High ATK/DEF","Shield Bash + Stun", COL_ORANGE,1},
        {"MAGE",   "High Magic", "Fireball + Poison",  COL_ACCENT,2},
        {"TANK",   "High HP",    "Iron Wall + Heal",   COL_BLUE,  3},
    };
    float cw=200,ch=170,gap=20;
    bool changed=false;
    for(int i=0;i<3;i++){
        float cx=startX+i*(cw+gap);
        float cy=startY;
        bool hov=CheckCollisionPointRec(GetMousePosition(),{cx,cy,cw,ch});
        float lift=hov?-5.0f:0.0f;
        bool sel=(selectedClass==cls[i].id);
        Color border=sel?cls[i].col:(hov?cls[i].col:COL_BORDER);
        DrawPanel(cx,cy+lift,cw,ch,COL_PANEL,border);
        if(sel){
            DrawRectangleRounded({cx,cy+lift,cw,ch},0.07f,8,
                {cls[i].col.r,cls[i].col.g,cls[i].col.b,30});
        }
        DrawTextC(cls[i].name,(float)(cx+cw/2),(float)(cy+lift+12),15,cls[i].col);
        DrawTextC(cls[i].d1, (float)(cx+cw/2),(float)(cy+lift+40),12,COL_DIMWHITE);
        DrawTextC(cls[i].d2, (float)(cx+cw/2),(float)(cy+lift+58),12,COL_DIMWHITE);
        if(sel){
            DrawRectangleRounded({cx+15,cy+lift+ch-30,cw-30,22},0.4f,6,cls[i].col);
            DrawTextC("SELECTED",(float)(cx+cw/2),(float)(cy+lift+ch-26),12,COL_BG);
        }
        if(hov&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            selectedClass=cls[i].id; changed=true;
        }
    }
    return changed;
}

// Draw a name input box; returns true if text changed
static bool DrawNameInput(float x,float y,float w,float h,string& name,bool active,Color accentCol){
    DrawRectangleRounded({x,y,w,h},0.2f,6,{30,25,55,255});
    DrawRectangleRoundedLines({x,y,w,h},0.2f,6,1.5f,active?accentCol:COL_BORDER);
    gCursorTimer+=GetFrameTime();
    if(gCursorTimer>0.5f){ gCursorVis=!gCursorVis; gCursorTimer=0; }
    string disp=name+(active&&gCursorVis?"|":" ");
    DrawText(disp.c_str(),(int)(x+10),(int)(y+h/2-10),18,active?COL_WHITE:COL_DIMWHITE);
    if(active){
        int k=GetCharPressed();
        while(k>0){
            if(k>=32&&k<=125&&(int)name.size()<14) name+=(char)k;
            k=GetCharPressed();
        }
        if(IsKeyPressed(KEY_BACKSPACE)&&!name.empty()) name.pop_back();
        return true;
    }
    return false;
}

// ─── Setup Screen ─────────────────────────────────────────────────────────────
static void DrawSetup(){
    // stars
    srand(99);
    for(int i=0;i<100;i++){
        float sx=(float)(rand()%SW), sy=(float)(rand()%SH);
        float a=0.3f+0.7f*sinf((float)GetTime()*0.6f+i*0.4f);
        DrawCircle((int)sx,(int)sy,1+(rand()%2)*0.5f,{200,180,255,(unsigned char)(a*200)});
    }

    // --- Step 0: P1 class select ---
    if(gSetupStep==0){
        DrawTextC("PLAYER 1 — CHOOSE YOUR CLASS",SW/2,40,26,COL_P1);
        DrawTextC("Select a class to continue",SW/2,75,16,COL_DIMWHITE);
        float pw=3*200+2*20;
        DrawClassPicker(SW/2-pw/2,120,gP1Class,COL_P1);
        if(gP1Class>0){
            float bw=240,bh=52,bx=SW/2-bw/2,by=320;
            float g=(sinf(GetTime()*2)+1)*0.5f;
            DrawRectangleRounded({bx-4,by-4,bw+8,bh+8},0.3f,6,
                {110,50,220,(unsigned char)(50+40*g)});
            if(DrawButton(" CONTINUE ",bx,by,bw,bh,{70,30,140,255},{110,50,220,255}))
                gSetupStep=1;
        }
    }

    // --- Step 1: P1 name ---
    else if(gSetupStep==1){
        DrawTextC("PLAYER 1 — ENTER YOUR NAME",SW/2,40,26,COL_P1);
        float nw=360,nh=50,nx=SW/2-nw/2,ny=120;
        DrawText("Name:",(int)(nx),(int)(ny-26),18,COL_DIMWHITE);
        DrawNameInput(nx,ny,nw,nh,gP1Name,true,COL_P1);

        bool canCont=!gP1Name.empty();
        if(canCont){
            float bw=240,bh=52,bx=SW/2-bw/2,by=220;
            float g=(sinf(GetTime()*2)+1)*0.5f;
            DrawRectangleRounded({bx-4,by-4,bw+8,bh+8},0.3f,6,
                {110,50,220,(unsigned char)(50+40*g)});
            if(DrawButton(" CONTINUE ",bx,by,bw,bh,{70,30,140,255},{110,50,220,255})||
               IsKeyPressed(KEY_ENTER)){
                if(!gVsAI) gSetupStep=2;
                else {
                    // AI opponent — auto-create P2
                    gP2Class=1+rand()%3;
                    gP2Name="CPU";
                    gP1=MakeCharacter(gP1Class,gP1Name);
                    gP2=MakeCharacter(gP2Class,gP2Name);
                    StartBattle();
                    gState=STATE_BATTLE;
                }
            }
        }
        if(DrawButton(" BACK ",SW/2-60,350,120,40,{40,30,70,255},{70,50,120,255}))
            gSetupStep=0;
    }

    // --- Step 2: P2 class (2P only) ---
    else if(gSetupStep==2){
        DrawTextC("PLAYER 2 — CHOOSE YOUR CLASS",SW/2,40,26,COL_P2);
        DrawTextC("Select a class to continue",SW/2,75,16,COL_DIMWHITE);
        float pw=3*200+2*20;
        DrawClassPicker(SW/2-pw/2,120,gP2Class,COL_P2);
        if(gP2Class>0){
            float bw=240,bh=52,bx=SW/2-bw/2,by=320;
            float g=(sinf(GetTime()*2)+1)*0.5f;
            DrawRectangleRounded({bx-4,by-4,bw+8,bh+8},0.3f,6,
                {80,180,255,(unsigned char)(50+40*g)});
            if(DrawButton(" CONTINUE ",bx,by,bw,bh,{30,60,120,255},{50,120,200,255}))
                gSetupStep=3;
        }
        if(DrawButton(" BACK ",SW/2-60,400,120,40,{40,30,70,255},{70,50,120,255}))
            gSetupStep=1;
    }

    // --- Step 3: P2 name (2P only) ---
    else if(gSetupStep==3){
        DrawTextC("PLAYER 2 — ENTER YOUR NAME",SW/2,40,26,COL_P2);
        float nw=360,nh=50,nx=SW/2-nw/2,ny=120;
        DrawText("Name:",(int)(nx),(int)(ny-26),18,COL_DIMWHITE);
        DrawNameInput(nx,ny,nw,nh,gP2Name,true,COL_P2);

        bool canStart=!gP2Name.empty();
        if(canStart){
            float bw=240,bh=52,bx=SW/2-bw/2,by=220;
            float g=(sinf(GetTime()*2)+1)*0.5f;
            DrawRectangleRounded({bx-4,by-4,bw+8,bh+8},0.3f,6,
                {80,180,255,(unsigned char)(50+40*g)});
            if(DrawButton(" START BATTLE ",bx,by,bw,bh,{30,60,120,255},{50,120,200,255})||
               IsKeyPressed(KEY_ENTER)){
                gP1=MakeCharacter(gP1Class,gP1Name);
                gP2=MakeCharacter(gP2Class,gP2Name);
                StartBattle();
                gState=STATE_BATTLE;
            }
        }
        if(DrawButton(" BACK ",SW/2-60,350,120,40,{40,30,70,255},{70,50,120,255}))
            gSetupStep=2;
    }
}

// ─── Mode Select Screen ───────────────────────────────────────────────────────
static void DrawModeSelect(){
    srand(77);
    for(int i=0;i<120;i++){
        float sx=(float)(rand()%SW),sy=(float)(rand()%SH);
        float a=0.3f+0.7f*sinf((float)GetTime()*0.5f+i*0.35f);
        DrawCircle((int)sx,(int)sy,1+(rand()%2)*0.5f,{180,160,255,(unsigned char)(a*180)});
    }

    DrawTextC("TURN-BASED RPG BATTLE",SW/2,55,52,COL_ACCENT);
    DrawTextC("ARENA",SW/2,115,28,COL_GOLD);
    DrawRectangle(SW/2-220,155,440,2,COL_BORDER);
    DrawTextC("SELECT MODE",SW/2,172,20,COL_DIMWHITE);

    // Mode cards
    float cw=440,ch=220,gap=60;
    float totalW=cw*2+gap;
    float startX=SW/2-totalW/2;

    // VS Computer
    {
        float cx=startX,cy=210;
        bool hov=CheckCollisionPointRec(GetMousePosition(),{cx,cy,cw,ch});
        Color border=hov?COL_ORANGE:COL_BORDER;
        DrawPanel(cx,cy,cw,ch,COL_PANEL,border);
        if(hov) DrawRectangleRounded({cx,cy,cw,ch},0.07f,8,{255,140,50,15});

        DrawTextC("VS COMPUTER",(float)(cx+cw/2),(float)(cy+20),24,COL_ORANGE);
        DrawRectangle((int)(cx+40),(int)(cy+56),cw-80,1,COL_BORDER);
        DrawTextC("Fight against AI",(float)(cx+cw/2),(float)(cy+68),15,COL_DIMWHITE);
        DrawTextC("AI auto-plays optimally",(float)(cx+cw/2),(float)(cy+90),13,COL_DIMWHITE);
        DrawTextC("You control Player 1",(float)(cx+cw/2),(float)(cy+110),13,COL_DIMWHITE);

        float bw=200,bh=46,bx=cx+cw/2-bw/2,by=cy+ch-60;
        if(DrawButton("  PLAY VS AI  ",bx,by,bw,bh,{80,40,10,255},{150,80,20,255})){
            gVsAI=true; gSetupStep=0; gState=STATE_SETUP;
        }
    }

    // VS Player 2
    {
        float cx=startX+cw+gap,cy=210;
        bool hov=CheckCollisionPointRec(GetMousePosition(),{cx,cy,cw,ch});
        Color border=hov?COL_P2:COL_BORDER;
        DrawPanel(cx,cy,cw,ch,COL_PANEL,border);
        if(hov) DrawRectangleRounded({cx,cy,cw,ch},0.07f,8,{80,200,255,15});

        DrawTextC("VS PLAYER 2",(float)(cx+cw/2),(float)(cy+20),24,COL_P2);
        DrawRectangle((int)(cx+40),(int)(cy+56),cw-80,1,COL_BORDER);
        DrawTextC("Local 2-Player mode",(float)(cx+cw/2),(float)(cy+68),15,COL_DIMWHITE);
        DrawTextC("Both players on same keyboard",(float)(cx+cw/2),(float)(cy+90),13,COL_DIMWHITE);
        DrawTextC("Each chooses class & name",(float)(cx+cw/2),(float)(cy+110),13,COL_DIMWHITE);

        float bw=200,bh=46,bx=cx+cw/2-bw/2,by=cy+ch-60;
        if(DrawButton("  2-PLAYER  ",bx,by,bw,bh,{10,50,90,255},{20,100,180,255})){
            gVsAI=false; gSetupStep=0; gState=STATE_SETUP;
        }
    }

    DrawTextC("Click a mode to begin",(float)(SW/2),(float)(SH-40),14,COL_BORDER);
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(){
    srand((unsigned)time(0));
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_WINDOW_HIGHDPI);
    InitWindow(SW,SH,"Turn-Based RPG Battle");
    SetTargetFPS(60);

    while(!WindowShouldClose()){
        float dt=GetFrameTime();
        BeginDrawing();
        ClearBackground(COL_BG);

        switch(gState){
            case STATE_MODE_SELECT: DrawModeSelect(); break;
            case STATE_SETUP:       DrawSetup();      break;
            case STATE_BATTLE:      DrawBattle(dt);   break;
        }

        DrawText(TextFormat("FPS:%d",GetFPS()),SW-65,4,12,{80,70,100,160});
        EndDrawing();
    }

    delete gP1; delete gP2; delete gLogger;
    CloseWindow();
    return 0;
}
