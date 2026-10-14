#ifndef FOREST_FRONTIER_SRC_ADVENTURE_HPP_
#define FOREST_FRONTIER_SRC_ADVENTURE_HPP_

#include <string>
#include <vector>

#include "gameplay.hpp"

namespace frontier {
void InitializeAdventure(Save& save);
void SyncCitizens(Save& save);
const CharacterDef& PlayerCharacter(const Save& save);
const OutfitDef& PlayerOutfit(const Save& save);
float GatherTimeMultiplier(const Save& save);
int AvailableWeapon(const Save& save, const std::string& id,
                    int exclude_npc = -1);
int AvailableOutfit(const Save& save, const std::string& id,
                    int exclude_npc = -1);
Action EquipOutfit(Save& save, const std::string& id);
std::vector<std::string> OwnedOutfits(const Save& save);
const QuestDef* NpcQuest(const Save& save, const Npc& npc);
std::string NpcIntroduction(const Save& save, const Npc& npc);
int GetQuestProgress(const Save& save, const QuestProgress& progress);
std::string QuestStatus(const Save& save, const Npc& npc);
Action AcceptQuest(Save& save, const Npc& npc);
Action FinishQuest(Save& save, const Npc& npc);
bool RecipeKnown(const Save& save, const Recipe& recipe);
int AverageTrust(const Save& save);
struct ProgressNode {
  std::string id;
  std::string name;
  std::string description;
  int coins{};
  int residents{};
  int skills{};
  int clothes{};
  int weapons{};
  int cities{};
  int trust{};
  int quests{};
  std::string prerequisite;
  std::string second_prerequisite{};
};
const std::vector<ProgressNode>& ResearchNodes();
std::string ResearchRequirements(const Save& save, const ProgressNode& node);
bool ResearchReady(const Save& save, const ProgressNode& node);
Action Research(Save& save, int index);
const std::vector<std::string>& ResidentJobs();
Citizen* GetCitizen(Save& save, int npc);
const Citizen* GetCitizen(const Save& save, int npc);
Action AssignCitizen(Save& save, int npc, const std::string& job,
                     const std::string& weapon, const std::string& outfit);
void TickResidents(Save& save, float dt);
}  // namespace frontier

#endif  // FOREST_FRONTIER_SRC_ADVENTURE_HPP_
