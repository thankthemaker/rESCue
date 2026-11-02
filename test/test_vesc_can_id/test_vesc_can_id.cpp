/*
 * Host tests for the VESC CAN identifier encoding.
 *
 * Guards a regression that is invisible on the host otherwise: a receive id with
 * bit 31 set can never match an incoming frame, because the TWAI driver masks
 * extended identifiers to 29 bits - the CANBUS transport would then silently
 * stop decoding any data.
 *
 * Run with:  pio test -e native
 */
#include <unity.h>

#include "../../src/VescCanId.h"
#include "../../src/VescCanConstants.h"

void setUp(void) {}
void tearDown(void) {}

void test_receive_id_matches_the_29_bit_wire_format(void) {
    // (CAN_PACKET_STATUS << 8) | 25 => 0x0919
    TEST_ASSERT_EQUAL_HEX32(0x0919, canReceiveId(CAN_PACKET_STATUS, 25));
    // (CAN_PACKET_PROCESS_SHORT_BUFFER << 8) | 27 => 0x081B
    TEST_ASSERT_EQUAL_HEX32(0x081B, canReceiveId(CAN_PACKET_PROCESS_SHORT_BUFFER, 27));
}

void test_receive_id_must_not_set_bit_31(void) {
    TEST_ASSERT_EQUAL_HEX32(0, canReceiveId(CAN_PACKET_STATUS, 25) & 0x80000000U);
    TEST_ASSERT_EQUAL_HEX32(0, canReceiveId(CAN_PACKET_BMS_V_CELL, 1) & ~VESC_CAN_ID_MASK);
}

void test_send_and_receive_ids_agree_within_29_bits(void) {
    TEST_ASSERT_EQUAL_HEX32(canReceiveId(CAN_PACKET_STATUS, 25),
                            canSendId(CAN_PACKET_STATUS, 25) & VESC_CAN_ID_MASK);
    TEST_ASSERT_EQUAL_HEX32(canReceiveId(CAN_PACKET_PROCESS_SHORT_BUFFER, 27),
                            canSendId(CAN_PACKET_PROCESS_SHORT_BUFFER, 27) & VESC_CAN_ID_MASK);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_receive_id_matches_the_29_bit_wire_format);
    RUN_TEST(test_receive_id_must_not_set_bit_31);
    RUN_TEST(test_send_and_receive_ids_agree_within_29_bits);
    return UNITY_END();
}
