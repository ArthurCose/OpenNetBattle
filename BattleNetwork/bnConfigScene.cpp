#include "bnConfigScene.h"
#include <Swoosh/ActivityController.h>
#include "Segues/WhiteWashFade.h"
#include "bnRobotBackground.h"

const constexpr int OPTIONS   = 0;
const constexpr int ACTIONS   = 1;
const constexpr int BOUNDKEYS = 2;

ConfigScene::ConfigScene(swoosh::ActivityController &controller) : swoosh::Activity(&controller)
{
  isSelectingTopMenu = inGamepadList = inKeyboardList = false;

  // Draws the scrolling background
  bg = new RobotBackground();

  // dim
  bg->setColor(sf::Color(120, 120, 120));

  uiAnimator = Animation("resources/fonts/fonts.animation");
  uiAnimator.Load();

  endBtnAnimator = Animation("resources/backgrounds/config/end_btn.animation");
  endBtnAnimator.Load();

  audioAnimator = Animation("resources/backgrounds/config/audio.animation");
  audioAnimator.Load();

  auto sprite = sf::Sprite(LOAD_TEXTURE(FONT));
  sprite.setScale(2.f, 2.f);

  uiSprite = sprite;

  // audio button
  audioBGM =  sf::Sprite(LOAD_TEXTURE(AUDIO_ICO));
  audioBGM.setScale(2.f, 2.f);

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioBGM.setPosition(2*3, 2*140);

  audioSFX = audioBGM;
  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.Update(4, audioBGM);
  audioSFX.setPosition(2 * 6 + 2 * 16, 2 * 140);

  // end button
  endBtn = sf::Sprite(LOAD_TEXTURE(END_BTN));;
  endBtn.setScale(2.f, 2.f);
  endBtnAnimator.SetAnimation("BLINK");
  endBtnAnimator.SetFrame(1, endBtn);
  endBtn.setPosition(2*180, 2*10);

  // ui sprite maps
  // ascii 58 - 96
  std::list<std::string> actions;

  actions.push_back("AUDIO_BGM");
  actions.push_back("AUDIO_SFX");
  actions.push_back("SHADERS: ON");
  actions.push_back("MY KEYBOARD");
  actions.push_back("MY GAMEPAD");

  for (auto a : actions) {
    uiData::ActionItemType type = uiData::ActionItemType::KEYBOARD;

    if (a == "SHADERS: ON") {
      type = uiData::ActionItemType::DISABLED;
    }

    uiList[OPTIONS].push_back({a, sf::Vector2f(), sf::Vector2f(), type });
  }

  actions.clear();

  Logger::Log("input manager request");
  configSettings = INPUT.GetConfigSettings();

  // For keyboard keys 

  for (auto a : EventTypes::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::KEYBOARD;

    uiList[ACTIONS].push_back({ a, sf::Vector2f(), sf::Vector2f(), type });

    if (!configSettings.IsOK()) {
      boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
    }
    else {
      std::string keyStr;

      if (INPUT.ConvertKeyToString(configSettings.GetPairedInput(a), keyStr)) {
        keyHash.insert(std::make_pair(configSettings.GetPairedInput(a), a));

        boundKeys.push_back({ keyStr,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundKeys.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
      }
    }
  }
  
  maxMenuSelectionIndex += (int)actions.size();

  // For gamepad keys

  for (auto a : EventTypes::KEYS) {
    uiData::ActionItemType type = uiData::ActionItemType::GAMEPAD;

    if (!configSettings.IsOK()) {
      boundGamepadButtons.push_back({ "NO KEY", sf::Vector2f(), sf::Vector2f(), type });
    }
    else {
      auto gamepadCode = configSettings.GetPairedGamepadButton(a);
      gamepadHash.insert(std::make_pair(gamepadCode, a));

      if (gamepadCode != Gamepad::BAD_CODE) {
        std::string label = "BTN " + std::to_string((int)gamepadCode);

        switch (gamepadCode) {
        case Gamepad::DOWN:
          label = "-Y AXIS";
          break;
        case Gamepad::UP:
          label = "+Y AXIS";
          break;
        case Gamepad::LEFT:
          label = "-X AXIS";
          break;
        case Gamepad::RIGHT:
          label = "+X AXIS";
          break;
        case Gamepad::BAD_CODE:
          label = "BAD_CODE";
          break;
        }

        boundGamepadButtons.push_back({ label,  sf::Vector2f(), sf::Vector2f(), type });
      }
      else {
        boundGamepadButtons.push_back({ "NO KEY",  sf::Vector2f(), sf::Vector2f(), type });
      }
    }
  }

  leave = false;
  awaitingKey = false;
  gotoNextScene = true; // true when entering or leaving, prevents user from interacting with scene

  menuSelectionIndex = lastMenuSelectionIndex = 1; // select first item

  audioModeBGM = configSettings.GetMusicLevel();
  audioModeSFX = configSettings.GetSFXLevel();

  audioAnimator.SetAnimation("DEFAULT");
  audioAnimator.SetFrame(audioModeBGM + 1, audioBGM);
  audioAnimator.SetFrame(audioModeSFX + 1, audioSFX);

  colIndex = 0; maxCols = 3; // [options] [actions] [key]
}

void ConfigScene::onUpdate(double elapsed)
{
  bg->Update((float)elapsed);

  bool hasConfirmed = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_CONFIRM) : false ) || INPUT.GetAnyKey() == sf::Keyboard::Enter;
  bool isInSubmenu = inKeyboardList || inGamepadList;

  if (hasConfirmed && isSelectingTopMenu && !leave) {
    if (!awaitingKey) {
      using namespace swoosh::intent;
      using effect = segue<WhiteWashFade, swoosh::intent::milli<500>>;
      getController().queuePop<effect>();
      AUDIO.Play(AudioType::NEW_GAME);
      leave = true;

      configSettings.SetKeyboardHash(keyHash);
      configSettings.SetGamepadHash(gamepadHash);
      ConfigWriter writer(configSettings);
      writer.Write("options.ini");
      ConfigReader reader("options.ini");
      INPUT.SupportConfigSettings(reader);
    }
  }

  if (!leave) {
    bool hasCanceled = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_CANCEL) : false) ||
      (INPUT.GetAnyKey() == sf::Keyboard::BackSpace || INPUT.GetAnyKey() == sf::Keyboard::Escape);

    bool hasUp    = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_UI_UP)   : false) || INPUT.GetAnyKey() == sf::Keyboard::Up;
    bool hasDown  = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_UI_DOWN) : false) || INPUT.GetAnyKey() == sf::Keyboard::Down;
    bool hasLeft  = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_UI_LEFT) : false) || INPUT.GetAnyKey() == sf::Keyboard::Left;
    bool hasRight = (INPUT.IsConfigFileValid() ? INPUT.Has(EventTypes::PRESSED_UI_RIGHT): false) || INPUT.GetAnyKey() == sf::Keyboard::Right;

    if (hasCanceled && !awaitingKey) {
      if (!isInSubmenu) {
        isSelectingTopMenu = true;
        colIndex = menuSelectionIndex = 0;
      }
      else {
        if (inGamepadList) menuSelectionIndex = 4;
        if (inKeyboardList) menuSelectionIndex = 3;

        inGamepadList = inKeyboardList = false;
        isInSubmenu = false; // this flag affects the rest of this update step
        colIndex = 0;

        isSelectingTopMenu = false;
      }
    }
    else if (awaitingKey) {
      if (inKeyboardList) {
        auto key = INPUT.GetAnyKey();

        if (key != sf::Keyboard::Unknown) {
          std::string boundKey = "";

          if (INPUT.ConvertKeyToString(key, boundKey)) {

            auto iter = keyHash.begin();

            while (iter != keyHash.end()) {
              if (iter->second == EventTypes::KEYS[menuSelectionIndex]) break;
              iter++;
            }

            if (iter != keyHash.end()) {
              keyHash.erase(iter);
            }

            keyHash.insert(std::make_pair(key, EventTypes::KEYS[menuSelectionIndex]));

            std::transform(boundKey.begin(), boundKey.end(), boundKey.begin(), ::toupper);
            boundKeys[menuSelectionIndex].label = boundKey;
            AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

            awaitingKey = false;
          }
        }
      }

      if (inGamepadList) {
        // GAMEPAD
        auto gamepad = INPUT.GetAnyGamepadButton();

        if (gamepad != (Gamepad)-1) {
          auto iter = gamepadHash.begin();

          while (iter != gamepadHash.end()) {
            if (iter->second == EventTypes::KEYS[menuSelectionIndex]) break;
            iter++;
          }

          if (iter != gamepadHash.end()) {
            gamepadHash.erase(iter);
          }

          gamepadHash.insert(std::make_pair(gamepad, EventTypes::KEYS[menuSelectionIndex]));

          std::string label = "BTN " + std::to_string((int)gamepad);

          switch (gamepad) {
          case Gamepad::DOWN:
            label = "-Y AXIS";
            break;
          case Gamepad::UP:
            label = "+Y AXIS";
            break;
          case Gamepad::LEFT:
            label = "-X AXIS";
            break;
          case Gamepad::RIGHT:
            label = "+X AXIS";
            break;
          case Gamepad::BAD_CODE:
            label = "BAD_CODE";
            break;
          }

          boundGamepadButtons[menuSelectionIndex].label = label;

          AUDIO.Play(AudioType::CHIP_DESC_CLOSE);

          awaitingKey = false;
        }
      }
    }
    else if (hasUp) {
      if (menuSelectionIndex == 0 && !isSelectingTopMenu) {
        isSelectingTopMenu = true;
        colIndex = 0;
      }
      else {
        menuSelectionIndex--;
      }
    } else if (hasDown) {
      if (menuSelectionIndex == 0 && isSelectingTopMenu) {
        isSelectingTopMenu = false;
      }
      else {
        menuSelectionIndex++;
      }
    }
    else if (hasLeft) {
      /*if (!isSelectingTopMenu) {
        colIndex--;
      }*/
    }
    else if (hasRight) {
      /*if (!isSelectingTopMenu && !isInSubmenu) {
        colIndex++;
      }*/
    }
    else if (hasConfirmed) {
      // bg audio
      if (menuSelectionIndex == 0 && colIndex == 0) {
        audioModeBGM = (audioModeBGM+1) % 4;
        AUDIO.SetStreamVolume (((audioModeBGM)/3.0f)*100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeBGM + 1, audioBGM);
        configSettings.SetMusicLevel(audioModeBGM);
      }
      else if (menuSelectionIndex == 1 && colIndex == 0) {
        audioModeSFX = (audioModeSFX + 1) % 4;
        AUDIO.SetChannelVolume(((audioModeSFX) / 3.0f)*100.0f);
        audioAnimator.SetAnimation("DEFAULT");
        audioAnimator.SetFrame(audioModeSFX + 1, audioSFX);
        AUDIO.Play(AudioType::BUSTER_PEA);
        configSettings.SetSFXLevel(audioModeSFX);
      }
      else if (menuSelectionIndex == 2 && colIndex == 0) {
        // TODO: Shader Toggle
        AUDIO.Play(AudioType::CHIP_ERROR);

      }
      else if (menuSelectionIndex == 3 && colIndex == 0) {
        inKeyboardList = true;
        inGamepadList = false;
        colIndex = 1;
        menuSelectionIndex = 0; // move the row cursor to the top
      }
      else if (menuSelectionIndex == 4 && colIndex == 0) {
        inGamepadList = true;
        inKeyboardList = false;
        colIndex = 1;
        menuSelectionIndex = 0; // move the row cursor to the top
      }
      else if(!awaitingKey) {
        awaitingKey = true;
        AUDIO.Play(AudioType::CHIP_DESC);
      }
    }
  }

  if (isInSubmenu) {
    colIndex = std::max(1, colIndex);
  }
  else {
    colIndex = std::max(0, colIndex);
  }

  colIndex = std::min(colIndex, maxCols - 1 - 1); // we share the last colum: bound keys

  menuSelectionIndex = std::max(0, menuSelectionIndex);

  if (colIndex != BOUNDKEYS) {
    maxMenuSelectionIndex = int(uiList[colIndex].size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }
  else {
    maxMenuSelectionIndex = int(boundKeys.size()) - 1;
    menuSelectionIndex = std::min(maxMenuSelectionIndex, menuSelectionIndex);
  }

  if (lastMenuSelectionIndex != menuSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_SELECT);

    lastMenuSelectionIndex = menuSelectionIndex;
  }

  lastMenuSelectionIndex = menuSelectionIndex;

  for (int j = 0; j < maxCols; j++) {
    std::vector<uiData>* container = &boundKeys;
    
    if (inGamepadList) {
      container = &boundGamepadButtons;
    }

    if (j != BOUNDKEYS) {
      container = &uiList[j];
    }

    for (int i = 0; i < container->size(); i++) {
      auto w = 0.3f;
      auto diff = i - menuSelectionIndex;
      float scale = 1.0f - (w*abs(diff));
      auto colSpan = 45;
      auto lineSpan = 15;
      scale = std::max(scale, 0.8f);
      scale = std::min(scale, 1.0f);

      //scale = 1.0f;
      auto starty = 40;
      auto delta = 48.0f *float(elapsed);

      auto s = sf::Vector2f(2.f*scale, 2.f*scale);
      auto slerp = sf::Vector2f(swoosh::ease::interpolate(delta, s.x, (*container)[i].scale.x), swoosh::ease::interpolate(delta, s.y, (*container)[i].scale.y));
      (*container)[i].scale = slerp;

      // recalculate, unbounded this time
      scale = 1.0f - (w*abs(diff));
      scale = std::max(scale, 0.2f);
      scale = std::min(scale, 2.0f);

      s = sf::Vector2f(2.f*scale, 2.f*scale);

      auto limit = std::min(menuSelectionIndex, maxMenuSelectionIndex - 2); // stop the screen from rolling up when reaching this point of the list
      auto pos = sf::Vector2f(/*2.f * (5 + (3 * s.x))*/2.0f+2.0f*(j*colSpan), 2.f*(starty + (i * lineSpan) - (limit * 10)));
      auto lerp = sf::Vector2f(swoosh::ease::interpolate(delta, pos.x, (*container)[i].position.x), swoosh::ease::interpolate(delta, pos.y, (*container)[i].position.y));
      
      if (j != BOUNDKEYS) {
        (*container)[i].position = lerp;
      }
      else {
        // this effectively will right-align the bound keys
        (*container)[i].position.x = 238 * 2.f; //240px is the entire size of the screen
        (*container)[i].position.y = lerp.y;
      }

      if (j == 0 && colIndex == 0) {
        if (i != menuSelectionIndex) {
          (*container)[i].alpha = 150;

          if (awaitingKey) {
            (*container)[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = 255;
        }
      }
      else if(j > 0 && colIndex > 0) {
        if (i != menuSelectionIndex) {
          (*container)[i].alpha = 150;

          if (awaitingKey) {
            (*container)[i].alpha = 50;
          }
        }
        else {
          (*container)[i].alpha = 255;
        }
      }
      else {
        (*container)[i].alpha = 100;

        if (awaitingKey) {
          (*container)[i].alpha = 50;
        }
      }

      if (isSelectingTopMenu) {
        (*container)[i].alpha = 100;
      }
      // end for-loop
    }
  }

  // Make endBtn stick at the top of the screen
  endBtn.setPosition(endBtn.getPosition().x, uiList[0][0].position.y - 60);

  if (isSelectingTopMenu) {
    endBtnAnimator.SetFrame(2, endBtn);
  }
  else {
    endBtnAnimator.SetFrame(1, endBtn);
  }
}

void ConfigScene::onDraw(sf::RenderTexture & surface)
{
  ENGINE.SetRenderSurface(surface);
  ENGINE.Draw(bg);

  ENGINE.Draw(endBtn);

  ENGINE.Draw(audioBGM);
  ENGINE.Draw(audioSFX);

  // Draw options
  DrawMenuOptions();

  if (inKeyboardList) {
    // Keyboard keys
    DrawMappedKeyMenu(boundKeys);
  }

  if (inGamepadList) {
    // Gamepad keys
    DrawMappedKeyMenu(boundGamepadButtons);
  }
}

void ConfigScene::DrawMenuOptions()
{
  for (int i = 0; i < 3; i++) {
    if (i > 0) {
      // Expand menu if the gamepad or keyboard option was selected
      if (!inGamepadList && !inKeyboardList) return;
    }

    for (auto ui : uiList[i]) {
      if (ui.label == "AUDIO_BGM") {
        audioBGM.setScale(ui.scale);
        audioBGM.setPosition(ui.position.x, ui.position.y);
        audioBGM.setColor(sf::Color(255, 0, 255, ui.alpha));

      }
      else if (ui.label == "AUDIO_SFX") {
        audioSFX.setScale(ui.scale);
        audioSFX.setPosition(ui.position.x, ui.position.y);
        audioSFX.setColor(sf::Color(10, 165, 255, ui.alpha));

      }
      else {
        int offset = 0;
        for (auto c : ui.label) {
          if (c == ' ') {
            offset += 11; continue;
          }

          std::string sc = "SMALL_";
          sc += c;
          uiAnimator.SetAnimation(sc);
          uiAnimator.SetFrame(1, uiSprite);
          uiSprite.setScale(ui.scale);
          uiSprite.setPosition(2.f*(ui.position.x + offset), ui.position.y);

          if (ui.type == uiData::ActionItemType::KEYBOARD) {
            uiSprite.setColor(sf::Color(255, 165, 0, ui.alpha));
          }
          else if (ui.type == uiData::ActionItemType::DISABLED) {
            uiSprite.setColor(sf::Color(255, 0, 0, ui.alpha));
          }
          else {
            uiSprite.setColor(sf::Color(10, 165, 255, ui.alpha));
          }

          ENGINE.Draw(uiSprite);
          offset += (int)uiSprite.getLocalBounds().width + 2;
        }
      }
    }
  }
}

void ConfigScene::DrawMappedKeyMenu(std::vector<uiData>& container)
{
  for (auto ui : container) {
    int offset = 0;

    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11; continue;
      }

      std::string sc = "SMALL_";
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition(2.f*(ui.position.x + offset), ui.position.y);

      offset += (int)uiSprite.getLocalBounds().width + 2;

      ENGINE.Draw(uiSprite);

    }

    auto totalOffset = offset;
    offset = 0;

    // 2nd pass for right alignment adjustment
    for (auto c : ui.label) {
      if (c == ' ') {
        offset += 11;
        continue;
      }

      std::string sc = "SMALL_";
      sc += c;

      uiAnimator.SetAnimation(sc);

      uiAnimator.SetFrame(1, uiSprite);
      uiSprite.setScale(ui.scale);
      uiSprite.setPosition((ui.position.x + 2.0f*(offset - totalOffset)), ui.position.y);

      if (ui.type == uiData::ActionItemType::KEYBOARD) {
        uiSprite.setColor(sf::Color(255, 165, 0, ui.alpha));
      }
      else {
        uiSprite.setColor(sf::Color(10, 165, 255, ui.alpha));
      }

      ENGINE.Draw(uiSprite);
      offset += (int)uiSprite.getLocalBounds().width + 2;
    }
  }
}

void ConfigScene::onStart()
{
  AUDIO.Stream("resources/loops/config.ogg", false);
}

void ConfigScene::onLeave()
{
  AUDIO.StopStream();
}

void ConfigScene::onExit()
{
}

void ConfigScene::onEnter()
{
  AUDIO.StopStream();
}

void ConfigScene::onResume()
{
}

void ConfigScene::onEnd()
{
}
