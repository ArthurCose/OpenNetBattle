#include "bnCharacter.h"
#include "bnDefenseRule.h"
#include "bnSpell.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnElementalDamage.h"
#include "bnShaderResourceManager.h"
#include "bnAnimationComponent.h"
#include "bnShakingEffect.h"
#include <Swoosh/Ease.h>

Character::Character(Rank _rank) :
  health(0),
  maxHealth(0),
  counterable(false),
  canTilePush(true),
  slideFromDrag(false),
  canShareTile(false),
  stunCooldown(0),
  invincibilityCooldown(0),
  counterSlideDelta(0),
  name("unnamed"),
  rank(_rank),
  invokeDeletion(false),
  hit(false),
  CounterHitPublisher(), Entity() {

  whiteout = SHADERS.GetShader(ShaderType::WHITE);
  stun = SHADERS.GetShader(ShaderType::YELLOW);
}

Character::~Character() {
  // Defense items need to be manually deleted where they are created
  defenses.clear();
}

bool Character::IsStunned()
{
  return this->stunCooldown > 0;
}

const Character::Rank Character::GetRank() const {
  return rank;
}

void Character::ShareTileSpace(bool enabled)
{
  canShareTile = enabled;

}

const bool Character::CanShareTileSpace() const
{
  return this->canShareTile;
}

void Character::EnableTilePush(bool enabled)
{
  this->canTilePush = enabled;
}

const bool Character::CanTilePush() const {
  return this->canTilePush;
}

void Character::Update(float _elapsed) {
  sf::Vector2f shakeOffset;

  double prevThisFrameStun = this->stunCooldown;

  if (this->IsBattleActive()) {
    this->ResolveFrameBattleDamage();
  }

  this->setColor(sf::Color(255, 255, 255, getColor().a));

  if (!hit) {
      if (stunCooldown && (((int)(stunCooldown * 15))) % 2 == 0) {
          this->SetShader(stun);
      }
      else if(this->GetHealth() > 0) {
          this->SetShader(nullptr);

          if (this->counterable) {
            this->setColor(sf::Color(255, 55, 55, getColor().a));
            this->SetShader(SHADERS.GetShader(ShaderType::ADDITIVE));
          }
      }

      if (this->invincibilityCooldown > 0) {
          // This just blinks every 15 ms
          if ((((int)(invincibilityCooldown * 15))) % 2 == 0) {
            this->Hide();
          }
          else {
            this->Reveal();
          }

          invincibilityCooldown -= _elapsed;

          if (this->invincibilityCooldown <= 0) {
            this->Reveal();
          }
      }
  }
  else {
      SetShader(whiteout);
  }

  if (prevThisFrameStun <= 0.0) {
    // HACKY: If we are stunned this frame, let AI update step once
    // to turn into their respective hit state animations

    this->OnUpdate(_elapsed);
  } else if (this->stunCooldown > 0.0) {
    this->stunCooldown -= _elapsed;

    // TODO: is this needed here anymore?
    // setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

    if (this->stunCooldown <= 0.0) {
      this->stunCooldown = 0.0;
    }
  }

  Entity::Update(_elapsed);

  // If the counterSlideOffset has changed from 0, it's due to the character
  // being deleted on a counter frame. Begin animating the counter-delete slide
  if (counterSlideOffset.x != 0 || counterSlideOffset.y != 0) {
    counterSlideDelta += _elapsed;
    
    auto delta = swoosh::ease::linear(counterSlideDelta, 0.10f, 1.0f);
    auto offset = delta * counterSlideOffset;

    // Add this offset onto our offsets
    setPosition(tile->getPosition().x + offset.x, tile->getPosition().y + offset.y);
  }

  setPosition(getPosition() + shakeOffset);

  hit = false;

  // TODO: Something IS skipping the SetHealth() routine. Find out what and take this check out.
  if (health < 0) health = 0;

  TryDelete();

  // If drag status is over, reset the flag
  if (!IsSliding() && this->slideFromDrag) this->slideFromDrag = false;
}

bool Character::CanMoveTo(Battle::Tile * next)
{
  auto occupied = [this](Entity* in) {
    Character* c = dynamic_cast<Character*>(in);

    return c && c != this && !c->CanShareTileSpace();
  };

  bool result = (Entity::CanMoveTo(next) && next->FindEntities(occupied).size() == 0);
  result = result && !next->IsEdgeTile();

  return result;
}

const bool Character::Hit(Hit::Properties props) {

  if (this->GetHealth() <= 0) return false;

  // Pierce hits even when passthrough or flinched
  if ((props.flags & Hit::pierce) != Hit::pierce) {
    if (this->invincibilityCooldown > 0 || this->IsPassthrough()) return false;
  }

  if ((props.flags & Hit::shake) == Hit::shake) {
    this->RegisterComponent(new ShakingEffect(this));
  }
  
  for (auto& defense : defenses) {
    props = defense->FilterStatuses(props);
  }

  for (auto c : shareHit) {
    c->Hit(props);
  }

  // If the character itself is also super-effective,
  // double the damage independently from tile damage
  bool isSuperEffective = IsSuperEffective(props.element);

  // Show ! super effective symbol on the field
  if (isSuperEffective) {
      props.damage *= 2;

    Artifact *seSymbol = new ElementalDamage(field);
    field->AddEntity(*seSymbol, tile->GetX(), tile->GetY());
  }

  this->SetHealth(GetHealth() - props.damage);

  // Add to status queue for state resolution
  this->statusQueue.push(props);

  Logger::Log("pushing states");

  return true;
}

int Character::GetHealth() const {
  return health;
}

const int Character::GetMaxHealth() const
{
  return this->maxHealth;
}

void Character::ResolveFrameBattleDamage()
{
  if(this->statusQueue.empty()) return;

  Character* frameCounterAggressor = nullptr;
  bool frameStunCancel = false;
  Direction postDragDir = Direction::NONE;

  std::queue<Hit::Properties> append;

  while(!this->statusQueue.empty() && !IsSliding()) {
    Hit::Properties& props = this->statusQueue.front();
    this->statusQueue.pop();

    int tileDamage = 0;

    // Calculate elemental damage if the tile the character is on is super effective to it
    if (props.element == Element::FIRE
      && GetTile()->GetState() == TileState::GRASS
      && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
      GetTile()->SetState(TileState::NORMAL);
    }
    else if (props.element == Element::ELEC
      && GetTile()->GetState() == TileState::ICE
      && !(this->HasAirShoe() || this->HasFloatShoe())) {
      tileDamage = props.damage;
      GetTile()->SetState(TileState::NORMAL);
    }

    // Pass on hit properties to the user-defined handler
    if (this->OnHit(props)) {
      // Only register counter if:
      // 1. Hit type is impact
      // 2. The character is on a counter frame
      // 3. Hit properties has an aggressor
      // This will set the counter aggressor to be the first non-impact hit and not check again this frame
      if (this->IsCountered() && (props.flags & Hit::impact) == Hit::impact && !frameCounterAggressor) {
        if (props.aggressor) {
          frameCounterAggressor = props.aggressor;
        }
      }

      // Requeue drag if already sliding by drag
      if ((props.flags & Hit::drag) == Hit::drag) {
        if (this->slideFromDrag) {
          append.push({ 0, Hit::drag, Element::NONE, nullptr, props.drag });
        }
        else {
          // Apply directional slide in a moment
          postDragDir = props.drag;

          // requeue counter hits
          append.push({ 0, Hit::impact, Element::NONE, frameCounterAggressor, Direction::NONE });
          frameCounterAggressor = nullptr;
        }

        // exclude this from the next processing step
        props.drag = Direction::NONE;
        props.flags &= ~Hit::drag;
      }

      bool hadStun = false;

      // Stun can be canceled by non-stun hits or queued if dragging
      if ((props.flags & Hit::stun) == Hit::stun) {
        if (postDragDir != Direction::NONE) {
          // requeue these statuses if in the middle of a slide
          append.push({ 0, props.flags, Element::NONE, nullptr, Direction::NONE });
        }
        else {
          this->stunCooldown = 3.0;
          hadStun = true;
        }
      }

      // exclude this from the next processing step
      props.flags &= ~Hit::stun;

      // Flinch is ignored if already flinching or stunned
      // and can be queued if dragging this frame
      if ((props.flags & Hit::flinch) == Hit::flinch && !hadStun) {
        if (postDragDir != Direction::NONE) {
          append.push({ 0, props.flags, Element::NONE, nullptr, Direction::NONE });
        }
        else {
          if (this->invincibilityCooldown <= 0.0) {
            this->invincibilityCooldown = 3.0;
          }

          // cancel stun
          this->stunCooldown = 0;
        }
      }

      hit = hit || true;
    }

    if (hit) {
      this->SetHealth(GetHealth() - tileDamage);

      if (this->GetHealth() == 0) {
        this->stunCooldown = 0;
        postDragDir = Direction::NONE; // Cancel slide post-status if blowing up
      }
    }
  }

  if (!append.empty()) {
    this->statusQueue = append;
  }

  if (postDragDir != Direction::NONE) {
    // enemies and objects on opposing side of field are granted immunity from drag
    if (Teammate(this->GetTile()->GetTeam())) {
      this->SlideToTile(true);
      this->slideFromDrag = true;
      this->Move(postDragDir);

      // cancel stun
      this->stunCooldown = 0;
    }
  }

  if (frameCounterAggressor) {
    this->Broadcast(*this, *frameCounterAggressor);
    this->ToggleCounter(false);
    this->Stun(3.0);
  }

  if (this->GetHealth() == 0 && !this->invokeDeletion) {

    while(this->statusQueue.size() > 0) {
      this->statusQueue.pop();
    }

    this->OnDelete();
    this->invokeDeletion = true;
    this->stunCooldown = 0;
    this->invincibilityCooldown = 0;

    this->SlideToTile(false); // cancel slide

    if(frameCounterAggressor) {
      // Slide entity back a few pixels
      this->counterSlideOffset = sf::Vector2f(50.f, 0.0f);
    }
  }
}

void Character::SetHealth(const int _health) {
  health = _health;

  if (maxHealth == 0) {
    maxHealth = health;
  }

  if (health < 0) health = 0;
  if (health > maxHealth) health = maxHealth;
}

void Character::AdoptTile(Battle::Tile * tile)
{
  tile->AddEntity(*this);

  if (!IsSliding()) {
    this->setPosition(tile->getPosition());
  }
}

void Character::TryDelete() {
  if (!IsBattleActive()) return;

  if (this->GetHealth() == 0 && !this->invokeDeletion) {
      this->OnDelete();
      this->invokeDeletion = true;
      this->SlideToTile(false);
  }
}

void Character::ToggleCounter(bool on)
{
  counterable = on;
}

void Character::Stun(double maxCooldown)
{
  stunCooldown = maxCooldown;
}

bool Character::IsCountered()
{
  return (counterable && stunCooldown <= 0);
}

void Character::SetName(std::string name)
{
  this->name = name;
}

const std::string Character::GetName() const
{
  return name;
}

void Character::AddDefenseRule(DefenseRule * rule)
{
  if (!rule) return;

  auto iter = std::find_if(defenses.begin(), defenses.end(), [rule](DefenseRule* other) { return rule->GetPriorityLevel() == other->GetPriorityLevel(); });

  if (rule && iter == defenses.end()) {
    defenses.push_back(rule);
    std::sort(defenses.begin(), defenses.end(), [](DefenseRule* first, DefenseRule* second) { return first->GetPriorityLevel() < second->GetPriorityLevel(); });
  }
  else {
    (*iter)->replaced = true; // Flag that this defense rule may be valid ptr, but is no longer in use
    this->RemoveDefenseRule(*iter);

    // call again, adding this time
    this->AddDefenseRule(rule);
  }
}

void Character::RemoveDefenseRule(DefenseRule * rule)
{
  auto iter = std::remove_if(defenses.begin(), defenses.end(), [&rule](DefenseRule * in) { return in == rule; });

  if(iter != defenses.end())
    defenses.erase(iter);
}

const bool Character::CheckDefenses(Spell* in)
{
  for (int i = 0; i < defenses.size(); i++) {
    if (defenses[i]->Check(in, this)) {
      return true;
    }
  }

  return false;
}

void Character::SharedHitboxDamage(Character * to)
{
  auto iter = std::find(shareHit.begin(), shareHit.end(), to);

  if (to && iter == shareHit.end()) {
    shareHit.push_back(to);
  }
}

void Character::CancelSharedHitboxDamage(Character * to)
{
  auto iter = std::remove_if(shareHit.begin(), shareHit.end(), [&to](Character * in) { return in == to; });

  if(iter != shareHit.end())
    shareHit.erase(iter);
}
