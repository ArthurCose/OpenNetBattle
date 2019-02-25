#include "bnBubble.h"
#include "bnBubbleTrap.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"

Bubble::Bubble(Field* _field, Team _team, double speed) : Obstacle(field, team) {
  SetLayer(1);
  field = _field;
  direction = Direction::NONE;
  deleted = false;
  hit = false;
  health = 1;
  texture = TEXTURES.GetTexture(TextureType::SPELL_BUBBLE);
  this->speed = speed;

  this->slideTime = sf::seconds(0.5f / (float)speed);

  animation = Animation("resources/spells/bubble.animation");
  
  auto onFinish = [this]() { animation << "FLOAT" << Animate::Mode::Loop; };

  animation << "INIT" << onFinish;

  EnableTileHighlight(false);
}

Bubble::~Bubble(void) {
}

void Bubble::Update(float _elapsed) {
  setTexture(*texture);
  setScale(2.f, 2.f);
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);

  animation.Update(_elapsed*(float)this->speed, *this);

  // Keep moving
  if (!this->isSliding && animation.GetAnimationString() == "FLOAT") {
    if (this->tile->GetX() == 1) {
      if (this->tile->GetY() == 2 && this->GetDirection() == Direction::LEFT) {
        this->Delete();
      }
      else if (this->tile->GetY() == 1) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::DOWN);
        }
        else {
          this->Delete();
        }
      }
      else if (this->tile->GetY() == 3) {
        if (this->GetDirection() == Direction::LEFT) {
          this->SetDirection(Direction::UP);
        }
        else {
          this->Delete();
        }
      }
    }
    else if (this->tile->GetX() == 6) {
      this->Delete();
    }

    this->SlideToTile(true);
    this->Move(this->GetDirection());
  }

  tile->AffectEntities(this);

  Entity::Update(_elapsed);
}

bool Bubble::CanMoveTo(Battle::Tile* tile) {
  return true;
}


const bool Bubble::Hit(int damage, Hit::Properties props) {
  if (!hit) {
    hit = true;

    auto onFinish = [this]() { this->Delete(); };
    animation << "POP" << onFinish;
    return true;
  }

  return false;
}

void Bubble::Attack(Character* _entity) {
  if (!hit) {
    hit = _entity->Hit(40);
    if (hit) {

      Obstacle* other = dynamic_cast<Obstacle*>(_entity);

      if (!other && _entity->GetComponent<BubbleTrap>() == nullptr) {
        BubbleTrap* trap = new BubbleTrap(_entity);
        _entity->RegisterComponent(trap);
        GetField()->AddEntity(*trap, GetTile()->GetX(), GetTile()->GetY());
      }

      auto onFinish = [this]() { this->Delete(); };
      animation << "POP" << onFinish;
    }
  }
}