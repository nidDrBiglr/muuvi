#ifndef UTILS_H
#define UTILS_H

#include <zephyr/bluetooth/bluetooth.h>

/** @brief Converts binary LE Bluetooth address to string. in comparison to the zephyr variant, this
 * does not append the address tpye in brackets (because it's irrelevant)...
 *
 *  @param addr Address of buffer containing binary LE Bluetooth address.
 *  @param str Address of user buffer with enough room to store
 *  formatted string containing binary LE address.
 *  @param len Length of data to be copied to user string buffer. Refer to
 *  BT_ADDR_LE_STR_LEN about recommended value.
 *
 *  @return Number of successfully formatted bytes from binary address.
 */
static inline int bt_addr_le_to_str_without_type(const bt_addr_le_t *addr, char *str, size_t len)
{
	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X", addr->a.val[5], addr->a.val[4],
			addr->a.val[3], addr->a.val[2], addr->a.val[1], addr->a.val[0]);
}

#endif // UTILS_H