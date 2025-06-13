#include "Battery.h"
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>

#define minVoltage 3.2f
#define maxVoltage 5.0f

const adc1_channel_t ADC_CHANNEL = ADC1_GPIO2_CHANNEL;

// ADC Configuration
const adc_atten_t ATTENUATION = ADC_ATTEN_DB_12;     // 11dB attenuation for full range
const adc_bits_width_t BIT_WIDTH = ADC_WIDTH_BIT_12; // 12-bit resolution

// ADC calibration characteristics
static esp_adc_cal_characteristics_t adc_chars;
static bool adc_initialized = false;

static float batteryVoltage = 1.0f;
static float batteryVoltageSmooth = 1.0f;

// Initialize ADC with calibration
void initADC()
{
  if (!adc_initialized)
  {
    // Configure ADC
    adc1_config_width(BIT_WIDTH);
    adc1_config_channel_atten(ADC_CHANNEL, ATTENUATION);

    // Characterize ADC for calibration
    esp_adc_cal_characterize(ADC_UNIT_1, ATTENUATION, BIT_WIDTH, 0, &adc_chars);

    adc_initialized = true;
  }
}

float batteryGetVoltage()
{
  return batteryVoltage;
}

float batteryGetVoltageSmooth()
{
  return batteryVoltageSmooth;
}

int batteryGetPercentage()
{
  float percentage = ((constrain(batteryVoltage, minVoltage, maxVoltage) - minVoltage) / (maxVoltage - minVoltage)) * 100.0f;

  return static_cast<int>(percentage);
}

int batteryGetPercentageSmooth()
{
  float percentage = ((constrain(batteryVoltageSmooth, minVoltage, maxVoltage) - minVoltage) / (maxVoltage - minVoltage)) * 100.0f;

  return static_cast<int>(percentage);
}

void batteryUpdate()
{
  // Initialize ADC if not already done
  initADC();

  int adcRaw = adc1_get_raw(ADC_CHANNEL);
  uint32_t adcVoltage_mV = esp_adc_cal_raw_to_voltage(adcRaw, &adc_chars);

  // Convert to volts
  float adcVoltage = adcVoltage_mV / 1000.0f;

  // adcVoltage += 0.029; // calibration

  batteryVoltage = adcVoltage * (BATTERY_SENSE_R1 + BATTERY_SENSE_R2) / BATTERY_SENSE_R2;

  // Apply smoothing filter
  if (batteryVoltageSmooth <= 1.0f)
  {
    batteryVoltageSmooth = batteryVoltage;
  }
  else
  {
    batteryVoltageSmooth = batteryVoltageSmooth * 0.9 + batteryVoltage * 0.1;
  }
}