#include <bluetooth/uuid.h>

/**
 * UUID (v1):  77e1XXXX-37af-11ea-a7a1-507b9de20712
 */

#define BT_UUID_BASE	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e10000, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS
 *  @brief Cooker Service
 */
#define BT_UUID_COS						BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e11801, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_INPUT_IO
 *  @brief COS Characteristic Input IO State
 */
#define BT_UUID_COS_INPUT_IO			BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a01, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_INPUT_POWER
 *  @brief COS Characteristic Input Power Level
 */
#define BT_UUID_COS_INPUT_POWER			BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a02, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_OUTPUT_IO
 *  @brief COS Characteristic Output IO State
 */
#define BT_UUID_COS_OUTPUT_IO			BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a03, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_OUTPUT_POWER
 *  @brief COS Characteristic Output Power State
 */
#define BT_UUID_COS_OUTPUT_POWER		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a04, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_REMOTE_TEMP_PROBE
 *  @brief COS Characteristic remote temperature probe
 */
#define BT_UUID_COS_REMOTE_TEMP_PROBE	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a05, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_CONTROL_STATE
 *  @brief COS Characteristic automatic control state
 */
#define BT_UUID_COS_CONTROL_STATE		BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a06, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_CONTROL_SETPOINT
 *  @brief COS Characteristic automatic control setvalue
 */
#define BT_UUID_COS_CONTROL_SETPOINT	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a07, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))

/** @def BT_UUID_COS_CONTROL_PID
 *  @brief COS Characteristic automatic control PID values
 */
#define BT_UUID_COS_CONTROL_PID			BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x77e12a08, 0x37af, 0x11ea, 0xa7a1, 0x507b9de20712))
