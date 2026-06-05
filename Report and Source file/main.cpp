#include "iostream"
#include "string"
#include "cstdlib"
#include "fstream"
#include "stdexcept"
#include "ctime"

using namespace std;

class BattleLogger
{
private:
    ofstream logFile;
    string filename;

public:
    BattleLogger(string fname) : filename(fname)
    {
        logFile.open(fname);
        if (!logFile.is_open())
        {
            throw runtime_error("Failed to open the log file: " + fname);
        }
        logFile << "=== BATTLE LOG ===" << endl;
    };

    void log(string message)
    {
        if (logFile.is_open())
        {
            logFile << message << "\n";
        }
    }

    ~BattleLogger()
    {
        if (logFile.is_open())
        {
            logFile << "=== End of Log ===" << "\n";
            logFile.close();
        }
    }
};

class StatusEffect
{
private:
    string effectName;
    int damagePerTurn;
    int duration;
    bool isStun;

public:
    StatusEffect(string name, int dmg, int dur, bool stun) : effectName(name), damagePerTurn(dmg), duration(dur), isStun(stun) {};

    string getName()
    {
        return effectName;
    }

    int getDamage()
    {
        return damagePerTurn;
    }

    int getDuration()
    {
        return duration;
    }

    bool getStun()
    {
        return isStun;
    }

    void decrementDuration()
    {
        duration--;
    }

    bool isExpired()
    {
        return duration <= 0;
    }
};

class Skill
{
private:
    string skillName;
    int energyCost;
    int damage;
    StatusEffect *effect;

public:
    Skill(string name, int cost, int dmg) : skillName(name), energyCost(cost), damage(dmg), effect(nullptr) {};
    Skill(string name, int cost, int dmg, StatusEffect *eff) : skillName(name), energyCost(cost), damage(dmg), effect(eff) {};

    string getName()
    {
        return skillName;
    }

    int getenergyCost()
    {
        return energyCost;
    }

    int getDamage()
    {
        return damage;
    }

    StatusEffect *getEffect()
    {
        return effect;
    }
};

class Character
{
private:
    string name;
    float health;
    float energy;
    int attackPower;
    int defense;
    int speed;

protected:
    StatusEffect *activeEffect;

public:
    Character(string n, float h, float m, int atk, int def, int spd) : name(n), health(h), energy(m), attackPower(atk), defense(def), speed(spd), activeEffect(nullptr) {}

    string getName()
    {
        return name;
    }

    float getHealth()
    {
        return health;
    }

    float getenergy()
    {
        return energy;
    }

    int getAttackPower()
    {
        return attackPower;
    }

    int getDefense()
    {
        return defense;
    }

    int getSpeed()
    {
        return speed;
    }

    void setHealth(float h)
    {
        health = h;
    }

    void setenergy(float m)
    {
        energy = m;
    }

    void applyStatusEffect(StatusEffect *eff)
    {
        delete activeEffect;
        activeEffect = eff;
    }

    void processStatusEffect()
    {
        if (activeEffect == nullptr)
        {
            return;
        }

        if (activeEffect->isExpired())
        {
            cout << activeEffect->getName() << " has worn off on " << name << "!" << endl;
            delete activeEffect;
            activeEffect = nullptr;
            return;
        }
        if (!activeEffect->getStun())
        {
            float dmg = activeEffect->getDamage();
            health -= dmg;
            cout << name << " takes " << dmg << " damage from " << activeEffect->getName() << "!\n";
        }
        activeEffect->decrementDuration();
    }

    bool isStunned()
    {
        return (activeEffect != nullptr && activeEffect->getStun());
    }

    virtual bool isAlive()
    {
        return health > 0;
    }

    virtual void displayStatus()
    {
        cout << "-----------------------------\n";
        cout << "Name:         " << name << "\n";
        cout << "Health:       " << health << "\n";
        cout << "energy:         " << energy << "\n";
        cout << "Attack Power: " << attackPower << "\n";
        cout << "Defense:      " << defense << "\n";
        cout << "Speed:        " << speed << "\n";
        cout << "-----------------------------\n";
    }
    virtual void attack(Character *target) = 0;
    virtual void useSkill(Character *target) = 0;

    virtual ~Character()
    {
        delete activeEffect;
    }
};

class Warrior : public Character
{
private:
    int shieldBonus;
    int skillenergyCost;

public:
    Warrior(string n, float h, float m, int atk, int def, int spd, int shield, int cost) : Character(n, h, m, atk, def, spd), shieldBonus(shield), skillenergyCost(cost) {}

    void attack(Character *target) override
    {
        int damage = getAttackPower() - target->getDefense();
        if (damage < 0)
        {
            damage = 0;
        }
        target->setHealth(target->getHealth() - damage);
        cout << getName() << " attacks " << target->getName() << " and deals " << damage << " damage!\n";
    }

    void useSkill(Character *target) override
    {
        if (getenergy() < skillenergyCost)
        {
            cout << getName() << " doesn't have enough energy" << endl;
            return;
        }

        setenergy(getenergy() - skillenergyCost);
        int damage = 15 + shieldBonus;

        StatusEffect *stun = new StatusEffect("Stun", 0, 1, true);
        target->applyStatusEffect(stun);
        target->setHealth(target->getHealth() - damage);

        cout << getName() << " uses Shield attack on " << target->getName() << " and deals " << damage << " damage !" << endl;
        cout << " === Target is now Stunned ===" << endl;
    }
};

class Mage : public Character
{
private:
    int spellPower;

public:
    Mage(string n, float h, float m, int atk, int def, int spd, int sp) : Character(n, h, m, atk, def, sp), spellPower(sp) {};

    void attack(Character *target) override
    {
        int damage = getAttackPower() - target->getDefense();

        if (damage < 0)
        {
            damage = 0;
        }

        target->setHealth(target->getHealth() - damage);
        cout << getName() << " fires a magic blast at " << target->getName() << " for " << damage << " damage! " << endl;
    }

    void useSkill(Character *target) override
    {
        if (getenergy() < 30)
        {
            cout << getName() << " doesn't have enough energy " << endl;
            return;
        }
        setenergy(getenergy() - 30);
        int damage = 30 + spellPower;

        StatusEffect *poison = new StatusEffect("Poison", 5, 3, false);
        target->applyStatusEffect(poison);
        target->setHealth(target->getHealth() - damage);

        cout << getName() << " casts Fireball on " << target->getName() << " and dealt a " << damage << " xp damage !" << endl
             << " === Target is now Poisoned ===" << endl;
    }
};

class Tank : public Character
{
private:
    int damageReduction;

public:
    Tank(string n, float h, float m, int atk, int def, int spd, int dr) : Character(n, h, m, atk, def, spd), damageReduction(dr) {}

    void attack(Character *target) override
    {
        int damage = getAttackPower() - target->getDefense();

        if (damage < 0)
        {
            damage = 0;
        }

        target->setHealth(target->getHealth() - damage);
        cout << getName() << " slams " << target->getName()
             << " for " << damage << " damage!\n";
    }

    void useSkill(Character *target) override
    {
        if (getenergy() < 15)
        {
            cout << getName() << " doesn't have enough energy!\n";
            return;
        }
        setenergy(getenergy() - 15);
        setHealth(getHealth() + 20 + damageReduction);

        cout << getName() << " uses Iron Wall — restored "
             << (20 + damageReduction) << " HP!\n";
    }
};

class BattleEngine
{
private:
    Character *player;
    Character *enemy;
    BattleLogger logger;
    int roundNumber = 1;

    void displayBattleStatus()
    {
        int currentRound = roundNumber++;
        cout << "\n===== ROUND " << currentRound << " =====\n";
        cout << "PLAYER: " << player->getName()
             << "  HealthPower: " << player->getHealth()
             << "  EnergyPower: " << player->getenergy() << "\n";
        cout << "ENEMY:  " << enemy->getName()
             << "  HealthPower: " << enemy->getHealth()
             << "  EnergyPower: " << enemy->getenergy() << "\n";
        cout << "=============================\n";

        string status =
            "Player: " + player->getName() +
            "  HealthPower: " + to_string((int)player->getHealth()) +
            "  EnergyPower: " + to_string((int)player->getenergy()) +
            " | Enemy: " + enemy->getName() +
            "  HealthPower: " + to_string((int)enemy->getHealth()) +
            "  EnergyPower: " + to_string((int)enemy->getenergy());
        logger.log(status);
        logger.log("--- Round " + to_string(currentRound) + " ---");
    }

    void playerTurn()
    {
        if (player->isStunned())
        {
            string message = player->getName() + " is stunned and skips their turns ";
            cout << message << "\n";
            logger.log(message);
            player->processStatusEffect();
            return;
        }

        cout << "Your turn! Choose your move " << endl;
        cout << "1. Basic Attack\n";
        cout << "2. Use Skill (Warrior: Shield Bash | Mage: Fireball | Tank: Iron Wall)\n";

        int choice;
        try
        {
            cin >> choice;
            if (cin.fail())
            {
                throw invalid_argument("Input must be a number ");
            }
            if (choice < 1 || choice > 2)
            {
                throw out_of_range("Choice must be 1 or 2");
            }
        }

        catch (invalid_argument &e)
        {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "Invalid input: " << e.what() << "Assigning basic attacks ... \n";
            logger.log("Invalid input caught - defaulted to basic attacks");
            choice = 1;
        }

        catch (out_of_range &e)
        {
            cout << "Out of range: " << e.what() << "Assigning to basic attacks ... \n";
            logger.log("Out of range input caught - defaulted to basic attacks");
            choice = 1;
        }

        if (choice == 1)
        {
            player->attack(enemy);
        }
        else if (choice == 2)
        {
            player->useSkill(enemy);
        }
        else
        {
            cout << "Invalid choice " << endl;
            player->attack(enemy);
        }
        player->processStatusEffect();
    }

    void enemyTurn()
    {
        if (enemy->isStunned())
        {
            string message = enemy->getName() + " is stunned. Their turn is now skipped ...";
            cout << message << endl;
            logger.log(message);
            enemy->processStatusEffect();
            return;
        }

        if (enemy->getenergy() >= 20)
        {
            enemy->useSkill(player);
            logger.log(enemy->getName() + " used skill ");
        }
        else
        {
            enemy->attack(player);
            logger.log(enemy->getName() + " used basic attacks ");
        }
        enemy->processStatusEffect();
    }

public:
    BattleEngine(Character *p, Character *e) : player(p), enemy(e), logger("battle_log.txt") {};

    void startBattle()
    {
        logger.log("Battle started: " + player->getName() + " vs " + enemy->getName());

        cout << "=== Battle is starting ===" << endl;
        player->displayStatus();
        enemy->displayStatus();

        bool playerFirst = player->getSpeed() >= enemy->getSpeed();
        cout << endl
             << (playerFirst ? player->getName() : enemy->getName()) << " goes first ... " << endl;

        while (player->isAlive() && enemy->isAlive())
        {
            displayBattleStatus();

            if (playerFirst)
            {
                if (enemy->isAlive())
                {
                    playerTurn();
                }
                if (player->isAlive())
                {
                    enemyTurn();
                }
            }
            else
            {
                if (player->isAlive())
                {
                    enemyTurn();
                }
                if (enemy->isAlive())
                {
                    playerTurn();
                }
            }
        }

        string result = player->isAlive() ? player->getName() + " wins !" : enemy->getName() + " wins !";

        cout << "=== BATTLE OVER ===" << endl;

        cout << "\n *** Battle Over ***\n"
             << result << endl;
        logger.log("Battle over -" + result);
    }
};

int main()
{
    try
    {
        cout << "=============================\n";
        cout << "   TURN-BASED RPG BATTLE\n";
        cout << "=============================\n";
        cout << "Choose your character:\n";
        cout << "1. Warrior  (High attack & defense)\n";
        cout << "2. Mage     (High magic damage)\n";
        cout << "3. Tank     (High health & healing)\n";

        int choice;
        srand(time(0));
        cin >> choice;

        if (cin.fail())
        {
            throw invalid_argument("Character selection must be a number ");
        }

        Character *player = nullptr;
        Character *enemy = nullptr;

        if (choice == 1)
        {
            string playerName;
            cout << "Enter your character name: ";
            cin >> playerName;
            player = new Warrior(playerName, 150, 60, 30, 15, 12, 10, 20);
        }
        else if (choice == 2)
        {
            string playerName;
            cout << "Enter your character name: ";
            cin >> playerName;
            player = new Mage(playerName, 100, 120, 25, 8, 15, 20);
        }
        else if (choice == 3)
        {
            string playerName;
            cout << "Enter your character name: ";
            cin >> playerName;
            player = new Tank(playerName, 200, 50, 20, 20, 8, 15);
        }
        else
        {
            cout << "Invalid choice - Your default character is Warrior ..." << endl;
            player = new Warrior("Warrior", 150, 60, 30, 15, 12, 10, 20);
        }

        int enemyType = rand() % 3;
        if (enemyType == 0)
        {
            enemy = new Warrior("Enemy Warrior", 130, 50, 28, 13, 11, 8, 20);
        }
        else if (enemyType == 1)
        {
            enemy = new Mage("Enemy Mage", 90, 100, 23, 6, 14, 18);
        }
        else
        {
            enemy = new Tank("Enemy Tank", 180, 40, 18, 18, 7, 12);
        }

        BattleEngine battle(player, enemy);
        battle.startBattle();

        delete player;
        delete enemy;
    }

    catch (invalid_argument &e)
    {
        cout << "Input error: " << e.what() << endl;
    }
    catch (out_of_range &e)
    {
        cout << "Selection error: " << e.what() << endl;
    }
    catch (runtime_error &e)
    {
        cout << "Runtime error: " << e.what() << endl;
    }
    catch (...)
    {
        cout << "An unexpected error occured " << endl;
    }
}