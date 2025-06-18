// /*
//  * LED MENU SYSTEM EXAMPLE - Complete Implementation Guide
//  * =======================================================
//  *
//  * This file demonstrates how to use the enhanced LEDMenu system with hierarchical
//  * sub-menus for ESP32 Tail Lights. The system now supports complex menu structures
//  * with multiple levels of navigation.
//  *
//  * HARDWARE REQUIREMENTS:
//  * ----------------------
//  * - 4 Buttons: BtnBoot, BtnPrev, BtnSel, BtnNext
//  * - 2 Status LEDs: statusLed1, statusLed2
//  *
//  * MENU NAVIGATION:
//  * ----------------
//  * 1. ENTERING MENU:
//  *    - Long press SELECT button (hold for ~1 second) to enter menu mode
//  *    - LEDs will change from dim green (idle) to menu indication mode
//  *
//  * 2. NAVIGATING MENU ITEMS:
//  *    - NEXT button: Move to next menu item
//  *    - PREV button: Move to previous menu item
//  *    - Each menu item has a unique LED pattern for identification
//  *
//  * 3. ENTERING SUB-MENUS:
//  *    - SELECT button on submenu items: Enter the submenu
//  *    - Sub-menus have their own set of items with unique patterns
//  *
//  * 4. EDITING VALUES:
//  *    - SELECT button on toggle/select items: Enter editing mode
//  *    - LEDs show alternating red/blue pattern during editing
//  *
//  * 5. MODIFYING VALUES (in editing mode):
//  *    - NEXT button: Increase/next value
//  *    - PREV button: Decrease/previous value
//  *    - SELECT button: Save changes and exit editing
//  *    - BOOT button: Cancel changes and exit editing
//  *
//  * 6. NAVIGATING BACK:
//  *    - BOOT button: Go back one level (submenu to parent, or exit main menu)
//  *    - "Back" menu items: Explicit back navigation
//  *
//  * 7. AUTO-EXIT:
//  *    - Menu exits automatically after 10 seconds of inactivity
//  *
//  * MENU STRUCTURE:
//  * ---------------
//  * Main Menu:
//  * ├── Sync Settings (submenu)
//  * │   ├── Create Group
//  * │   ├── Leave Group
//  * │   ├── Auto-Join (toggle)
//  * │   └── Back
//  * ├── LED Settings (submenu)
//  * │   ├── Brightness (0-255)
//  * │   ├── Speed (Slow/Medium/Fast/Turbo)
//  * │   ├── Mode (Normal/Pulse/Flash/Rainbow)
//  * │   └── Back
//  * ├── System Settings (submenu)
//  * │   ├── LED Enable (toggle)
//  * │   ├── Save Settings
//  * │   ├── Reset Defaults
//  * │   └── Back
//  * └── Exit
//  */

// #include "Application.h"
// #include "IO/StatusLed.h"
// #include "IO/LEDMenu.h"
// #include "Sync/SyncManager.h"

// // Settings variables
// static bool ledEnabled = true;
// static bool autoJoinEnabled = false;
// static int brightness = 128;
// static int speed = 2; // 0=Slow, 1=Medium, 2=Fast, 3=Turbo
// static int mode = 0;  // 0=Normal, 1=Pulse, 2=Flash, 3=Rainbow

// // Options for select menu items
// static std::vector<String> speedOptions = {"Slow", "Medium", "Fast", "Turbo"};
// static std::vector<String> modeOptions = {"Normal", "Pulse", "Flash", "Rainbow"};

// // Sub-menu containers
// static std::vector<LEDMenuItem> syncMenuItems;
// static std::vector<LEDMenuItem> ledMenuItems;
// static std::vector<LEDMenuItem> systemMenuItems;

// // Action functions
// void saveSettings()
// {
//   // In real implementation, save to preferences/EEPROM
//   ESP_LOGI("Settings", "Settings saved!");
//   statusLed1.blink(0x00FF00, 3, 3); // Green success
//   statusLed2.blink(0x00FF00, 3, 3);
// }

// void resetDefaults()
// {
//   ledEnabled = true;
//   autoJoinEnabled = false;
//   brightness = 128;
//   speed = 2;
//   mode = 0;
//   ESP_LOGI("Settings", "Reset to defaults!");
//   statusLed1.blink(0xFF8000, 5, 5); // Orange warning
//   statusLed2.blink(0xFF8000, 5, 5);
// }

// void Application::btnLoop()
// {
//   static bool menuInitialized = false;

//   if (!appInitialized)
//     return;

//   // Initialize the hierarchical LED menu system once
//   if (!menuInitialized)
//   {
//     menuInitialized = true;
//     auto *sync = SyncManager::getInstance();

//     // Build Sync Settings submenu
//     syncMenuItems.clear();

//     // Create Group - Green pulsing
//     syncMenuItems.push_back(LEDMenuItem::createAction(
//         "Create Group",
//         [sync]()
//         {
//           sync->createGroup();
//           statusLed1.blink(0x00FF00, 3, 3);
//           statusLed2.blink(0x00FF00, 3, 3);
//         },
//         LEDPattern::createPulse(0x00FF00, 0x00FF00, 1000, 128, 255)));

//     // Leave Group - Red pulsing
//     syncMenuItems.push_back(LEDMenuItem::createAction(
//         "Leave Group",
//         [sync]()
//         {
//           if (sync->isInGroup())
//           {
//             sync->leaveGroup();
//             statusLed1.blink(0xFF0000, 3, 3);
//             statusLed2.blink(0xFF0000, 3, 3);
//           }
//         },
//         LEDPattern::createPulse(0xFF0000, 0xFF0000, 1000, 128, 255)));

//     // Auto-Join Toggle - Purple/White alternating
//     syncMenuItems.push_back(LEDMenuItem::createToggle(
//         "Auto-Join",
//         &autoJoinEnabled,
//         LEDPattern::createAlternating(0x800080, 0xFFFFFF, 0x800080, 0xFFFFFF, 500),
//         LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250)));

//     // Back item
//     syncMenuItems.push_back(LEDMenuItem::createBack());

//     // Build LED Settings submenu
//     ledMenuItems.clear();

//     // Brightness - Yellow pulse
//     ledMenuItems.push_back(LEDMenuItem::createSelect(
//         "Brightness",
//         &brightness,
//         0, 255, nullptr,
//         LEDPattern::createPulse(0xFFFF00, 0xFFFF00, 800, 100, 255),
//         LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250)));

//     // Speed selection - Cyan alternating
//     ledMenuItems.push_back(LEDMenuItem::createSelect(
//         "Speed",
//         &speed,
//         0, 3, &speedOptions,
//         LEDPattern::createAlternating(0x00FFFF, 0x000000, 0x00FFFF, 0x000000, 600),
//         LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250)));

//     // Mode selection - White flash
//     ledMenuItems.push_back(LEDMenuItem::createSelect(
//         "Mode",
//         reinterpret_cast<int *>(&mode),
//         0, 3, &modeOptions,
//         LEDPattern::createFlash(0xFFFFFF, 0xFFFFFF, 200, 800),
//         LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250)));

//     // Back item
//     ledMenuItems.push_back(LEDMenuItem::createBack());

//     // Build System Settings submenu
//     systemMenuItems.clear();

//     // LED Enable toggle - Green/Red alternating
//     systemMenuItems.push_back(LEDMenuItem::createToggle(
//         "LED Enable",
//         &ledEnabled,
//         LEDPattern::createAlternating(0x00FF00, 0xFF0000, 0x00FF00, 0xFF0000, 750),
//         LEDPattern::createAlternating(0xFF0000, 0x000000, 0x0000FF, 0x000000, 250)));

//     // Save Settings - Blue pulse
//     systemMenuItems.push_back(LEDMenuItem::createAction(
//         "Save Settings",
//         saveSettings,
//         LEDPattern::createPulse(0x0000FF, 0x0000FF, 1200, 80, 255)));

//     // Reset Defaults - Orange flash
//     systemMenuItems.push_back(LEDMenuItem::createAction(
//         "Reset Defaults",
//         resetDefaults,
//         LEDPattern::createFlash(0xFF8000, 0xFF8000, 150, 850)));

//     // Back item
//     systemMenuItems.push_back(LEDMenuItem::createBack());

//     // Build main menu with submenus
//     ledMenu.clearMenuItems();

//     // Sync Settings submenu - Green pulse
//     ledMenu.addMenuItem(LEDMenuItem::createSubMenu(
//         "Sync Settings",
//         &syncMenuItems,
//         LEDPattern::createPulse(0x00FF00, 0x008000, 1500, 100, 255)));

//     // LED Settings submenu - Yellow pulse
//     ledMenu.addMenuItem(LEDMenuItem::createSubMenu(
//         "LED Settings",
//         &ledMenuItems,
//         LEDPattern::createPulse(0xFFFF00, 0x808000, 1500, 100, 255)));

//     // System Settings submenu - Blue pulse
//     ledMenu.addMenuItem(LEDMenuItem::createSubMenu(
//         "System Settings",
//         &systemMenuItems,
//         LEDPattern::createPulse(0x0000FF, 0x000080, 1500, 100, 255)));

//     // Exit - Orange flash
//     ledMenu.addMenuItem(LEDMenuItem::createAction(
//         "Exit",
//         []()
//         { ledMenu.exit(); },
//         LEDPattern::createFlash(0xFFA500, 0xFFA500, 300, 1700)));

//     // Enable the LED menu
//     ledMenu.setEnabled(true);
//     ledMenu.begin();
//   }

//   // Handle button events and clear click counts after processing
//   auto handleButton = [](auto &btn, ButtonType type)
//   {
//     if (btn.clicks == 1)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::SINGLE_CLICK});
//       btn.clicks = 0;
//     }
//     else if (btn.clicks == 2)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::DOUBLE_CLICK});
//       btn.clicks = 0;
//     }
//     else if (btn.clicks == 3)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::TRIPLE_CLICK});
//       btn.clicks = 0;
//     }
//     else if (btn.clicks == -1)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::LONG_CLICK});
//       btn.clicks = 0;
//     }
//     else if (btn.clicks == -2)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::DOUBLE_LONG_CLICK});
//       btn.clicks = 0;
//     }
//     else if (btn.clicks == -3)
//     {
//       ledMenu.handleButtonEvent(ButtonEvent{type, ButtonEventType::TRIPLE_LONG_CLICK});
//       btn.clicks = 0;
//     }
//   };

//   // Process all button events
//   handleButton(BtnBoot, ButtonType::BOOT);
//   handleButton(BtnPrev, ButtonType::PREV);
//   handleButton(BtnSel, ButtonType::SELECT);
//   handleButton(BtnNext, ButtonType::NEXT);

//   // Update the menu system
//   ledMenu.update();
// }