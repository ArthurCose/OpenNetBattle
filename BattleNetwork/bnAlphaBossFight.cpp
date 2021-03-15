#include "bnAlphaBossFight.h"
#include "bnAlphaCore.h"
#include "bnMettaur.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnCard.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnFadeInState.h"
#include "bnVirusBackground.h"

AlphaBossFight::AlphaBossFight(Field* field) : MobFactory(field)
{
}


AlphaBossFight::~AlphaBossFight()
{
}

Mob* AlphaBossFight::Build() {
  Mob* mob = new Mob(field);
  auto bg = std::make_shared<VirusBackground>();
  bg->SetScrollSpeed(2.f);
  bg->ScrollLeft();

  // Changes music and ranking algorithm
  mob->ToggleBossFlag();
  mob->StreamCustomMusic("resources/loops/proto.ogg");
  mob->SetBackground(bg);
  mob->Spawn<Rank1<AlphaCore, FadeInState>>(5, 2);

  Battle::Tile* tile = field->GetAt(5, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

  field->GetAt(2, 2)->SetState(TileState::holy);

  return mob;
}
