#include <LiquidCrystal.h>
#include "ClickButton.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//ClickButton Select (PIN, ANALOG VALUE MIN, ANALOG VALUE MAX);
ClickButton Select (A0, 630, 650);
ClickButton Up     (A0, 90, 110);
ClickButton Down   (A0, 200, 300);
ClickButton Left   (A0, 400, 420);
ClickButton Right  (A0, 0, 10);

void setup() {
  lcd.begin(16, 2);

  lcd.setCursor(3, 0);
  lcd.print("BUTTON TEST");
  delay(2000);
  lcd.clear();
}

void loop() {
  Select.Update();
  Up.Update();
  Down.Update();
  Left.Update();
  Right.Update();

  if (Select.clicks != 0) {
    lcd.setCursor(0, 0);
    lcd.print(Select.clicks);
    lcd.print(" ");
  }
  if (Up.clicks != 0) {
    lcd.setCursor(6, 0);
    lcd.print(Up.clicks);
    lcd.print(" ");
  }
  if (Down.clicks != 0) {
    lcd.setCursor(9, 0);
    lcd.print(Down.clicks);
    lcd.print(" ");
  }
  if (Left.clicks != 0) {
    lcd.setCursor(3, 0);
    lcd.print(Left.clicks);
    lcd.print(" ");
  }
  if (Right.clicks != 0) {
    lcd.setCursor(12, 0);
    lcd.print(Right.clicks);
    lcd.print(" ");
  }

  if (button() != "NO BUTTON") {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(button());
    lcd.setCursor(10, 1);
    lcd.print(analogRead(A0));
    while (button() != "NO BUTTON") {
      Select.Update();
      Up.Update();
      Down.Update();
      Left.Update();
      Right.Update();
    }
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("PUSH BUTTON");
  }
}

String button() {
  if (Select.depressed)      return "SELECT";
  else if (Up.depressed)     return "UP";
  else if (Down.depressed)   return "DOWN";
  else if (Left.depressed)   return "LEFT";
  else if (Right.depressed)  return "RIGHT";
  else                       return "NO BUTTON";
}
