/**
 * @file led_transition_manager.h
 * @brief Header file for LED transition management library.
 *
 * This library handles transitions between different LED animations.
 *
 * @copyright Copyright (c) 2024
 */

#ifndef INC_LED_TRANSITION_MANAGER_H
#define INC_LED_TRANSITION_MANAGER_H

#include "led_animation.h"

// Default transition times
#define DEFAULT_TRANSITION_CLEAN_ENTRY_TIMEOUT_MS 2000
#define DEFAULT_TRANSITION_UPON_COMPLETION_TIMEOUT_MS 5000
#define DEFAULT_TRANSITION_INTERPOLATE_TIME_MS 200

/**
 * @brief Enum for different types of LED transitions.
 */
typedef enum
{
    LED_TRANSITION_INVALID = 0,     /**< Invalid transition type */
    LED_TRANSITION_IMMINENT,        /**< Immediately switch to the target animation */
    LED_TRANSITION_INTERPOLATE,     /**< Smoothly interpolate between animations */
    LED_TRANSITION_UPON_COMPLETION, /**< Wait for the current animation to complete before transitioning */
    LED_TRANSITION_AT_CLEAN_ENTRY,  /**< Wait for a clean entry point in the current animation before transitioning */
    LED_TRANSITION_LAST             /**< Placeholder for the last transition type */
} LED_Transition_Type_t;

#define IS_VALID_TRANSITION_TYPE(type) ((type) > LED_TRANSITION_INVALID && (type) < LED_TRANSITION_LAST)

/**
 * @brief Structure for configuring an LED transition.
 * @note: Duration of the transition, meaning varies with TransitionType
 * - For LED_TRANSITION_IMMINENT: This field is ignored
 * - For LED_TRANSITION_INTERPOLATE: Time in milliseconds to interpolate between animations
 * - For LED_TRANSITION_UPON_COMPLETION: Maximum wait time in milliseconds before force switching
 * - For LED_TRANSITION_AT_CLEAN_ENTRY: Maximum wait time in milliseconds before force switching
 */
typedef struct
{
    const void*           StartAnim;      /**< Pointer to the starting animation configuration */
    const void*           EndAnim;        /**< Pointer to the target animation configuration */
    LED_Transition_Type_t TransitionType; /**< Type of transition */
    uint16_t              Duration;       /**< Duration of the transition, meaning varies with TransitionType */
} LED_Transition_Config_t;

typedef enum
{
    LED_TRANSITION_STATE_INVALID = 0, /**< State indicating an invalid or uninitialized transition. */
    LED_TRANSITION_STATE_IDLE,        /**< State indicating the transition is idle and not active. */
    LED_TRANSITION_STATE_SETUP,       /**< State indicating the transition is being set up. */
    LED_TRANSITION_STATE_ONGOING,     /**< State indicating the transition is currently in progress. */
    LED_TRANSITION_STATE_COMPLETED,   /**< State indicating the transition has been completed. */
    LED_TRANSITION_STATE_LAST,        /**< Marker for the last valid state; used for validation purposes. */
} LED_Transition_State_t;

#define IS_VALID_TRANSITION_STATE(state) ((state) > LED_TRANSITION_STATE_INVALID && (state) < LED_TRANSITION_STATE_LAST)

typedef struct
{
    const void*            transitionMap;                    /**< Pointer to the transition map. */
    LED_Handle_t*          LedHandle;                        /**< Pointer to the LED handle. */
    const void*            targetAnimData;                   /**< Pointer to the target animation data. */
    uint32_t               lastTick;                         /**< Last tick value. */
    uint16_t               Duration;                         /**< Duration of the transition. */
    uint8_t                mapSize;                          /**< Size of the transition map. */
    LED_Animation_Type_t   targetAnimType;                   /**< Type of the target animation. */
    LED_Transition_Type_t  transitionType;                   /**< Type of the transition. */
    LED_Transition_State_t state;                            /**< State of the transition. */
    uint8_t                currentColor[MAX_COLOR_CHANNELS]; /**< Current color of the LED. */
    uint8_t                targetColor[MAX_COLOR_CHANNELS];  /**< Target color of the LED. */
} LED_Transition_Handle_t;

LED_Status_t LED_Transition_Init(LED_Transition_Handle_t* this, LED_Handle_t* LedHandle);
LED_Status_t LED_Transition_SetMapping(LED_Transition_Handle_t* this, const void* transitionsMap, uint32_t mapSize);
LED_Status_t LED_Transition_Update(LED_Transition_Handle_t* this, uint32_t tick);
LED_Status_t LED_Transition_ExecAnimation(LED_Transition_Handle_t* this, const void* animConfig,
                                          LED_Animation_Type_t animType);

#endif /* INC_LED_TRANSITION_MANAGER_H */
