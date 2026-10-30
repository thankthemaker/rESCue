/*
 * Host tests for the VESC packet framing / CRC used by the CAN<->BLE proxy.
 *
 * The reported symptom these guard: writing the motor or app configuration via
 * the proxy failed because the VESC answers such writes with a one byte payload
 * ("COMM_SET_MCCONF"), which travels in a CAN short buffer frame. That frame
 * carries no CRC and its payload starts at index 2 - the proxy used to take the
 * wrong slice and to read the CRC out of the frame instead of deriving it.
 *
 * Run with:  pio test -e native
 */
#include <unity.h>
#include <cstring>

#include "../../lib/vesc-protocol/src/vesc_packet.h"

void setUp(void) {}
void tearDown(void) {}

static const uint8_t COMM_GET_VALUES = 0x04;
static const uint8_t COMM_SET_MCCONF = 0x65;
static const uint8_t COMM_FLOAT_PACKAGE = 0x24;

void test_crc16_matches_the_vesc_implementation(void) {
    const uint8_t digits[] = "123456789";
    TEST_ASSERT_EQUAL_HEX16(0x31C3, crc16(digits, 9));

    const uint8_t one[] = {COMM_SET_MCCONF};
    TEST_ASSERT_EQUAL_HEX16(0x3C03, crc16(one, 1));

    // cross check against the CRC hard coded in Uart::sendFloatPackageDataRequest
    const uint8_t floatCmd[] = {COMM_FLOAT_PACKAGE, 0x65, 0x01};
    TEST_ASSERT_EQUAL_HEX16(0xBEF8, crc16(floatCmd, 3));
}

/*
 * The exact reply the VESC sends for COMM_SET_MCCONF: the payload is only the
 * command byte, so it arrives as a short buffer frame without any CRC.
 * Frame layout (comm_can.c): [controller id, send flag, payload...]
 */
void test_short_buffer_frame_of_a_set_mcconf_ack(void) {
    const uint8_t ackFrame[3] = {25 /* VESC controller id */, 0x01 /* send */, COMM_SET_MCCONF};

    const uint8_t *payload = nullptr;
    const int payloadLen = vesc_short_buffer_payload(ackFrame, sizeof(ackFrame), &payload);

    TEST_ASSERT_EQUAL_INT(1, payloadLen);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL_HEX8(COMM_SET_MCCONF, payload[0]);
}

/* What the app must receive for that ack: a well formed VESC packet. */
void test_set_mcconf_ack_is_forwarded_as_a_valid_packet(void) {
    const uint8_t ackFrame[3] = {25, 0x01, COMM_SET_MCCONF};

    const uint8_t *payload = nullptr;
    const int payloadLen = vesc_short_buffer_payload(ackFrame, sizeof(ackFrame), &payload);
    TEST_ASSERT_EQUAL_INT(1, payloadLen);

    uint8_t packet[16] = {0};
    const int written = vesc_packet_encode(payload, (unsigned int) payloadLen, packet, sizeof(packet));
    TEST_ASSERT_EQUAL_INT(6, written);

    // [start][len][payload][crc_hi][crc_lo][end]
    TEST_ASSERT_EQUAL_HEX8(0x02, packet[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, packet[1]);
    TEST_ASSERT_EQUAL_HEX8(COMM_SET_MCCONF, packet[2]);
    const unsigned short crc = crc16(payload, (unsigned int) payloadLen);
    TEST_ASSERT_EQUAL_HEX8((uint8_t) (crc >> 8), packet[3]);
    TEST_ASSERT_EQUAL_HEX8((uint8_t) (crc & 0xFF), packet[4]);
    TEST_ASSERT_EQUAL_HEX8(0x03, packet[5]);
}

void test_short_buffer_frame_payload_starts_at_index_two(void) {
    const uint8_t frame[8] = {25, 0x01, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    const uint8_t *payload = nullptr;
    const int payloadLen = vesc_short_buffer_payload(frame, 8, &payload);

    TEST_ASSERT_EQUAL_INT(6, payloadLen);
    TEST_ASSERT_EQUAL_HEX8(0xAA, payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, payload[5]);

    // frames too short to hold a payload are rejected instead of read out of bounds
    TEST_ASSERT_EQUAL_INT(-1, vesc_short_buffer_payload(frame, 1, &payload));
    TEST_ASSERT_EQUAL_INT(0, vesc_short_buffer_payload(frame, 2, &payload));
}

void test_packet_header_length_follows_the_payload_size(void) {
    TEST_ASSERT_EQUAL_INT(2, vesc_packet_header_length(255));
    TEST_ASSERT_EQUAL_INT(3, vesc_packet_header_length(256));
    TEST_ASSERT_EQUAL_INT(3, vesc_packet_header_length(65535));
    TEST_ASSERT_EQUAL_INT(4, vesc_packet_header_length(65536));

    TEST_ASSERT_EQUAL_UINT(255 + 2 + 3, vesc_packet_frame_length(255));
    TEST_ASSERT_EQUAL_UINT(300 + 3 + 3, vesc_packet_frame_length(300));
}

void test_encode_frames_a_large_payload_with_a_two_byte_length(void) {
    uint8_t payload[300];
    for (unsigned int i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t) i;
    }

    uint8_t packet[512] = {0};
    const int written = vesc_packet_encode(payload, sizeof(payload), packet, sizeof(packet));

    TEST_ASSERT_EQUAL_INT((int) vesc_packet_frame_length(sizeof(payload)), written);
    TEST_ASSERT_EQUAL_HEX8(0x03, packet[0]);
    TEST_ASSERT_EQUAL_HEX8(300 >> 8, packet[1]);
    TEST_ASSERT_EQUAL_HEX8(300 & 0xFF, packet[2]);
    TEST_ASSERT_EQUAL_HEX8(payload[0], packet[3]);
    TEST_ASSERT_EQUAL_HEX8(payload[299], packet[3 + 299]);

    const unsigned short crc = crc16(payload, sizeof(payload));
    TEST_ASSERT_EQUAL_HEX8((uint8_t) (crc >> 8), packet[written - 3]);
    TEST_ASSERT_EQUAL_HEX8((uint8_t) (crc & 0xFF), packet[written - 2]);
    TEST_ASSERT_EQUAL_HEX8(0x03, packet[written - 1]);
}

void test_encode_rejects_what_does_not_fit(void) {
    const uint8_t payload[4] = {1, 2, 3, 4};
    uint8_t small[4] = {0};

    TEST_ASSERT_EQUAL_INT(-1, vesc_packet_encode(payload, sizeof(payload), small, sizeof(small)));
    TEST_ASSERT_EQUAL_INT(-1, vesc_packet_encode(nullptr, 4, small, sizeof(small)));
    TEST_ASSERT_EQUAL_INT(-1, vesc_packet_encode(payload, 0, small, sizeof(small)));
}

/* Build a real VESC packet so the finder is tested against the encoder. */
static unsigned int buildPacket(uint8_t *out, uint8_t command, const uint8_t *args, unsigned int argLen) {
    uint8_t payload[512];
    payload[0] = command;
    if (argLen > 0) {
        memcpy(payload + 1, args, argLen);
    }
    return (unsigned int) vesc_packet_encode(payload, 1 + argLen, out, 512);
}

void test_finder_returns_a_complete_packet(void) {
    uint8_t buf[64];
    const unsigned int n = buildPacket(buf, COMM_GET_VALUES, nullptr, 0);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_TRUE(vesc_packet_next(buf, n, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(0, offset);
    TEST_ASSERT_EQUAL_UINT(n, view.packetLength);
    TEST_ASSERT_EQUAL_UINT(2, view.headerLength);
    TEST_ASSERT_EQUAL_UINT(1, view.payloadLength);
    TEST_ASSERT_EQUAL_HEX8(COMM_GET_VALUES, buf[offset + view.headerLength]);
}

void test_finder_waits_for_an_incomplete_packet(void) {
    uint8_t buf[64];
    const unsigned int n = buildPacket(buf, COMM_GET_VALUES, nullptr, 0);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_FALSE(vesc_packet_next(buf, n - 1, &offset, &view));
    // everything stays buffered, so the packet can complete on the next chunk
    TEST_ASSERT_EQUAL_UINT(0, offset);
}

void test_finder_skips_leading_garbage(void) {
    uint8_t buf[64];
    const unsigned int n = buildPacket(buf, COMM_SET_MCCONF, nullptr, 0);

    uint8_t stream[80] = {0xFF, 0x00, 0x99};
    memcpy(stream + 3, buf, n);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_TRUE(vesc_packet_next(stream, 3 + n, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(3, offset);
    TEST_ASSERT_EQUAL_HEX8(COMM_SET_MCCONF, stream[offset + view.headerLength]);
}

void test_finder_resynchronises_on_a_missing_end_byte(void) {
    // a well formed looking header with a wrong end byte, followed by a real packet
    const uint8_t bogus[6] = {0x02, 0x01, 0x41, 0x00, 0x00, 0x99};
    uint8_t buf[64];
    const unsigned int n = buildPacket(buf, COMM_GET_VALUES, nullptr, 0);

    uint8_t stream[80];
    memcpy(stream, bogus, sizeof(bogus));
    memcpy(stream + sizeof(bogus), buf, n);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_TRUE(vesc_packet_next(stream, sizeof(bogus) + n, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(sizeof(bogus), offset);
    TEST_ASSERT_EQUAL_HEX8(COMM_GET_VALUES, stream[offset + view.headerLength]);
}

/* Several packets in one chunk: the client may batch its writes. */
void test_finder_returns_consecutive_packets(void) {
    uint8_t buf[128];
    const unsigned int n1 = buildPacket(buf, COMM_GET_VALUES, nullptr, 0);
    const unsigned int n2 = buildPacket(buf + n1, COMM_SET_MCCONF, nullptr, 0);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_TRUE(vesc_packet_next(buf, n1 + n2, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(0, offset);
    TEST_ASSERT_EQUAL_HEX8(COMM_GET_VALUES, buf[offset + view.headerLength]);

    offset += view.packetLength;
    TEST_ASSERT_TRUE(vesc_packet_next(buf, n1 + n2, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(n1, offset);
    TEST_ASSERT_EQUAL_HEX8(COMM_SET_MCCONF, buf[offset + view.headerLength]);

    offset += view.packetLength;
    TEST_ASSERT_FALSE(vesc_packet_next(buf, n1 + n2, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(n1 + n2, offset);
}

void test_finder_handles_a_two_byte_length_header(void) {
    uint8_t args[300];
    memset(args, 0x11, sizeof(args));
    uint8_t buf[512];
    const unsigned int n = buildPacket(buf, COMM_SET_MCCONF, args, sizeof(args));

    TEST_ASSERT_EQUAL_HEX8(0x03, buf[0]);

    unsigned int offset = 0;
    VescPacketView view;
    TEST_ASSERT_TRUE(vesc_packet_next(buf, n, &offset, &view));
    TEST_ASSERT_EQUAL_UINT(3, view.headerLength);
    TEST_ASSERT_EQUAL_UINT(1 + sizeof(args), view.payloadLength);
    TEST_ASSERT_EQUAL_UINT(n, view.packetLength);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_crc16_matches_the_vesc_implementation);
    RUN_TEST(test_short_buffer_frame_of_a_set_mcconf_ack);
    RUN_TEST(test_set_mcconf_ack_is_forwarded_as_a_valid_packet);
    RUN_TEST(test_short_buffer_frame_payload_starts_at_index_two);
    RUN_TEST(test_packet_header_length_follows_the_payload_size);
    RUN_TEST(test_encode_frames_a_large_payload_with_a_two_byte_length);
    RUN_TEST(test_encode_rejects_what_does_not_fit);
    RUN_TEST(test_finder_returns_a_complete_packet);
    RUN_TEST(test_finder_waits_for_an_incomplete_packet);
    RUN_TEST(test_finder_skips_leading_garbage);
    RUN_TEST(test_finder_resynchronises_on_a_missing_end_byte);
    RUN_TEST(test_finder_returns_consecutive_packets);
    RUN_TEST(test_finder_handles_a_two_byte_length_header);
    return UNITY_END();
}
