#ifndef __LED_ANIMATION_H__
#define __LED_ANIMATION_H__

#include <stdbool.h>
#include <stdint.h>

// Only one of the following should be set to 1
// 81 ~ 181 cycles // led_pwm.c library
// 90 ~ 95 cycles // Quadratic
// 7300 ~ 8000 cycles // Exponential
// 9000 ~ 11000 cycles // Sine
// 500 ~ 560 cycles // Sine Approx
#define USE_LED_ANIMATION_QUADRATIC (1)
#define USE_LED_ANIMATION_EXPONENTIAL (0)
#define USE_LED_ANIMATION_SINE (0)
#define USE_LED_ANIMATION_SINE_APPROX (0)

#define LED_MAX_BRIGHTNESS (255) // Standardized 8-bit brightness level

/** @brief Status codes for LED operations. */
typedef enum
{
    LED_SUCCESS = 0,
    LED_ERROR_INVALID_COLOR,
    LED_ERROR_INVALID_BRIGHTNESS,
    LED_ERROR_PATTERN_NOT_SUPPORTED,
    LED_ERROR_NULL_POINTER,
    LED_ERROR_INVALID_ANIMATION_TYPE,
    LED_ERROR_INVALID_LED_TYPE,
    LED_ERROR_INVALID_ARGUMENT,
    LED_ERROR_INVALID_VALUE,
    LED_ERROR_INVALID_LED_POLARITY,
    LED_ANIMATION_COMPLETED,
} LED_Status_t;

/** @brief Structure to represent a PWM channel. */
typedef struct
{
    void (*setDutyCycle)(uint16_t DutyCycle); /**< Function to set the duty cycle. */
} PWM_Channel_t;

/** @brief Types of LED configurations. */
typedef enum
{
    LED_TYPE_INVALID = 0,
    LED_TYPE_RGB,
    LED_TYPE_RGY,
    LED_TYPE_RGBW,
    LED_TYPE_SINGLE_COLOR,
    LED_TYPE_DUAL_COLOR,
    LED_TYPE_LAST
} LED_Type_t;

#define IS_VALID_LED_TYPE(type) ((type > LED_TYPE_INVALID) && (type < LED_TYPE_LAST))

#define MAX_COLOR_CHANNELS (4) // Maximum number of color channels

/** @brief RGB LED configuration. */
typedef struct
{
    PWM_Channel_t Red;
    PWM_Channel_t Green;
    PWM_Channel_t Blue;
} PWM_RGB_t;

/** @brief RGY LED configuration. */
typedef struct
{
    PWM_Channel_t Red;
    PWM_Channel_t Green;
    PWM_Channel_t Yellow;
} PWM_RGY_t;

/** @brief RGBW LED configuration. */
typedef struct
{
    PWM_Channel_t Red;
    PWM_Channel_t Green;
    PWM_Channel_t Blue;
    PWM_Channel_t White;
} PWM_RGBW_t;

/** @brief Single color LED configuration. */
typedef struct
{
    PWM_Channel_t led;
} PWM_Single_t;

/** @brief Dual color LED configuration. */
typedef struct
{
    PWM_Channel_t led1;
    PWM_Channel_t led2;
} PWM_Dual_t;

// Macro to convert brightness (0-255) to duty cycle
#define BRIGHTNESS_TO_DUTY_CYCLE(brightness, MAX_DUTY_CYCLE) ((brightness * MAX_DUTY_CYCLE) / 255)

// Macro to convert duty cycle to brightness (0-255)
#define DUTY_CYCLE_TO_BRIGHTNESS(dutyCycle, MAX_DUTY_CYCLE) ((uint8_t)(((dutyCycle)*255) / (MAX_DUTY_CYCLE)))

/** @brief Main LED controller structure. */
typedef struct
{
    void (*Start)(void);     /**< Function to start the PWM. */
    void (*Stop)(void);      /**< Function to stop the PWM. */
    void*      PwmConfig;    /**< Pointer to the LED configuration. */
    LED_Type_t LedType;      /**< Type of the LED configuration. */
    uint16_t   MaxDutyCycle; /**< Maximum duty cycle value. */

} LED_Controller_t;

/** @brief Types of LED animations. */
typedef enum
{
    LED_ANIMATION_INVALID = 0,
    LED_ANIMATION_NONE,
    LED_ANIMATION_OFF,
    LED_ANIMATION_SOLID,
    LED_ANIMATION_BLINK,
    LED_ANIMATION_FLASH,
    LED_ANIMATION_BREATH,
    LED_ANIMATION_PULSE,
    LED_ANIMATION_FADE_IN,
    LED_ANIMATION_FADE_OUT,
    LED_ANIMATION_ALTERNATING_COLORS, // Abruptly switches between colors based
                                      // on durationMs.
    LED_ANIMATION_COLOR_CYCLE,        // Smoothly transitions between colors over
                                      // transitionMs.
    LED_ANIMATION_LAST

} LED_Animation_Type_t;

#define IS_VALID_LED_ANIMATION_TYPE(type) ((type > LED_ANIMATION_INVALID) && (type < LED_ANIMATION_LAST))

/** @brief RGB color structure. */
typedef struct
{
    uint8_t R; // Red brightness level (0-255)
    uint8_t G; // Green brightness level (0-255)
    uint8_t B; // Blue brightness level (0-255)
} RGB_Color_t;

/** @brief RGY color structure. */
typedef struct
{
    uint8_t R; // Red brightness level (0-255)
    uint8_t G; // Green brightness level (0-255)
    uint8_t Y; // Yellow brightness level (0-255)
} RGY_Color_t;

/** @brief RGBW color structure. */
typedef struct
{
    uint8_t R; // Red brightness level (0-255)
    uint8_t G; // Green brightness level (0-255)
    uint8_t B; // Blue brightness level (0-255)
    uint8_t W; // White brightness level (0-255)
} RGBW_Color_t;

/** @brief Single color structure. */
typedef struct
{
    uint8_t Brightness; // Brightness level (0-255)
} SingleColor_t;

/** @brief Dual color structure. */
typedef struct
{
    uint8_t Color1; // Color1 brightness level (0-255)
    uint8_t Color2; // Color2 brightness level (0-255)
} DualColor_t;

/** @brief Callback type for indicating pattern completion. */
typedef void (*LED_Animation_Complete_Callback)(LED_Animation_Type_t animationType, LED_Status_t status);

/** @brief Structure to represent an LED handle. */
typedef struct
{
    LED_Controller_t*               controller;    /**< Pointer to the LED controller. */
    LED_Animation_Type_t            animationType; /**< Type of the LED animation. */
    bool                            isRunning;     /**< Flag to indicate if the animation is running. */
    void*                           animationData; /**< Pointer to the animation data. */
    uint32_t                        startTime;     /**< Start time of the animation. */
    uint32_t                        lastTick;      /**< Last tick value. */
    LED_Animation_Complete_Callback callback;      /**< pattern completion. */
    int8_t                          repeatTimes;   /**< Number of times to repeat the animation. */
    uint8_t                         currentColor[MAX_COLOR_CHANNELS]; /**< Current color of the LED. */
} LED_Handle_t;

//////////////// LED ANIMATION STRUCTURES //////////////////////

/** @brief Structure for solid animation configuration. */
typedef struct
{
    void*    color;           // Color configuration for the solid animation
    uint32_t executionTimeMs; // Duration for which the solid color is displayed
                              // in milliseconds, (0 for infinite)
} LED_Animation_Solid_t;

/** @brief Structure for blink animation configuration. */
typedef struct
{
    void*    color;       // Color to blink
    uint16_t periodMs;    // Time for one complete on/off cycle in milliseconds
    int8_t   repeatTimes; // Number of times to repeat the blink (-1 for infinite)
} LED_Animation_Blink_t;

/** @brief Structure for flash animation configuration. */
typedef struct
{
    void*    color;       // Color to flash
    uint16_t onTimeMs;    // Time for which the LED is on in milliseconds
    uint16_t offTimeMs;   // Time for which the LED is off in milliseconds
    int8_t   repeatTimes; // Number of times to repeat the flash (-1 for infinite)
} LED_Animation_Flash_t;

/** @brief Structure for breath animation configuration. */
typedef struct
{
    void*    color;       // Color to use for the breathing effect
    uint16_t riseTimeMs;  // Time for the intensity to increase from min to max
    uint16_t fallTimeMs;  // Time for the intensity to decrease from max to min
    int8_t   repeatTimes; // Number of times to repeat (-1 for infinite)
    bool     invert;      // Flag to invert the breathing effect (start high and end high if true)
} LED_Animation_Breath_t;

/** @brief Structure for fade-in animation configuration. */
typedef struct
{
    void*    color;       // Color to use for the fade effect
    uint16_t durationMs;  // Time for the intensity to increase from min to max
    int8_t   repeatTimes; // Number of times to repeat (-1 for infinite)
} LED_Animation_FadeIn_t;

/** @brief Structure for fade-out animation configuration. */
typedef struct
{
    void*    color;       // Color to use for the fade effect
    uint16_t durationMs;  // Time for the intensity to decrease from max to min
    int8_t   repeatTimes; // Number of times to repeat (-1 for infinite)
} LED_Animation_FadeOut_t;

/** @brief Structure for pulse animation configuration. */
typedef struct
{
    void*    color;         // Color to use for the pulsing effect
    uint16_t riseTimeMs;    // Time for the intensity to increase from min to max
    uint16_t holdOnTimeMs;  // Time for the intensity to stay at max
    uint16_t holdOffTimeMs; // Time for the intensity to stay at min
    uint16_t fallTimeMs;    // Time for the intensity to decrease from max to min
    int8_t   repeatTimes;   // Number of times to repeat (-1 for infinite)
} LED_Animation_Pulse_t;

/** @brief Structure for alternating colors animation configuration. */
typedef struct
{
    void*    colors;      // Array of colors to alternate between
    uint16_t colorCount;  // Number of colors in the array
    uint16_t durationMs;  // Duration for each color in milliseconds
    int8_t   repeatTimes; // Number of times to repeat the color alternation (-1
                          // for infinite)
} LED_Animation_AlternatingColors_t;

/** @brief Structure for color cycle animation configuration. */
typedef struct
{
    uint8_t* colors;       // Array of colors to cycle through
    uint16_t colorCount;   // Number of colors in the array
    uint16_t transitionMs; // Duration of the transition between each color in
                           // milliseconds
    uint16_t holdTimeMs;   // Time to hold each color in milliseconds
    int8_t   repeatTimes;  // Number of times to repeat the color cycle (-1 for
                           // infinite)
    bool leaveLastColor;   // Flag to leave the last color on after the animation
                           // completes
} LED_Animation_ColorCycle_t;

/** @brief Initialize the LED animation handle.
 *
 *  @param this Pointer to the LED handle.
 *  @param Controller Pointer to the LED controller.
 *  @param callback Callback function for animation completion.
 *  @return LED status.
 */
LED_Status_t
LED_Animation_Init(LED_Handle_t* this, LED_Controller_t* Controller, LED_Animation_Complete_Callback callback);

/** @brief Set the LED animation.
 *
 *  @param this Pointer to the LED handle.
 *  @param animationType Type of the LED animation.
 *  @param animationData Pointer to the animation configuration.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetAnimation(LED_Handle_t* this, void* animationData, LED_Animation_Type_t animationType);

/** @brief Set the solid animation configuration.
 *
 *  @param this Pointer to the LED handle.
 *  @param Solid Pointer to the solid animation configuration.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetSolid(LED_Handle_t* this, LED_Animation_Solid_t* Solid);

/** @brief Set the flash animation configuration.
 *
 *  @param this Pointer to the LED handle.
 *  @param Flash Pointer to the flash animation configuration.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetFlash(LED_Handle_t* this, LED_Animation_Flash_t* Flash);

/** @brief Set the blink animation configuration.
 *
 *  @param this Pointer to the LED handle.
 *  @param Blink Pointer to the blink animation configuration.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetBlink(LED_Handle_t* this, LED_Animation_Blink_t* Blink);

/** @brief Set the breath animation configuration.
 *
 *  @param this Pointer to the LED handle.
 *  @param Breath Pointer to the breath animation configuration.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetBreath(LED_Handle_t* this, LED_Animation_Breath_t* Breath);

/** @brief Set the fade-in animation configuration.
 *
 * @param this Pointer to the LED handle.
 * @param FadeIn Pointer to the fade-in animation configuration.
 * @return LED status.
 */
LED_Status_t LED_Animation_SetFadeIn(LED_Handle_t* this, LED_Animation_FadeIn_t* FadeIn);

/** @brief Set the fade-out animation configuration.
 *
 * @param this Pointer to the LED handle.
 * @param FadeOut Pointer to the fade-out animation configuration.
 * @return LED status.
 */
LED_Status_t LED_Animation_SetFadeOut(LED_Handle_t* this, LED_Animation_FadeOut_t* FadeOut);

/** @brief Set the pulse animation configuration.
 *
 * @param this Pointer to the LED handle.
 * @param Pulse Pointer to the pulse animation configuration.
 * @return LED status.
 */
LED_Status_t LED_Animation_SetPulse(LED_Handle_t* this, LED_Animation_Pulse_t* Pulse);

/** @brief Set the alternating colors animation configuration.
 *
 * @param this Pointer to the LED handle.
 * @param AlternatingColors Pointer to the alternating colors animation
 * configuration.
 * @return LED status.
 */
LED_Status_t LED_Animation_SetAlternatingColors(LED_Handle_t* this,
                                                LED_Animation_AlternatingColors_t* AlternatingColors);

/** @brief Set the color cycle animation configuration.
 *
 * @param this Pointer to the LED handle.
 * @param ColorCycle Pointer to the color cycle animation configuration.
 * @return LED status.
 */
LED_Status_t LED_Animation_SetColorCycle(LED_Handle_t* this, LED_Animation_ColorCycle_t* ColorCycle);

/** @brief Update the LED animation.
 *
 *  @param this Pointer to the LED handle.
 *  @param tick Current tick value, in milliseconds.
 *  @return LED status.
 */
LED_Status_t LED_Animation_Update(LED_Handle_t* this, uint32_t tick);

/** @brief Stop the LED animation.
 *
 *  @param this Pointer to the LED handle.
 *  @return LED status.
 */
LED_Status_t LED_Animation_Stop(LED_Handle_t* this);

/** @brief Start the LED animation.
 *
 *  @param this Pointer to the LED handle.
 *  @return LED status.
 */
LED_Status_t LED_Animation_Start(LED_Handle_t* this);

/** @brief Set the LED off.
 *
 *  @param this Pointer to the LED handle.
 *  @return LED status.
 */
LED_Status_t LED_Animation_SetOff(LED_Handle_t* this);

/** @brief Get the color of the LED.
 *
 *  @param this Pointer to the LED handle.
 *  @param color Pointer to the color structure to store the LED color.
 * @param colorCount Pointer to store the number of colors in the color structure.
 *
 *  @return LED status.
 */
LED_Status_t LED_Animation_GetColor(LED_Handle_t* this, uint8_t* color, uint8_t* colorCount);

/** @brief Set the color of the LED.
 *
 *  @param this Pointer to the LED handle.
 *  @param color Pointer to the color structure to set the LED color.
 * @param colorCount Number of colors in the color structure.
 *
 *  @return LED status.
 */
uint32_t CalculateColorCount(LED_Type_t ledType);

#endif // __LED_ANIMATION_H__