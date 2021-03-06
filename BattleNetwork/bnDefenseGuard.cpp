#include "bnDefenseGuard.h"
#include "bnEntity.h"
#include "bnField.h"
#include "bnSpell.h"
#include "bnHitbox.h"
#include "bnGuardHit.h"

DefenseGuard::DefenseGuard(DefenseGuard::Callback callback) : callback(callback), DefenseRule(Priority(1))
{
}

DefenseGuard::~DefenseGuard()
{
}

const bool DefenseGuard::Check(Spell * in, Character* owner)
{
  auto props = in->GetHitboxProperties();

  if ((props.flags & Hit::breaking) == 0) {
    this->callback(in, owner);

    owner->GetField()->AddEntity(*new GuardHit(owner->GetField(), owner, true), owner->GetTile()->GetX(), owner->GetTile()->GetY());
    owner->GetField()->AddEntity(*new Hitbox(owner->GetField(), owner->GetTeam(), 0), owner->GetTile()->GetX(), owner->GetTile()->GetY());

    return true; // Guard disallows an attack to passthrough
  }

  return false;
}
