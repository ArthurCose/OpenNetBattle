#include "bnPlayer.h"
#include "bnNaviExplodeState.h"
#include "bnField.h"
#include "bnBuster.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnLogger.h"
#include "bnAura.h"

#include "bnBubbleTrap.h"
#include "bnBubbleState.h"

#define RESOURCE_PATH "resources/navis/megaman/megaman.animation"

Player::Player(void)
  :
  state(PLAYER_IDLE),
  chargeComponent(this),
  AI<Player>(this),
  Character(Rank::_1)
{
  this->ChangeState<PlayerIdleState>();
  
  // The charge component is also a scene node
  // Make sure the charge is in front of this node
  // Otherwise children scene nodes are drawn behind 
  // their parents
  chargeComponent.SetLayer(-1);
  this->AddNode(&chargeComponent);
  chargeComponent.setPosition(0, -20.0f); // translate up -20

  SetHealth(1000);
  
  name = "Megaman";
  SetLayer(0);
  team = Team::RED;

  moveCount = hitCount = 0;

  setScale(2.0f, 2.0f);

  AnimationComponent* animationComponent = new AnimationComponent(this);
  animationComponent->Setup(RESOURCE_PATH);
  animationComponent->Reload();
  this->RegisterComponent(animationComponent);

  setTexture(*TEXTURES.GetTexture(TextureType::NAVI_MEGAMAN_ATLAS));

  previous = nullptr;

  moveCount = 0;

  invincibilityCooldown = 0;
}

Player::~Player() {
}

void Player::Update(float _elapsed) {
  if (tile != nullptr) {
    setPosition(tileOffset.x + tile->getPosition().x, tileOffset.y + tile->getPosition().y);
  }

  if (invincibilityCooldown > 0) {
    // This just blinks every 15 frames
    if ((((int)(invincibilityCooldown * 15))) % 2 == 0) {
      this->Hide();
    }
    else {
      this->Reveal();
    }

    invincibilityCooldown -= _elapsed;
  }
  else {
    this->Reveal();
  }

  AI<Player>::Update(_elapsed);

  //Components updates
  chargeComponent.Update(_elapsed);

  Character::Update(_elapsed);
}

void Player::Attack() {
  if (tile->GetX() <= static_cast<int>(field->GetWidth())) {
    Spell* spell = new Buster(field, team, chargeComponent.IsFullyCharged());
    spell->SetDirection(Direction::RIGHT);
    field->AddEntity(*spell, tile->GetX(), tile->GetY());
  }
}

void Player::OnDelete() {
  chargeComponent.Hide();
  auto animationComponent = this->GetFirstComponent<AnimationComponent>();
  animationComponent->CancelCallbacks();
  animationComponent->SetAnimation(PLAYER_HIT);
  this->ChangeState<NaviExplodeState<Player>>(5, 0.65);
}

const bool Player::OnHit(const Hit::Properties props) {
  // Don't take damage while blinking
  if (invincibilityCooldown > 0) return false;

  hitCount++;

  // Respond to the recoil bit state
  if ((props.flags & Hit::recoil) == Hit::recoil) {
    // this->ChangeState<PlayerHitState>((float)props.secs );
    this->ChangeState<PlayerHitState>();
  }

  return true;
}

int Player::GetMoveCount() const
{
  return moveCount;
}

int Player::GetHitCount() const
{
  return hitCount;
}

void Player::SetCharging(bool state)
{
  chargeComponent.SetCharging(state);
}

void Player::SetAnimation(string _state, std::function<void()> onFinish) {
  state = _state;

  if (state == PLAYER_IDLE) {
    auto playback = Animate::Mode::Loop;
    animationComponent.SetAnimation(_state, playback);
  }
  else {
    animationComponent.SetAnimation(_state, 0, onFinish);
  }
}
