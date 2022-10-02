#include <unity.h>
#include "../../lib/vesc-protocol/src/VescParser.h"


void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void testCommandpingCan() {
    uint8_t buffer[512] = {2, 1, 62};

    VescMessage message =  VescParser::parseMessage(buffer);
    TEST_ASSERT_EQUAL(::SHORT, message.getType());
    TEST_ASSERT_EQUAL(1, message.getLength());
    TEST_ASSERT_EQUAL(62, message.getCommand());
}

void testCommandFirmwareVersion() {
    uint8_t buffer[512] = { 2, 1, 0 };

    VescMessage message =  VescParser::parseMessage(buffer);
    TEST_ASSERT_EQUAL(::SHORT, message.getType());
    TEST_ASSERT_EQUAL(1, message.getLength());
    TEST_ASSERT_EQUAL(0, message.getCommand());
}

void testCommandGetValuesSelective() {
    uint8_t buffer[512] = { 2, 5, 50, 0, 0, 135, 195, 35, 77, 3 };

    VescMessage message =  VescParser::parseMessage(buffer);
    TEST_ASSERT_EQUAL(::SHORT, message.getType());
    TEST_ASSERT_EQUAL(5, message.getLength());
    TEST_ASSERT_EQUAL(50, message.getCommand());
}

int main( int argc, char **argv) {
    UNITY_BEGIN(); // IMPORTANT LINE!
    RUN_TEST(testCommandpingCan);
    UNITY_END(); // stop unit testing
    return 0;
}
