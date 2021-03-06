#include "bnPlayerForm.h"
#include "bnPlayer.h"

PlayerFormMeta::PlayerFormMeta(int index) : index(index), path() { ; }

void PlayerFormMeta::SetUIPath(std::string path) {
  this->path = path;
}

const std::string PlayerFormMeta::GetUIPath() const
{
  return path;
}

void PlayerFormMeta::ActivateForm(Player& player) {
  player.ActivateFormAt(index);
}

PlayerForm* PlayerFormMeta::BuildForm() {
  loadFormClass();
  auto res = form;
  form = nullptr;
  return res;
}
