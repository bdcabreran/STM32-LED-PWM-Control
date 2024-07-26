/**
 * @file led_control.c
 * @author Bayron Cabrera (bayron.nanez@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-07-12
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "led_animation.h"
#include "stdbool.h"
#include "string.h"

#define PRINT_COORDINATES 0
#define PI 3.14159

#define DEBUG_BUFFER_SIZE 100
#define LED_CONTROL_DBG 1

#if LED_CONTROL_DBG
#include "stm32l4xx_hal.h"
#include <stdio.h>
extern UART_HandleTypeDef huart2;
#define LED_CONTROL_DBG_MSG(fmt, ...)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        static char dbgBuff[DEBUG_BUFFER_SIZE];                                                                        \
        snprintf(dbgBuff, DEBUG_BUFFER_SIZE, (fmt), ##__VA_ARGS__);                                                    \
        HAL_UART_Transmit(&huart2, (uint8_t*)dbgBuff, strlen(dbgBuff), 1000);                                          \
    } while (0)
#else
#define LED_CONTROL_DBG_MSG(fmt, ...)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0)
#endif

static LED_Status_t LED_AnimationNoneExecute(LED_Handle_t* this);
static LED_Status_t LED_AnimationSolidExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationFlashExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationBlinkExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationBreathExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationFadeInExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationFadeOutExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationPulseExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationOffExecute(LED_Handle_t* this);
static LED_Status_t LED_AnimationAlternatingColorsExecute(LED_Handle_t* this, uint32_t tick);
static LED_Status_t LED_AnimationColorCycleExecute(LED_Handle_t* this, uint32_t tick);
static void         update_exp_multiplier_factor();

typedef uint32_t (*CalculateFadeBrightness_t)(uint32_t elapsed,
                                              uint32_t duration,
                                              uint32_t maxDutyCycle,
                                              bool     isFadingIn);

#if USE_LED_ANIMATION_QUADRATIC
static uint32_t GetFadeBrightnessQuadratic(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn);
static const CalculateFadeBrightness_t CalculateFadeBrightness = GetFadeBrightnessQuadratic;

#elif USE_LED_ANIMATION_EXPONENTIAL
static uint32_t
GetFadeBrightnessExponential(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn);
static const CalculateFadeBrightness_t CalculateFadeBrightness = GetFadeBrightnessExponential;

#elif USE_LED_ANIMATION_SINE
static uint32_t GetFadeBrightnessSine(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn);
static const CalculateFadeBrightness_t CalculateFadeBrightness = GetFadeBrightnessSine;

#elif USE_LED_ANIMATION_SINE_APPROX
static uint32_t GetFadeBrightnessSineAprox(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn);
static const CalculateFadeBrightness_t CalculateFadeBrightness = GetFadeBrightnessSineAprox;

#endif

static LED_Status_t LED_Animation_SetCurrentColor(LED_Handle_t* this, uint8_t* colorValues, uint32_t colorCount)
{
    if (this == NULL || colorValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    for (uint32_t i = 0; i < colorCount; i++)
    {
        this->currentColor[i] = colorValues[i];
    }

    return LED_SUCCESS;
}

static LED_Status_t
LED_Animation_SetCurrentColorFromDutyCycle(LED_Handle_t* this, uint16_t* DutyCycleValues, uint32_t DutyCycleCount)
{
    void* pwmConfig = this->controller->PwmConfig;

    // Check for null pointers to ensure safety before accessing them
    if (this == NULL || this->controller == NULL || pwmConfig == NULL || DutyCycleValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    // Ensure that the DutyCycleCount does not exceed predefined limits (if applicable)
    if (DutyCycleCount > MAX_COLOR_CHANNELS)
    {
        return LED_ERROR_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0; i < DutyCycleCount; i++)
    {
        // checks to ensure DutyCycleValues[i] is within the expected range if needed
        if (DutyCycleValues[i] > this->controller->MaxDutyCycle)
        {
            return LED_ERROR_INVALID_VALUE;
        }

        this->currentColor[i] = DUTY_CYCLE_TO_BRIGHTNESS(DutyCycleValues[i], this->controller->MaxDutyCycle);
    }

    return LED_SUCCESS;
}

/**
 * @brief Calculates the number of color channels based on the LED type.
 *
 * This function returns the count of individual color channels for different
 * LED configurations, which is useful for setting up PWM configurations or for
 * iterating over color data arrays in a generic manner.
 *
 * @param ledType The type of LED configuration as defined in LED_Type_t.
 * @return uint32_t The number of color channels for the given LED type.
 */
uint32_t CalculateColorCount(LED_Type_t ledType)
{
    switch (ledType)
    {
    case LED_TYPE_RGB:
        return 3; // Red, Green, Blue
    case LED_TYPE_RGY:
        return 3; // Red, Green, Yellow
    case LED_TYPE_RGBW:
        return 4; // Red, Green, Blue, White
    case LED_TYPE_SINGLE_COLOR:
        return 1; // Single color channel
    case LED_TYPE_DUAL_COLOR:
        return 2; // Two color channels
    case LED_TYPE_INVALID:
    case LED_TYPE_LAST:
    default:
        return 0; // Invalid or unknown LED type
    }
}

/**
 * @brief Sets the duty cycles for PWM outputs based on provided color
 * brightness values.
 *
 * This function is ideal for high-level applications where LED control is
 * needed based on color brightness values. It takes an array of brightness
 * levels (0-LED_MAX_BRIGHTNESS), converts them to duty cycles and sets these
 * duty cycles on the specified PWM configuration.
 *
 * @param this Pointer to the LED handle containing controller information.
 * @param colorValues Array of uint8_t representing color brightness levels
 * (0-LED_MAX_BRIGHTNESS).
 * @param colorCount Number of elements in the colorValues array.
 *
 * @note This function is suitable for user interfaces or systems where color
 * control is specified in terms of standard RGB color levels. It abstracts away
 * the lower-level PWM details by automatically converting color brightness into
 * PWM duty cycles.
 */
static LED_Status_t SetColorGeneric(LED_Handle_t* this, uint8_t* colorValues, uint32_t colorCount)
{

    void* pwmConfig = this->controller->PwmConfig;

    // Check for null pointers to ensure safety before accessing them
    if (this == NULL || this->controller == NULL || pwmConfig == NULL || colorValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    // Ensure that the colorCount does not exceed predefined limits (if applicable)
    if (colorCount > MAX_COLOR_CHANNELS)
    {
        return LED_ERROR_INVALID_ARGUMENT;
    }

    LED_Animation_SetCurrentColor(this, colorValues, colorCount);

    for (uint32_t i = 0; i < colorCount; i++)
    {
        // Checks to ensure colorValues[i] is within the expected range if needed
        if (colorValues[i] > LED_MAX_BRIGHTNESS)
        {
            return LED_ERROR_INVALID_VALUE;
        }

        uint32_t dutyCycle = BRIGHTNESS_TO_DUTY_CYCLE(colorValues[i], this->controller->MaxDutyCycle);

        // check if Function exists in the PWM_Channel_t structure
        if (((PWM_Channel_t*)pwmConfig)[i].setDutyCycle == NULL)
        {
            return LED_ERROR_NULL_POINTER;
        }

        ((PWM_Channel_t*)pwmConfig)[i].setDutyCycle(dutyCycle);
    }

    return LED_SUCCESS;
}

/**
 * @brief Directly sets duty cycles for PWM outputs based on provided duty cycle
 * values.
 *
 * This function provides a direct method to control the brightness of LEDs by
 * directly setting the PWM duty cycles. It is intended for scenarios where
 * precise control over the light output is necessary or where duty cycles are
 * pre-calculated.
 *
 * @param this Pointer to the LED handle containing controller information.
 * @param DutyCycleValues Array of uint16_t representing duty cycle values that
 * directly set the PWM output.
 * @param DutyCycleCount Number of elements in the DutyCycleValues array.
 *
 * @note This function is particularly useful in applications requiring precise
 * control over LED brightness or complex animations, where the duty cycle needs
 * specific adjustment or is determined by system logic, rather than
 * straightforward color brightness input.
 */
static LED_Status_t SetDutyCycleGeneric(LED_Handle_t* this, uint16_t* DutyCycleValues, uint32_t DutyCycleCount)
{

    void* pwmConfig = this->controller->PwmConfig;

    // Check for null pointers to ensure safety before accessing them
    if (this == NULL || this->controller == NULL || pwmConfig == NULL || DutyCycleValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    // Ensure that the DutyCycleCount does not exceed predefined limits (if applicable)
    if (DutyCycleCount > MAX_COLOR_CHANNELS)
    {
        return LED_ERROR_INVALID_ARGUMENT;
    }

    // LED_Animation_SetCurrentColorFromDutyCycle(this, DutyCycleValues, DutyCycleCount);

    for (uint32_t i = 0; i < DutyCycleCount; i++)
    {
        // checks to ensure DutyCycleValues[i] is within the expected range if needed
        if (DutyCycleValues[i] > this->controller->MaxDutyCycle)
        {
            return LED_ERROR_INVALID_VALUE;
        }
        uint32_t dutyCycle = DutyCycleValues[i];

        // check if Function exists in the PWM_Channel_t structure
        if (((PWM_Channel_t*)pwmConfig)[i].setDutyCycle == NULL)
        {
            return LED_ERROR_NULL_POINTER;
        }

        ((PWM_Channel_t*)pwmConfig)[i].setDutyCycle(dutyCycle);
    }

    return LED_SUCCESS;
}

/**
 * @brief
 *
 * @param this
 * @param colorValues
 * @return LED_Status_t
 */
/**
 * @brief Executes the color setting for the LED.
 *
 * This function sets the color values for the LED based on the provided color values array.
 * It calculates the number of color channels based on the LED type and sets the color values
 * using a generic function. If an error occurs during the color setting, it stops the animation
 * and calls the callback function with the animation type and the error status.
 *
 * @param this Pointer to the LED handle structure.
 * @param colorValues Pointer to the array of color values.
 * @return LED_Status_t The status of the color setting operation.
 *         - LED_SUCCESS: Color setting was successful.
 *         - LED_ERROR_NULL_POINTER: Null pointer was passed as an argument.
 */
static LED_Status_t ExecuteColorSetting(LED_Handle_t* this, uint8_t* colorValues)
{
    if (this == NULL || colorValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Status_t result = LED_SUCCESS;

    // Calculate the number of color channels based on LED type
    uint32_t colorCount = CalculateColorCount(this->controller->LedType);

    // Set the color values directly using a generic function
    result = SetColorGeneric(this, colorValues, colorCount);

    if (result != LED_SUCCESS)
    {
        // Stop the animation if an error occurred
        if (this->callback != NULL)
        {
            this->callback(this->animationType, result);
        }
        this->animationType = LED_ANIMATION_NONE;
    }

    return result;
}

/**
 * @brief Executes the duty cycle setting for the LED animation.
 *
 * This function sets the duty cycle values for the LED animation based on the provided
 * dutyCycleValues array. It calculates the number of color channels based on the LED type and sets
 * the color values using a generic function. If an error occurs during the setting of duty cycle
 * values, it stops the animation and calls the registered callback function.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param dutyCycleValues Pointer to the array of duty cycle values.
 * @return LED_Status_t The status of the execution. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
static LED_Status_t ExecuteDutyCycleSetting(LED_Handle_t* this, uint16_t* dutyCycleValues)
{
    if (this == NULL || dutyCycleValues == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Status_t result = LED_SUCCESS;

    // Calculate the number of color channels based on LED type
    uint32_t colorCount = CalculateColorCount(this->controller->LedType);

    // Set the color values directly using a generic function
    result = SetDutyCycleGeneric(this, dutyCycleValues, colorCount);

    if (result != LED_SUCCESS)
    {
        // Stop the animation if an error occurred
        if (this->callback != NULL)
        {
            this->callback(this->animationType, result);
        }
        this->animationType = LED_ANIMATION_NONE;
    }

    return result;
}

/**
 * @brief Handles the repeat logic for the LED animation.
 *
 * This function decrements the repeat times for the LED animation and checks if the animation has
 * completed. If the repeat times reach zero, the function stops the animation and calls the
 * registered callback function.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param PatternRepeatTimes The number of times the pattern should repeat. If set to -1, the
 * pattern repeats indefinitely.
 * @param StopOnCompletion Flag to indicate if the animation should stop when it completes.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or
 * LED_ANIMATION_COMPLETED if the animation has completed.
 */
static LED_Status_t HandleRepeatLogic(LED_Handle_t* this, int32_t PatternRepeatTimes, bool StopOnCompletion)
{
    if (PatternRepeatTimes != -1)
    {
        this->repeatTimes--;
        LED_CONTROL_DBG_MSG("Repeat Times: %d\r\n", this->repeatTimes);
    }
    if (this->repeatTimes == 0)
    {
        this->isRunning = false;
        if (this->callback != NULL)
        {
            this->callback(this->animationType, LED_SUCCESS);
        }

        if (StopOnCompletion)
        {
            this->controller->Stop();
        }

        return LED_ANIMATION_COMPLETED;
    }
    return LED_SUCCESS;
}

/**
 * @brief Validates the LED controller structure for the animation.
 *
 * This function checks the LED controller structure for the animation to ensure that all required
 * function pointers are not NULL. It also validates the LED type to ensure that it is a valid LED
 * type.
 *
 * @param Controller Pointer to the LED_Controller_t structure to validate.
 * @return LED_Status_t The status of the validation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_AnimationValidateController(LED_Controller_t* Controller)
{
    if (Controller == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }
    if (Controller->Start == NULL || Controller->Stop == NULL || Controller->PwmConfig == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }
    if (!IS_VALID_LED_TYPE(Controller->LedType))
    {
        return LED_ERROR_INVALID_LED_TYPE;
    }
    return LED_SUCCESS;
}

/**
 * @brief Initializes the LED animation structure.
 *
 * This function initializes the LED animation structure with the provided LED controller and
 * callback function. It sets the animation type to LED_ANIMATION_NONE and the animation data to
 * NULL.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param Controller Pointer to the LED_Controller_t structure representing the LED controller.
 * @param callback Callback function to call when the animation completes.
 * @return LED_Status_t The status of the initialization. Returns LED_SUCCESS if successful, or an
 * error code if an error occurred.
 */
LED_Status_t
LED_Animation_Init(LED_Handle_t* this, LED_Controller_t* Controller, LED_Animation_Complete_Callback callback)
{
    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Status_t status = LED_AnimationValidateController(Controller);
    if (status != LED_SUCCESS)
    {
        return status;
    }

    this->controller    = Controller;
    this->animationType = LED_ANIMATION_NONE;
    this->animationData = NULL;
    this->isRunning     = false;
    this->startTime     = 0;
    this->callback      = callback;

#if USE_LED_ANIMATION_SINE_APPROX
    update_exp_multiplier_factor();
#endif
    LED_CONTROL_DBG_MSG("LED Animation Initialized, LED type = %d\r\n", this->controller->LedType);

    return LED_SUCCESS;
}

/**
 * @brief Starts the LED animation.
 *
 * This function starts the LED animation by setting the isRunning flag to true and the start time
 * to 0.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_Start(LED_Handle_t* this)
{
    LED_Status_t status = LED_SUCCESS;

    if (this == NULL)
    {
        status = LED_ERROR_NULL_POINTER;
    }

    this->isRunning = true;
    this->startTime = 0;
    LED_CONTROL_DBG_MSG("LED Animation Started\r\n");

    return status;
}

/**
 * @brief Stops the LED animation.
 *
 * This function stops the LED animation by setting the isRunning flag to false.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_Stop(LED_Handle_t* this)
{
    LED_Status_t status = LED_SUCCESS;

    if (this == NULL)
    {
        status = LED_ERROR_NULL_POINTER;
    }

    this->isRunning = false;
    LED_CONTROL_DBG_MSG("LED Animation Stopped\r\n");

    return status;
}

/**
 * @brief Updates the LED animation based on the current tick value.
 *
 * This function updates the LED animation based on the current tick value. It executes the
 * animation based on the animation type and the provided animation data. If an error occurs during
 * the animation execution, it stops the animation and calls the registered callback function.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param tick The current tick value for the animation in ms.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_Update(LED_Handle_t* this, uint32_t tick)
{
    LED_Status_t status = LED_SUCCESS;

    // update only if tick has changed since last update call
    if (this->lastTick == tick)
    {
        return LED_SUCCESS;
    }

    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    if (IS_VALID_LED_ANIMATION_TYPE(this->animationType) == false)
    {
        return LED_ERROR_INVALID_ANIMATION_TYPE;
    }

    switch (this->animationType)
    {
    case LED_ANIMATION_NONE:
        status = LED_AnimationNoneExecute(this);
        break;
    case LED_ANIMATION_OFF:
        status = LED_AnimationOffExecute(this);
        break;
    case LED_ANIMATION_SOLID:
        status = LED_AnimationSolidExecute(this, tick);
        break;
    case LED_ANIMATION_FLASH:
        status = LED_AnimationFlashExecute(this, tick);
        break;
    case LED_ANIMATION_BLINK:
        status = LED_AnimationBlinkExecute(this, tick);
        break;
    case LED_ANIMATION_BREATH:
        status = LED_AnimationBreathExecute(this, tick);
        break;
    case LED_ANIMATION_FADE_IN:
        status = LED_AnimationFadeInExecute(this, tick);
        break;
    case LED_ANIMATION_FADE_OUT:
        status = LED_AnimationFadeOutExecute(this, tick);
        break;
    case LED_ANIMATION_PULSE:
        status = LED_AnimationPulseExecute(this, tick);
        break;
    case LED_ANIMATION_ALTERNATING_COLORS:
        status = LED_AnimationAlternatingColorsExecute(this, tick);
        break;
    case LED_ANIMATION_COLOR_CYCLE:
        status = LED_AnimationColorCycleExecute(this, tick);
        break;
    default:
        break;
    }

    this->lastTick = tick;
    return status;
}

/**
 * @brief Executes the LED animation for the LED_ANIMATION_NONE type.
 *
 * This function executes the LED animation for the LED_ANIMATION_NONE type, which is a no-op
 * function.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
static LED_Status_t LED_AnimationNoneExecute(LED_Handle_t* this)
{
    return LED_SUCCESS;
}

/**
 * @brief Sets the LED animation to the off state.
 *
 * This function sets the LED animation to the off state by setting the animation type to
 * LED_ANIMATION_OFF and the animation data to NULL.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_SetOff(LED_Handle_t* this)
{

    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_OFF;
    this->animationData = NULL;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Off\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationOffExecute(LED_Handle_t* this)
{
    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    if (this->isRunning == false)
    {
        return LED_SUCCESS;
    }

    this->controller->Stop();
    this->isRunning = false;

    if (this->callback != NULL)
    {
        this->callback(this->animationType, LED_SUCCESS);
        this->animationType = LED_ANIMATION_NONE;
    }

    return LED_SUCCESS;
}

/**
 * @brief Sets the LED animation to the solid state.
 *
 * This function sets the LED animation to the solid state by setting the animation type to
 * LED_ANIMATION_SOLID and the animation data to the provided Solid color.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param Solid Pointer to the LED_Animation_Solid_t structure representing the solid color
 * animation.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_SetSolid(LED_Handle_t* this, LED_Animation_Solid_t* Solid)
{
    if (this == NULL || Solid == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_SOLID;
    this->animationData = Solid;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Solid\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationSolidExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Animation_Solid_t* Solid = (LED_Animation_Solid_t*)this->animationData;
    if (this->isRunning == false)
    {
        return LED_SUCCESS;
    }

    if (this->startTime == 0)
    {
        this->startTime = tick;
        this->controller->Start();

        // Execute color only once
        LED_Status_t result = ExecuteColorSetting(this, (uint8_t*)Solid->color);

        if (result != LED_SUCCESS)
        {
            return result;
        }
    }

    // Check if the execution time has elapsed
    uint32_t elapsedTime = tick - this->startTime;
    if (Solid->executionTimeMs > 0 && elapsedTime >= Solid->executionTimeMs)
    {
        this->isRunning = false;
        this->controller->Stop();

        if (this->callback != NULL)
        {
            this->callback(this->animationType, LED_SUCCESS);
        }
    }

    return LED_SUCCESS;
}

/**
 * @brief Sets the LED animation to the flash state.
 *
 * This function sets the LED animation to the flash state by setting the animation type to
 * LED_ANIMATION_FLASH and the animation data to the provided Flash color.
 *
 * @param this Pointer to the LED_Handle_t structure representing the LED.
 * @param Flash Pointer to the LED_Animation_Flash_t structure representing the flash color
 * animation.
 * @return LED_Status_t The status of the operation. Returns LED_SUCCESS if successful, or an error
 * code if an error occurred.
 */
LED_Status_t LED_Animation_SetFlash(LED_Handle_t* this, LED_Animation_Flash_t* Flash)
{
    if (this == NULL || Flash == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_FLASH;
    this->animationData = Flash;
    this->isRunning     = false;
    this->repeatTimes   = Flash->repeatTimes;
    LED_CONTROL_DBG_MSG("LED Animation Flash\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationFlashExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Animation_Flash_t* Flash = (LED_Animation_Flash_t*)this->animationData;
    if (this->isRunning == false)
    {
        return LED_SUCCESS;
    }

    if (this->startTime == 0)
    {
        this->startTime   = tick; // set initial start time
        this->repeatTimes = Flash->repeatTimes;

        // Execute color only once
        LED_Status_t result = ExecuteColorSetting(this, (uint8_t*)Flash->color);
        if (result != LED_SUCCESS)
        {
            return result;
        }
    }

    uint32_t elapsedTime   = tick - this->startTime;
    uint16_t totalPeriodMs = Flash->onTimeMs + Flash->offTimeMs;

    // Determine the action based on the elapsed time within the current period
    if (elapsedTime < Flash->onTimeMs)
    {
        this->controller->Start(); // LED on during the 'on' time
    }
    else if (elapsedTime < totalPeriodMs)
    {
        this->controller->Stop(); // LED off during the 'off' time
    }

    // Reset for the next cycle or handle the completion
    if (elapsedTime >= totalPeriodMs)
    {
        this->startTime = tick; // Reset the start time for the next cycle
        HandleRepeatLogic(this, Flash->repeatTimes, false);
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetBlink(LED_Handle_t* this, LED_Animation_Blink_t* Blink)
{
    if (this == NULL || Blink == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_BLINK;
    this->animationData = Blink;
    this->isRunning     = false;
    this->repeatTimes   = Blink->repeatTimes;
    LED_CONTROL_DBG_MSG("LED Animation Blink\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationBlinkExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    LED_Animation_Blink_t* Blink = (LED_Animation_Blink_t*)this->animationData;
    if (this->isRunning == false)
    {
        return LED_SUCCESS;
    }

    if (this->startTime == 0)
    {
        this->startTime   = tick; // set initial start time
        this->repeatTimes = Blink->repeatTimes;

        // Execute color only once
        LED_Status_t result = ExecuteColorSetting(this, (uint8_t*)Blink->color);
        if (result != LED_SUCCESS)
        {
            return result;
        }
    }

    uint32_t elapsedTime = tick - this->startTime;

    // Determine if we should turn the LED on or off
    if (elapsedTime >= Blink->periodMs)
    {
        this->startTime = tick;
        HandleRepeatLogic(this, Blink->repeatTimes, false);
    }
    else if (elapsedTime >= Blink->periodMs / 2)
    {
        // Turn LED off after half the period
        this->controller->Stop();
    }
    else
    {
        // Turn LED on for the first half of the period
        this->controller->Start();
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetBreath(LED_Handle_t* this, LED_Animation_Breath_t* Breath)
{
    if (this == NULL || Breath == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_BREATH;
    this->animationData = Breath;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Breath\r\n");

    return LED_SUCCESS;
}

#if USE_LED_ANIMATION_QUADRATIC
// Computes the brightness for LED animations based on a quadratic curve.
// This function is designed for both fading in and fading out effects.
//
// Parameters:
//   elapsed      - The current time elapsed since the beginning of the animation.
//   duration     - The total duration of the LED animation.
//   maxDutyCycle - The maximum brightness level (duty cycle) for the LED.
//   isFadingIn   - A boolean flag indicating whether the LED is fading in or out.
//
// Returns:
//   The computed brightness level based on a quadratic curve.
static uint32_t GetFadeBrightnessQuadratic(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn)
{
    // Normalize time to a range from 0 to maxDutyCycle for direct scaling
    uint32_t normalizedTime = (elapsed * maxDutyCycle) / duration;
    uint32_t expBrightness;

    if (isFadingIn)
    {
        // Calculate brightness for fading in using a quadratic approximation:
        // Brightness = (NormalizedTime / MaxDutyCycle) ^ 2
        // Simplified here by avoiding division for optimization
        expBrightness = (normalizedTime * normalizedTime) / maxDutyCycle;
    }
    else
    {
        // Calculate brightness for fading out using the inverse of the quadratic approximation:
        // Brightness = (1 - NormalizedTime / MaxDutyCycle) ^ 2
        // Simplified here by computing the inverse time first and then squaring it
        uint32_t inverseTime = maxDutyCycle - normalizedTime;
        expBrightness        = (inverseTime * inverseTime) / maxDutyCycle;
    }

    return expBrightness;
}

#endif

#if USE_LED_ANIMATION_EXPONENTIAL
#include <math.h> // Required for exp() function
// Computes the brightness for LED animations based on an exponential curve.
// This function supports both exponential growth and decay for fading in and out.
//
// Parameters:
//   elapsed      - The current time elapsed since the beginning of the animation.
//   duration     - The total duration of the LED animation.
//   maxDutyCycle - The maximum brightness level for the LED.
//   isFadingIn   - A boolean indicating whether the LED is fading in or out.
//
// Returns:
//   The computed brightness level based on an exponential curve.
static uint32_t
GetFadeBrightnessExponential(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn)
{
    // Normalize progress from 0.0 to 1.0
    float    progress = (float)elapsed / duration;
    uint32_t expBrightness;
    uint8_t  exponentMultiplier = 3; // Multiplier to adjust the steepness of the curve

    if (isFadingIn)
    {
        // Exponential growth formula for fading in
        expBrightness = (exp(progress * exponentMultiplier) - 1) / (exp(exponentMultiplier) - 1) * maxDutyCycle;
    }
    else
    {
        // Exponential decay formula for fading out
        expBrightness = (exp((1 - progress) * exponentMultiplier) - 1) / (exp(exponentMultiplier) - 1) * maxDutyCycle;
    }

    return expBrightness;
}

#endif

#if USE_LED_ANIMATION_SINE
#include <math.h> // Required for exp() function
static uint32_t GetFadeBrightnessSine(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn)
{
    // Normalize progress ranging from 0.0 to 1.0
    float progress = (float)elapsed / duration;

    uint32_t brightness;
    uint8_t  exponentMultiplier = 3;

    if (isFadingIn)
    {
        // Rise phase: using the sine function for a smooth increase
        float sineInput = sin(progress * PI / 2); // Sine wave from 0 to PI/2 (0 to 1)
        brightness      = (exp(sineInput * exponentMultiplier) - 1) / (exp(exponentMultiplier) - 1) * maxDutyCycle;
    }
    else
    {
        // Fall phase: inverse of the rise phase
        float sineInput = sin((1 - progress) * PI / 2); // Sine wave from PI/2 to 0 (1 to 0)
        brightness      = (exp(sineInput * exponentMultiplier) - 1) / (exp(exponentMultiplier) - 1) * maxDutyCycle;
    }

    return brightness;
}
#endif

////////////////////////////////////////////////////////

#if USE_LED_ANIMATION_SINE_APPROX
#include <stdint.h>

#define PI 3.14159
#define PI_HALF (PI / 2)
#define EXP_MULTIPLIER 3

// Approximate sine function using a polynomial approximation for sine in the range [0, PI/2]
// sin(x) ≈ x - (x^3 / 6) + (x^5 / 120)
static inline float fast_sine(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    return x - (x3 / 6) + (x5 / 120);
}

// Precomputed exponential value for EXP_MULTIPLIER
// approximately exp(3) - 1
float EXP_MULTIPLIER_FACTOR = 19.0855f;

static inline float fast_exp(float x)
{
    // Use a polynomial approximation for exponential function
    return 1 + x + (x * x / 2) + (x * x * x / 6);
}

static void update_exp_multiplier_factor()
{
    float maxExpInput     = fast_exp(fast_sine(PI_HALF) * EXP_MULTIPLIER);
    EXP_MULTIPLIER_FACTOR = maxExpInput - 1;

    LED_CONTROL_DBG_MSG("Factor: %ld\r\n", (long)(EXP_MULTIPLIER_FACTOR * 1000));
}

static uint32_t GetFadeBrightnessSineAprox(uint32_t elapsed, uint32_t duration, uint32_t maxDutyCycle, bool isFadingIn)
{
    // Normalize progress ranging from 0.0 to 1.0
    float progress = (float)elapsed / duration;

    float sineInput;
    if (isFadingIn)
    {
        // Rise phase
        sineInput = fast_sine(progress * PI_HALF); // Approx. sine wave from 0 to PI/2 (0 to 1)
    }
    else
    {
        // Fall phase
        sineInput = fast_sine((1 - progress) * PI_HALF); // Approx. sine wave from PI/2 to 0 (1 to 0)
    }

    float    expInput   = fast_exp(sineInput * EXP_MULTIPLIER);
    uint32_t brightness = (uint32_t)((expInput - 1) / EXP_MULTIPLIER_FACTOR * maxDutyCycle);

    // Ensure that brightness does not exceed maxDutyCycle
    if (brightness > maxDutyCycle)
    {
        brightness = maxDutyCycle;
    }

    return brightness;
}
#endif

static uint32_t GetBreathBrightness(uint32_t timeInCycle, LED_Animation_Breath_t* Breath, uint32_t maxDutyCycle)
{
    // Calculate the total duration of one full breath cycle in milliseconds.
    uint32_t totalCycleTimeMs = Breath->riseTimeMs + Breath->fallTimeMs;

    // Get the duration of the rising phase of the breath cycle from the animation parameters.
    uint32_t riseTimeMs = Breath->riseTimeMs;

    uint32_t Brightness;

    // Determine the current phase of the cycle (rising or falling) and calculate brightness
    // accordingly.
    if (timeInCycle < riseTimeMs)
    {
        bool fadeDir = Breath->invert ? false : true;
        Brightness   = CalculateFadeBrightness(timeInCycle, riseTimeMs, maxDutyCycle, fadeDir);
    }
    else
    {
        bool fadeDir = Breath->invert ? true : false;
        Brightness   = CalculateFadeBrightness(timeInCycle - riseTimeMs, Breath->fallTimeMs, maxDutyCycle, fadeDir);
    }

    return Brightness;
}

static LED_Status_t LED_AnimationBreathExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    LED_Animation_Breath_t* Breath = (LED_Animation_Breath_t*)this->animationData;

    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = Breath->repeatTimes;
        this->controller->Start();
    }

    uint32_t elapsedTime      = tick - this->startTime;
    uint32_t totalCycleTimeMs = Breath->riseTimeMs + Breath->fallTimeMs;
    uint32_t timeInCycle      = elapsedTime % totalCycleTimeMs;

    uint32_t colorCount = CalculateColorCount(this->controller->LedType);
    uint16_t dutyCycleValues[colorCount];

    // Calculate the maximum duty cycle based on the maximum brightness level of  the LED.

    for (uint32_t i = 0; i < colorCount; i++)
    {
        uint32_t maxDutyCycle =
            BRIGHTNESS_TO_DUTY_CYCLE(((uint8_t*)(Breath->color))[i], this->controller->MaxDutyCycle);

        dutyCycleValues[i] = GetBreathBrightness(timeInCycle, Breath, maxDutyCycle);

#if PRINT_COORDINATES
        static uint32_t lastElapsedTime = 0;
        if (lastElapsedTime != timeInCycle && i == 0)
        {
            lastElapsedTime = timeInCycle;
            LED_CONTROL_DBG_MSG("%d; %d\r\n", timeInCycle, dutyCycleValues[i]);
        }
#endif
    }

    LED_Status_t result = ExecuteDutyCycleSetting(this, dutyCycleValues);
    if (result != LED_SUCCESS)
    {
        return result;
    }

    if (elapsedTime >= totalCycleTimeMs)
    {
        this->startTime       = tick;
        bool StopOnCompletion = Breath->invert ? false : true;
        HandleRepeatLogic(this, Breath->repeatTimes, StopOnCompletion);
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetFadeIn(LED_Handle_t* this, LED_Animation_FadeIn_t* FadeIn)
{
    if (this == NULL || FadeIn == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_FADE_IN;
    this->animationData = FadeIn;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation FadeIn\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationFadeInExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }
    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    LED_Animation_FadeIn_t* FadeIn = (LED_Animation_FadeIn_t*)this->animationData;

    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = FadeIn->repeatTimes;
        LED_CONTROL_DBG_MSG("Repeat times: %d\r\n", FadeIn->repeatTimes);
        this->controller->Start();
    }

    uint32_t elapsedTime = tick - this->startTime;

    uint32_t colorCount = CalculateColorCount(this->controller->LedType);
    uint16_t dutyCycleValues[colorCount];
    for (uint32_t i = 0; i < colorCount; i++)
    {
        // Calculate the maximum duty cycle based on the maximum brightness level of the LED.
        uint32_t maxDutyCycle =
            BRIGHTNESS_TO_DUTY_CYCLE(((uint8_t*)(FadeIn->color))[i], this->controller->MaxDutyCycle);
        uint32_t start_cycles, end_cycles, total_cycles;

        dutyCycleValues[i] = CalculateFadeBrightness(elapsedTime, FadeIn->durationMs, maxDutyCycle, true);

#if PRINT_COORDINATES
        static uint32_t lastElapsedTime = 0;
        if (lastElapsedTime != elapsedTime && i == 0)
        {
            lastElapsedTime = elapsedTime;
            LED_CONTROL_DBG_MSG("%d; %d\r\n", elapsedTime, dutyCycleValues[i]);
        }
#endif
    }

    LED_Status_t result = ExecuteDutyCycleSetting(this, dutyCycleValues);
    if (result != LED_SUCCESS)
    {
        return result;
    }

    if (elapsedTime >= FadeIn->durationMs)
    {
        HandleRepeatLogic(this, FadeIn->repeatTimes, false);
        this->startTime = tick;
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetFadeOut(LED_Handle_t* this, LED_Animation_FadeOut_t* FadeOut)
{
    if (this == NULL || FadeOut == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_FADE_OUT;
    this->animationData = FadeOut;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation FadeOut\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationFadeOutExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }
    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    LED_Animation_FadeIn_t* FadeOut = (LED_Animation_FadeOut_t*)this->animationData;

    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = FadeOut->repeatTimes;
        this->controller->Start();
    }

    uint32_t elapsedTime = tick - this->startTime;

    uint32_t colorCount = CalculateColorCount(this->controller->LedType);

    uint16_t dutyCycleValues[colorCount];
    for (uint32_t i = 0; i < colorCount; i++)
    {
        // Calculate the maximum duty cycle based on the maximum brightness level of the LED.
        uint32_t maxDutyCycle =
            BRIGHTNESS_TO_DUTY_CYCLE(((uint8_t*)(FadeOut->color))[i], this->controller->MaxDutyCycle);
        dutyCycleValues[i] = CalculateFadeBrightness(elapsedTime, FadeOut->durationMs, maxDutyCycle, false);

#if PRINT_COORDINATES
        static uint32_t lastElapsedTime = 0;
        if (lastElapsedTime != elapsedTime && i == 0)
        {
            lastElapsedTime = elapsedTime;
            LED_CONTROL_DBG_MSG("%d; %d\r\n", elapsedTime, dutyCycleValues[i]);
        }
#endif
    }

    LED_Status_t result = ExecuteDutyCycleSetting(this, dutyCycleValues);
    if (result != LED_SUCCESS)
    {
        return result;
    }

    if (elapsedTime >= FadeOut->durationMs)
    {
        HandleRepeatLogic(this, FadeOut->repeatTimes, true);
        this->startTime = tick;
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetPulse(LED_Handle_t* this, LED_Animation_Pulse_t* Pulse)
{
    if (this == NULL || Pulse == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_PULSE;
    this->animationData = Pulse;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Pulse\r\n");

    return LED_SUCCESS;
}

static uint32_t GetPulseBrightness(uint32_t timeInCycle, LED_Animation_Pulse_t* Pulse, uint32_t maxDutyCycle)
{
    uint32_t riseTimeMs    = Pulse->riseTimeMs;
    uint32_t holdOnTimeMs  = Pulse->holdOnTimeMs;
    uint32_t holdOffTimeMs = Pulse->holdOffTimeMs;
    uint32_t fallTimeMs    = Pulse->fallTimeMs;

    uint32_t Brightness;

    if (timeInCycle < riseTimeMs)
    {
        Brightness = CalculateFadeBrightness(timeInCycle, riseTimeMs, maxDutyCycle, true);
    }
    else if (timeInCycle < (riseTimeMs + holdOnTimeMs))
    {
        Brightness = maxDutyCycle;
    }
    else if (timeInCycle < (riseTimeMs + holdOnTimeMs + fallTimeMs))
    {
        Brightness = CalculateFadeBrightness(timeInCycle - riseTimeMs - holdOnTimeMs, fallTimeMs, maxDutyCycle, false);
    }
    else if (timeInCycle < (riseTimeMs + holdOnTimeMs + fallTimeMs + holdOffTimeMs))
    {
        Brightness = 0;
    }

    return Brightness;
}

static LED_Status_t LED_AnimationPulseExecute(LED_Handle_t* this, uint32_t tick)
{
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    LED_Animation_Pulse_t* Pulse = (LED_Animation_Pulse_t*)this->animationData;

    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = Pulse->repeatTimes;
        this->controller->Start();
    }

    uint32_t riseTimeMs    = Pulse->riseTimeMs;
    uint32_t holdOnTimeMs  = Pulse->holdOnTimeMs;
    uint32_t holdOffTimeMs = Pulse->holdOffTimeMs;
    uint32_t fallTimeMs    = Pulse->fallTimeMs;

    uint32_t elapsedTime      = tick - this->startTime;
    uint32_t totalCycleTimeMs = riseTimeMs + holdOnTimeMs + fallTimeMs + holdOffTimeMs;
    uint32_t timeInCycle      = elapsedTime % totalCycleTimeMs;

    uint32_t colorCount = CalculateColorCount(this->controller->LedType);
    uint16_t dutyCycleValues[colorCount];

    uint32_t start_cycles, end_cycles, total_cycles1, total_cycles2;
    for (uint32_t i = 0; i < colorCount; i++)
    {

        start_cycles          = DWT->CYCCNT;
        uint32_t maxDutyCycle = BRIGHTNESS_TO_DUTY_CYCLE(((uint8_t*)(Pulse->color))[i], this->controller->MaxDutyCycle);
        dutyCycleValues[i]    = GetPulseBrightness(timeInCycle, Pulse, maxDutyCycle);
        end_cycles            = DWT->CYCCNT;
        total_cycles1         = end_cycles - start_cycles;
        LED_CONTROL_DBG_MSG("C = %d\r\n", total_cycles1);

#if PRINT_COORDINATES
        static uint32_t lastElapsedTime = 0;
        if (lastElapsedTime != timeInCycle && i == 0)
        {
            lastElapsedTime = timeInCycle;
            LED_CONTROL_DBG_MSG("%d; %d\r\n", timeInCycle, dutyCycleValues[i]);
        }
#endif
    }

    LED_Status_t result = ExecuteDutyCycleSetting(this, dutyCycleValues);
    if (result != LED_SUCCESS)
    {
        return result;
    }

    // end_cycles = DWT->CYCCNT;

    // total_cycles1 = end_cycles - start_cycles;
    // LED_CONTROL_DBG_MSG("Cycles1 = %d\r\n", total_cycles1);

    // LED_CONTROL_DBG_MSG("Cycles2 = %d\r\n", total_cycles1 + total_cycles2);

    if (elapsedTime >= totalCycleTimeMs)
    {
        this->startTime = tick;
        HandleRepeatLogic(this, Pulse->repeatTimes, false);
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetAlternatingColors(LED_Handle_t* this,
                                                LED_Animation_AlternatingColors_t* AlternatingColors)
{
    if (this == NULL || AlternatingColors == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_ALTERNATING_COLORS;
    this->animationData = AlternatingColors;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Alternating Colors\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationAlternatingColorsExecute(LED_Handle_t* this, uint32_t tick)
{
    // Check for null pointers
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    // If the animation is not running, return success
    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    // Cast animation data to the appropriate type
    LED_Animation_AlternatingColors_t* AlternatingColors = (LED_Animation_AlternatingColors_t*)this->animationData;

    // Initialize the start time and start the controller if the animation is starting
    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = AlternatingColors->repeatTimes;
        this->controller->Start();
    }

    // Calculate the elapsed time since the animation started
    uint32_t elapsedTime = tick - this->startTime;

    // Calculate the total time for one full cycle of all colors
    uint32_t cycleTimeMs = AlternatingColors->durationMs * AlternatingColors->colorCount;

    // Calculate the number of full cycles completed and the time within the current cycle
    uint32_t timeInCurrentCycle = elapsedTime % cycleTimeMs;

    // Determine the current color index based on the elapsed time within the current cycle
    uint32_t colorIndex = timeInCurrentCycle / AlternatingColors->durationMs;

    // Get the current color
    uint8_t* currentColor = ((uint8_t**)AlternatingColors->colors)[colorIndex];

    // Set the LED color
    LED_Status_t result = ExecuteColorSetting(this, currentColor);
    if (result != LED_SUCCESS)
    {
        return result;
    }

    // Check if the animation should stop after the specified number of repeats
    if (AlternatingColors->repeatTimes != -1 && elapsedTime >= cycleTimeMs * AlternatingColors->repeatTimes)
    {
        this->isRunning = false;
        this->controller->Stop();
        if (this->callback != NULL)
        {
            this->callback(this->animationType, LED_SUCCESS);
        }
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetColorCycle(LED_Handle_t* this, LED_Animation_ColorCycle_t* ColorCycle)
{
    if (this == NULL || ColorCycle == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = LED_ANIMATION_COLOR_CYCLE;
    this->animationData = ColorCycle;
    this->isRunning     = false;
    LED_CONTROL_DBG_MSG("LED Animation Color Cycle\r\n");

    return LED_SUCCESS;
}

static LED_Status_t LED_AnimationColorCycleExecute(LED_Handle_t* this, uint32_t tick)
{
    // Validate input pointers
    if (this == NULL || this->animationData == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    // Return success if animation is not running
    if (!this->isRunning)
    {
        return LED_SUCCESS;
    }

    LED_Animation_ColorCycle_t* ColorCycle = (LED_Animation_ColorCycle_t*)this->animationData;

    // Initialize the animation
    if (this->startTime == 0)
    {
        this->startTime   = tick;
        this->repeatTimes = ColorCycle->repeatTimes;
        this->controller->Start();
    }

    uint32_t elapsedTime = tick - this->startTime;
    uint32_t cycleTimeMs = ColorCycle->transitionMs + ColorCycle->holdTimeMs;

    // Calculate the total cycle time
    uint32_t totalCycleTimeMs = (this->repeatTimes == -1 || this->repeatTimes > 1)
                                    ? cycleTimeMs * ColorCycle->colorCount
                                    : cycleTimeMs * ColorCycle->colorCount - ColorCycle->transitionMs;

    uint32_t timeInCycle    = elapsedTime % cycleTimeMs;
    uint32_t colorIndex     = (elapsedTime / cycleTimeMs) % ColorCycle->colorCount;
    uint32_t nextColorIndex = (colorIndex + 1) % ColorCycle->colorCount;

    uint8_t* currentColor = ((uint8_t**)ColorCycle->colors)[colorIndex];
    uint8_t* nextColor    = ((uint8_t**)ColorCycle->colors)[nextColorIndex];

    // Hold phase
    if (timeInCycle < ColorCycle->holdTimeMs)
    {
        LED_Status_t result = ExecuteColorSetting(this, currentColor);
        if (result != LED_SUCCESS)
        {
            return result;
        }
    }
    // Transition phase
    else
    {
        float   fraction             = (timeInCycle - ColorCycle->holdTimeMs) / (float)ColorCycle->transitionMs;
        uint8_t r                    = currentColor[0] + fraction * (nextColor[0] - currentColor[0]);
        uint8_t g                    = currentColor[1] + fraction * (nextColor[1] - currentColor[1]);
        uint8_t b                    = currentColor[2] + fraction * (nextColor[2] - currentColor[2]);
        uint8_t interpolatedColor[3] = {r, g, b};

        LED_Status_t result = ExecuteColorSetting(this, interpolatedColor);
        if (result != LED_SUCCESS)
        {
            return result;
        }
    }

    // Handle animation completion and repetition
    if (elapsedTime >= totalCycleTimeMs)
    {
        this->startTime       = tick;
        bool StopOnCompletion = ColorCycle->leaveLastColor ? true : false;
        HandleRepeatLogic(this, ColorCycle->repeatTimes, StopOnCompletion);
    }

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_SetAnimation(LED_Handle_t* this, void* animationData, LED_Animation_Type_t animationType)
{
    if (this == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    this->animationType = animationType;
    this->animationData = animationData;
    this->isRunning     = false;

    LED_CONTROL_DBG_MSG("LED Animation Set %d\r\n", animationType);

    return LED_SUCCESS;
}

LED_Status_t LED_Animation_GetColor(LED_Handle_t* this, uint8_t* color, uint8_t* colorCount)
{
    if (this == NULL || color == NULL || colorCount == NULL)
    {
        return LED_ERROR_NULL_POINTER;
    }

    *colorCount = CalculateColorCount(this->controller->LedType);

    for (uint32_t i = 0; i < *colorCount; i++)
    {
        color[i] = this->currentColor[i];
    }

    return LED_SUCCESS;
}

// How to Measure Machine Cycle Count
/**
#if MEASURE_CYCLES
start_cycles             = DWT->CYCCNT;
function_to_measure();
end_cycles               = DWT->CYCCNT;
// Calculate the total cycles used
total_cycles = end_cycles - start_cycles;
LED_CONTROL_DBG_MSG("Function consumed %lu cycles\r\n", total_cycles);
#else

**/