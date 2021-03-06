
/*! \brief Component that protects an entity from all contact damage
 * 
 * The shield is just an animation but it adds a defense rule 
 * to the attached entity.
 * 
 * Defense rules check against all attacks before resolving damage
 * If the rule passes, the attack hits
 * In this case, Reflect Shield protects all tangible damage
 * and will return true, firing a callback.
 * 
 * This callback will spawn a RowHit spell object to deal damage
 */

#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ReflectShield : virtual public Artifact, virtual public Component
{
private:
  DefenseRule* guard; /*!< Adds defense rule to attached entity */
  Animation animation; /*!< Shield animation */
  sf::Sprite shield; /*!< Shield graphic */
  bool activated; /*!< Flag if effect is active */
  int damage; /*!< Damage the reflect trail deals */
public:
  /**
   * @brief Adds a guard rule to the attached entity for a short time *
   * 
   * A guard rule object is constructed. When it fails it runs DoReflect()
   * which spawns a RowHit spell
   * 
   * At the end of the shield animation the rule is dropped and this 
   * component is removed from the owner and then deleted.
   */
  ReflectShield(Character* owner, int damage);
  
  /**
   * @brief Delete guard pointer
   */
  ~ReflectShield();
  
  /**
   * @brief This component does not get injected into the scene
   */
  virtual void Inject(BattleScene&);
  
  /**
   * @brief Updates animation
   * @param _elapsed
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief This artifact does not move across the grid
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }

  /**
   * @brief If the first time reflecting, spawn a RowHit spell
   * @param in the attack we are reflecting
   * @param owner the owner of the attack
   */
  void DoReflect(Spell* in, Character* owner);
};
