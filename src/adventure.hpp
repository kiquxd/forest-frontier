#pragma once
#include "gameplay.hpp"
namespace frontier {
void initializeAdventure(Save& save);
void syncCitizens(Save& save);
const CharacterDef& playerCharacter(const Save& save);
const OutfitDef& playerOutfit(const Save& save);
float gatherTimeMultiplier(const Save& save);
int availableWeapon(const Save& save,const std::string& id,int excludeNpc=-1);
int availableOutfit(const Save& save,const std::string& id,int excludeNpc=-1);
Action equipOutfit(Save& save,const std::string& id);
std::vector<std::string> ownedOutfits(const Save& save);
const QuestDef* npcQuest(const Save& save,const Npc& npc);
std::string npcIntroduction(const Save& save,const Npc& npc);
int questProgress(const Save& save,const QuestProgress& progress);
std::string questStatus(const Save& save,const Npc& npc);
Action acceptQuest(Save& save,const Npc& npc);
Action finishQuest(Save& save,const Npc& npc);
bool recipeKnown(const Save& save,const Recipe& recipe);
int averageTrust(const Save& save);
struct ProgressNode {std::string id,name,description;int coins{},residents{},skills{},clothes{},weapons{},cities{},trust{},quests{};std::string prerequisite;std::string secondPrerequisite{};};
const std::vector<ProgressNode>& researchNodes();
std::string researchRequirements(const Save& save,const ProgressNode& node);
bool researchReady(const Save& save,const ProgressNode& node);
Action research(Save& save,int index);
const std::vector<std::string>& residentJobs();
Citizen* citizen(Save& save,int npc);
const Citizen* citizen(const Save& save,int npc);
Action assignCitizen(Save& save,int npc,const std::string& job,const std::string& weapon,const std::string& outfit);
void tickResidents(Save& save,float dt);
}
