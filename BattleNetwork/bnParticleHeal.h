
/*! \brief Heal effect that plays when some chips support a character */

#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ParticleHeal : public Artifact {
private:
  Animation animation;
  sf::Sprite fx;
public:
  /**
   * \brief sets the animation
   */
  ParticleHeal();

  /**
   * @brief deconstructor
   */
  ~ParticleHeal();

  /**
   * @brief plays the animation and deletes when finished
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);

  /**
   * @brief heal effect doesn't move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) { return false; }
};