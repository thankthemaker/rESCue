/*
 * Host tests for the transport independent VESC command parser.
 *
 * The parser is a pure function of (bytes, length, batteryDrift, now), so it can
 * be exercised on the host without any Arduino/ESP dependency. The implementation
 * is pulled in directly because the project "src" directory is not compiled for
 * the native environment (the other sources depend on the Arduino framework).
 *
 * Run with:  pio test -e native
 */
#include <unity.h>

#include "../../src/VescCommandParser.cpp"

void setUp(void) {}
void tearDown(void) {}

/*
 * Build a COMM_GET_VALUES_SELECTIVE (0x32) response and verify every field.
 * The offsets match the parser (which reads relative to an offset of 1).
 */
void test_get_values_selective_decodes_all_fields(void) {
    uint8_t values[26] = {0};
    values[0] = 0x32;
    values[5] = 0x00; values[6] = 0xFA;                                          // mosfetTemp = 250 -> 25.0
    values[7] = 0x01; values[8] = 0x2C;                                          // motorTemp  = 300 -> 30.0
    values[9] = 0x01; values[10] = 0xF4;                                         // dutyCycle  = 500 -> 0.5
    values[11] = 0x00; values[12] = 0x00; values[13] = 0x30; values[14] = 0x39;  // erpm = 12345
    values[15] = 0x01; values[16] = 0xF4;                                        // inputVoltage = 500 -> 50.0
    values[17] = 0x00; values[18] = 0x00; values[19] = 0x03; values[20] = 0xE8;  // tachometer = 1000
    values[21] = 0x00; values[22] = 0x00; values[23] = 0x07; values[24] = 0xD0;  // tachometerAbsolut = 2000
    values[25] = 0x00;                                                           // fault

    VescCommandParser parser;
    VescData data;
    std::string type = parser.parseCommand(values, sizeof(values), &data, 0.0, 4711);

    TEST_ASSERT_EQUAL_STRING("COMM_GET_VALUES_SELECTIVE", type.c_str());
    TEST_ASSERT_EQUAL_DOUBLE(25.0, data.mosfetTemp);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, data.motorTemp);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, data.dutyCycle);
    TEST_ASSERT_EQUAL_DOUBLE(12345, data.erpm);
    TEST_ASSERT_EQUAL_DOUBLE(50.0, data.inputVoltage);
    TEST_ASSERT_EQUAL_DOUBLE(1000, data.tachometer);
    TEST_ASSERT_EQUAL_DOUBLE(2000, data.tachometerAbsolut);
    TEST_ASSERT_EQUAL_UINT8(0, data.fault);
    // the parser stamps the realtime timestamp on every realtime command
    TEST_ASSERT_EQUAL_UINT32(4711, parser.getLastRealtimeDataTimestamp());
}

void test_battery_drift_is_applied_to_input_voltage(void) {
    uint8_t values[26] = {0};
    values[0] = 0x32;
    values[15] = 0x01; values[16] = 0xF4;  // inputVoltage = 50.0

    VescCommandParser parser;
    VescData data;
    parser.parseCommand(values, sizeof(values), &data, 1.5, 0);

    TEST_ASSERT_EQUAL_DOUBLE(51.5, data.inputVoltage);
}

/*
 * A truncated frame must never read out of bounds: fields whose bytes are
 * missing decode to 0, fields that are fully present still decode correctly.
 */
void test_truncated_frame_is_bounds_safe(void) {
    uint8_t values[26] = {0};
    values[0] = 0x32;
    values[5] = 0x00; values[6] = 0xFA;  // mosfetTemp present (25.0)

    VescCommandParser parser;
    VescData data;
    parser.parseCommand(values, 7, &data, 0.0, 0);

    TEST_ASSERT_EQUAL_DOUBLE(25.0, data.mosfetTemp);   // fully inside the 7 bytes
    TEST_ASSERT_EQUAL_DOUBLE(0.0, data.erpm);          // beyond the truncated frame
    TEST_ASSERT_EQUAL_DOUBLE(0.0, data.inputVoltage);  // beyond the truncated frame
}

void test_empty_frame_is_ignored(void) {
    uint8_t values[1] = {0x32};

    VescCommandParser parser;
    VescData data;
    std::string type = parser.parseCommand(values, 0, &data, 0.0, 0);

    TEST_ASSERT_TRUE(type.empty());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, data.erpm);
}

void test_null_pointer_is_ignored(void) {
    VescCommandParser parser;
    VescData data;
    TEST_ASSERT_TRUE(parser.parseCommand(nullptr, 16, &data, 0.0, 0).empty());
}

void test_firmware_version(void) {
    uint8_t fw[16] = {0x00, 3, 4, 'T', 'e', 's', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0};

    VescCommandParser parser;
    VescData data;
    std::string type = parser.parseCommand(fw, sizeof(fw), &data, 0.0, 0);

    TEST_ASSERT_EQUAL_STRING("COMM_FW_VERSION", type.c_str());
    TEST_ASSERT_EQUAL_UINT8(3, data.majorVersion);
    TEST_ASSERT_EQUAL_UINT8(4, data.minorVersion);
    TEST_ASSERT_EQUAL_STRING("Test", data.name.c_str());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_get_values_selective_decodes_all_fields);
    RUN_TEST(test_battery_drift_is_applied_to_input_voltage);
    RUN_TEST(test_truncated_frame_is_bounds_safe);
    RUN_TEST(test_empty_frame_is_ignored);
    RUN_TEST(test_null_pointer_is_ignored);
    RUN_TEST(test_firmware_version);
    return UNITY_END();
}
