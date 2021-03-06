#include "bnTwinFangChipAction.h"
#include "bnChipAction.h"
#include "bnSpriteSceneNode.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnTwinFang.h"

#define FRAME1 { 1, 0.05 }
#define FRAME2 { 2, 0.05 }
#define FRAME3 { 3, 0.05 }

#define FRAMES FRAME1, FRAME1, FRAME1, FRAME1, FRAME2, FRAME3, FRAME2, FRAME1, FRAME1


TwinFangChipAction::TwinFangChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_SHOOTING", nullptr, "Buster") {
  this->damage = damage;

  // add override anims
  this->OverrideAnimationFrames({ FRAMES });
}

void TwinFangChipAction::Execute() {
  auto owner = GetOwner();

  // On shoot frame, drop projectile
  auto onFire = [this, owner]() -> void {
    auto tile = GetOwner()->GetTile();

    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    /**
    
    These duds were written before the grid was surrounded by invisible tiles
    The original games have invisible tiles for items that appear to float or go off the stage
    This can now be re-written to not need twin fang dud objects...
    */
    if (tile->GetY() != 1) {
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::ABOVE, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::RIGHT);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() - 1);
    }
    else { // TwinFang floats above the scene
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::ABOVE_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::RIGHT);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
    }

    if (tile->GetY() != 3) {
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::BELOW, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::RIGHT);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY() + 1);
    }
    else { // TwinFang floats below the scene
      TwinFang* twinfang = new TwinFang(GetOwner()->GetField(), GetOwner()->GetTeam(), TwinFang::Type::BELOW_DUD, damage);
      auto props = twinfang->GetHitboxProperties();
      props.aggressor = GetOwnerAs<Character>();
      twinfang->SetHitboxProperties(props);
      twinfang->SetDirection(Direction::RIGHT);

      GetOwner()->GetField()->AddEntity(*twinfang, tile->GetX(), tile->GetY());
    }
  };

  this->AddAction(2, onFire);
}

TwinFangChipAction::~TwinFangChipAction()
{
}

void TwinFangChipAction::OnUpdate(float _elapsed)
{
  ChipAction::OnUpdate(_elapsed);
}

void TwinFangChipAction::EndAction()
{
  GetOwner()->FreeComponentByID(this->GetID());
  delete this;
}
