// #include "Application.h"
// #include "IO/StatusLed.h"
// #include "Sync/SyncManager.h"

// // Constants for menu timing
// #define GROUP_INFO_DISPLAY_TIME 3000 // 3 seconds
// #define DOUBLE_CLICK_TIME 500        // 500ms for double click detection
// #define SYNC_UPDATE_INTERVAL 50      // 50ms for smooth sync LED updates
// #define GROUP_ID_MAX_DIGITS 8        // Maximum digits for group ID input

// void Application::btnLoop()
// {
//   if (!appInitialized)
//     return;

//   // Handle menu navigation first
//   handleMenuNavigation();

//   // Update synced LED timing based on current state
//   updateSyncedLEDTiming();

//   // Display appropriate LED status based on current menu state
//   switch (menuContext.currentState)
//   {
//   case MenuState::NORMAL_MODE:
//     displayNormalMode();
//     break;
//   case MenuState::GROUP_INFO:
//     displayGroupInfo();
//     break;
//   case MenuState::GROUP_MENU:
//     displayGroupMenu();
//     break;
//   }
// }

// void Application::handleMenuNavigation()
// {
//   uint32_t currentTime = millis();

//   // Handle Boot button for menu state transitions
//   if (BtnBoot.clicks == 2)
//   {
//     // Double click: Toggle between normal mode and group info
//     if (menuContext.currentState == MenuState::NORMAL_MODE)
//     {
//       menuContext.currentState = MenuState::GROUP_INFO;
//       menuContext.groupInfoDisplayStart = currentTime;
//     }
//     else if (menuContext.currentState == MenuState::GROUP_INFO)
//     {
//       menuContext.currentState = MenuState::GROUP_MENU;
//       menuContext.inGroupMenu = true;
//     }
//     else if (menuContext.currentState == MenuState::GROUP_MENU)
//     {
//       menuContext.currentState = MenuState::NORMAL_MODE;
//       menuContext.inGroupMenu = false;
//       menuContext.editingGroupId = false;
//     }
//     BtnBoot.clicks = 0;
//   }
//   else if (BtnBoot.clicks == 1)
//   {
//     // Single click: Mode cycling in normal mode, menu actions in group menu
//     if (menuContext.currentState == MenuState::NORMAL_MODE)
//     {
//       // Cycle through application modes
//       mode = static_cast<ApplicationMode>((static_cast<uint8_t>(mode) + 1) % NUM_MODES);
//       preferences.putUInt("mode", static_cast<uint8_t>(mode));
//     }
//     else if (menuContext.currentState == MenuState::GROUP_MENU)
//     {
//       executeGroupMenuAction();
//     }
//     BtnBoot.clicks = 0;
//   }

//   // Handle navigation buttons in group menu
//   if (menuContext.currentState == MenuState::GROUP_MENU)
//   {
//     if (BtnNext.clicks == 1)
//     {
//       if (menuContext.editingGroupId)
//       {
//         // Navigate through group ID digits
//         if (menuContext.groupIdDigitIndex < GROUP_ID_MAX_DIGITS - 1)
//           menuContext.groupIdDigitIndex++;
//       }
//       else
//       {
//         // Navigate through menu options
//         int optionCount = static_cast<int>(GroupMenuOption::BACK_TO_NORMAL) + 1;
//         menuContext.selectedOption = static_cast<GroupMenuOption>(
//             (static_cast<int>(menuContext.selectedOption) + 1) % optionCount);
//       }
//       BtnNext.clicks = 0;
//     }

//     if (BtnPrev.clicks == 1)
//     {
//       if (menuContext.editingGroupId)
//       {
//         // Navigate through group ID digits backwards
//         if (menuContext.groupIdDigitIndex > 0)
//           menuContext.groupIdDigitIndex--;
//       }
//       else
//       {
//         // Navigate through menu options backwards
//         int optionCount = static_cast<int>(GroupMenuOption::BACK_TO_NORMAL) + 1;
//         int currentOption = static_cast<int>(menuContext.selectedOption);
//         menuContext.selectedOption = static_cast<GroupMenuOption>(
//             (currentOption - 1 + optionCount) % optionCount);
//       }
//       BtnPrev.clicks = 0;
//     }

//     if (BtnSel.clicks == 1)
//     {
//       if (menuContext.editingGroupId)
//       {
//         // Increment digit value
//         uint32_t digitValue = (menuContext.groupIdToJoin /
//                                static_cast<uint32_t>(pow(10, GROUP_ID_MAX_DIGITS - 1 - menuContext.groupIdDigitIndex))) %
//                               10;
//         digitValue = (digitValue + 1) % 10;

//         uint32_t powerOf10 = static_cast<uint32_t>(pow(10, GROUP_ID_MAX_DIGITS - 1 - menuContext.groupIdDigitIndex));
//         menuContext.groupIdToJoin = (menuContext.groupIdToJoin / (powerOf10 * 10)) * (powerOf10 * 10) +
//                                     digitValue * powerOf10 +
//                                     (menuContext.groupIdToJoin % powerOf10);
//       }
//       BtnSel.clicks = 0;
//     }
//   }

//   // Auto-return to normal mode from group info after timeout
//   if (menuContext.currentState == MenuState::GROUP_INFO &&
//       currentTime - menuContext.groupInfoDisplayStart > GROUP_INFO_DISPLAY_TIME)
//   {
//     menuContext.currentState = MenuState::NORMAL_MODE;
//   }
// }

// void Application::displayNormalMode()
// {
//   // Display current application mode on status LEDs
//   switch (mode)
//   {
//   case ApplicationMode::NORMAL:
//     statusLed1.setColor(0, 255, 0); // Green
//     statusLed2.setColor(0, 255, 0);
//     break;
//   case ApplicationMode::TEST:
//     statusLed1.setColor(255, 0, 255); // Magenta
//     statusLed2.setColor(255, 0, 255);
//     break;
//   case ApplicationMode::REMOTE:
//     statusLed1.setColor(0, 0, 255); // Blue
//     statusLed2.setColor(0, 0, 255);
//     break;
//   case ApplicationMode::OFF:
//     statusLed1.setColor(255, 0, 0); // Red
//     statusLed2.setColor(255, 0, 0);
//     break;
//   }
// }

// void Application::displayGroupInfo()
// {
//   SyncManager *syncMgr = SyncManager::getInstance();

//   if (syncMgr->isInGroup())
//   {
//     // In a group: Show group status
//     if (syncMgr->isGroupMaster())
//     {
//       // Master: Solid cyan
//       statusLed1.setColor(0, 255, 255);
//       statusLed2.setColor(0, 255, 255);
//     }
//     else
//     {
//       // Member: Blinking cyan
//       uint32_t blinkTime = millis() % 1000;
//       if (blinkTime < 500)
//       {
//         statusLed1.setColor(0, 255, 255);
//         statusLed2.setColor(0, 255, 255);
//       }
//       else
//       {
//         statusLed1.off();
//         statusLed2.off();
//       }
//     }
//   }
//   else
//   {
//     // Not in a group: Show search status
//     uint32_t searchTime = millis() % 2000;
//     if (searchTime < 200)
//     {
//       statusLed1.setColor(255, 255, 0); // Yellow flash
//       statusLed2.off();
//     }
//     else if (searchTime < 400)
//     {
//       statusLed1.off();
//       statusLed2.setColor(255, 255, 0);
//     }
//     else
//     {
//       statusLed1.off();
//       statusLed2.off();
//     }
//   }
// }

// void Application::displayGroupMenu()
// {
//   // Display current menu option using LED patterns
//   uint32_t menuTime = millis() % 1000;

//   switch (menuContext.selectedOption)
//   {
//   case GroupMenuOption::CREATE_GROUP:
//   {
//     // Green pulsing
//     uint8_t brightness = static_cast<uint8_t>(128 + 127 * sin(menuTime * 2 * PI / 1000));
//     statusLed1.setColor(0, brightness, 0);
//     statusLed2.setColor(0, brightness, 0);
//   }
//   break;

//   case GroupMenuOption::JOIN_GROUP:
//   {
//     // Blue pulsing (editing mode shows different pattern)
//     if (menuContext.editingGroupId)
//     {
//       // Show digit position with alternating pattern
//       if ((millis() / 200) % 2 == 0)
//       {
//         statusLed1.setColor(0, 0, 255);
//         statusLed2.setColor(255, 255, 255);
//       }
//       else
//       {
//         statusLed1.setColor(255, 255, 255);
//         statusLed2.setColor(0, 0, 255);
//       }
//     }
//     else
//     {
//       uint8_t brightness = static_cast<uint8_t>(128 + 127 * sin(menuTime * 2 * PI / 1000));
//       statusLed1.setColor(0, 0, brightness);
//       statusLed2.setColor(0, 0, brightness);
//     }
//   }
//   break;

//   case GroupMenuOption::LEAVE_GROUP:
//   {
//     // Red pulsing
//     uint8_t brightness = static_cast<uint8_t>(128 + 127 * sin(menuTime * 2 * PI / 1000));
//     statusLed1.setColor(brightness, 0, 0);
//     statusLed2.setColor(brightness, 0, 0);
//   }
//   break;

//   case GroupMenuOption::AUTO_JOIN_TOGGLE:
//   {
//     // Purple/White alternating
//     if (menuTime < 500)
//     {
//       statusLed1.setColor(128, 0, 128);
//       statusLed2.setColor(128, 0, 128);
//     }
//     else
//     {
//       statusLed1.setColor(255, 255, 255);
//       statusLed2.setColor(255, 255, 255);
//     }
//   }
//   break;

//   case GroupMenuOption::BACK_TO_NORMAL:
//   {
//     // Orange blinking
//     if (menuTime < 300)
//     {
//       statusLed1.setColor(255, 165, 0);
//       statusLed2.setColor(255, 165, 0);
//     }
//     else
//     {
//       statusLed1.off();
//       statusLed2.off();
//     }
//   }
//   break;
//   }
// }

// void Application::executeGroupMenuAction()
// {
//   SyncManager *syncMgr = SyncManager::getInstance();

//   if (menuContext.selectedOption == GroupMenuOption::CREATE_GROUP)
//   {
//     syncMgr->createGroup();
//     statusLed1.blink(0x00FF00, 3, 3);
//     statusLed2.blink(0x00FF00, 3, 3);
//   }
//   else if (menuContext.selectedOption == GroupMenuOption::JOIN_GROUP)
//   {
//     if (!menuContext.editingGroupId)
//     {
//       menuContext.editingGroupId = true;
//       menuContext.groupIdToJoin = 0;
//       menuContext.groupIdDigitIndex = 0;
//     }
//     else
//     {
//       if (menuContext.groupIdToJoin > 0)
//       {
//         syncMgr->joinGroup(menuContext.groupIdToJoin);
//         statusLed1.blink(0x0000FF, 3, 3);
//         statusLed2.blink(0x0000FF, 3, 3);
//       }
//       menuContext.editingGroupId = false;
//     }
//   }
//   else if (menuContext.selectedOption == GroupMenuOption::LEAVE_GROUP)
//   {
//     if (syncMgr->isInGroup())
//     {
//       syncMgr->leaveGroup();
//       statusLed1.blink(0xFF0000, 3, 3);
//       statusLed2.blink(0xFF0000, 3, 3);
//     }
//   }
//   else if (menuContext.selectedOption == GroupMenuOption::AUTO_JOIN_TOGGLE)
//   {
//     syncMgr->enableAutoJoin(!syncMgr->isAutoJoinEnabled());
//     uint32_t color = syncMgr->isAutoJoinEnabled() ? 0x800080 : 0xFFFFFF;
//     statusLed1.blink(color, 3, 3);
//     statusLed2.blink(color, 3, 3);
//   }
//   else if (menuContext.selectedOption == GroupMenuOption::BACK_TO_NORMAL)
//   {
//     menuContext.currentState = MenuState::NORMAL_MODE;
//     menuContext.inGroupMenu = false;
//     menuContext.editingGroupId = false;
//   }
// }

// void Application::updateSyncedLEDTiming()
// {
//   uint32_t currentTime = millis();

//   // Only update synced LED when in normal mode and at controlled intervals
//   if (menuContext.currentState == MenuState::NORMAL_MODE &&
//       currentTime - menuContext.lastSyncUpdate > SYNC_UPDATE_INTERVAL)
//   {
//     menuContext.lastSyncUpdate = currentTime;
//     SyncManager::getInstance()->updateSyncedLED();
//   }
// }